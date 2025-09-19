#include "python_engine.h"
#include "micropython_wrapper.h"
#include "python_executor.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "python_engine";

// Engine state
static python_engine_status_t engine_status = PYTHON_ENGINE_IDLE;
static bool repl_active = false;

// Memory management
#define PYTHON_HEAP_SIZE (128 * 1024)  // 128KB heap for Python
static void *python_heap = NULL;

// Output capture buffers
#define MAX_OUTPUT_SIZE (4096)
static char output_buffer[MAX_OUTPUT_SIZE];
static size_t output_pos = 0;

esp_err_t python_engine_init(void)
{
    if (engine_status != PYTHON_ENGINE_IDLE) {
        ESP_LOGW(TAG, "Engine already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing MicroPython engine");

    // Configure MicroPython wrapper
    micropython_config_t config = {
        .heap_size = PYTHON_HEAP_SIZE,
        .stack_size = 16 * 1024,
        .task_priority = 5,
        .use_psram = true
    };

    // Initialize MicroPython wrapper
    esp_err_t ret = micropython_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MicroPython wrapper");
        return ret;
    }

    output_pos = 0;
    memset(output_buffer, 0, sizeof(output_buffer));

    engine_status = PYTHON_ENGINE_IDLE;
    ESP_LOGI(TAG, "MicroPython engine initialized successfully");
    ESP_LOGI(TAG, "Version: %s", micropython_get_version());

    return ESP_OK;
}

void python_engine_deinit(void)
{
    if (engine_status == PYTHON_ENGINE_IDLE) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing MicroPython engine");

    // Stop REPL if running
    if (repl_active) {
        python_stop_repl();
        repl_active = false;
    }

    // Deinitialize MicroPython wrapper
    micropython_deinit();

    if (python_heap) {
        heap_caps_free(python_heap);
        python_heap = NULL;
    }

    engine_status = PYTHON_ENGINE_IDLE;
}

esp_err_t python_engine_execute_string(const char *script, python_execution_result_t *result)
{
    if (!script || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    if (engine_status == PYTHON_ENGINE_RUNNING) {
        ESP_LOGW(TAG, "Engine is busy");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Executing Python script: %.50s%s", script, strlen(script) > 50 ? "..." : "");

    // Initialize result
    memset(result, 0, sizeof(python_execution_result_t));

    engine_status = PYTHON_ENGINE_RUNNING;

    // Execute using wrapper
    python_exec_result_t exec_result;
    python_exec_options_t options = {
        .capture_output = true,
        .interactive = false,
        .timeout_ms = 10000
    };

    esp_err_t ret = python_execute_string(script, &options, &exec_result);

    if (ret == ESP_OK) {
        // Convert wrapper result to engine result format
        result->success = exec_result.success;

        if (exec_result.output && exec_result.output_len > 0) {
            result->output_len = exec_result.output_len;
            result->output = malloc(result->output_len + 1);
            if (result->output) {
                strcpy(result->output, exec_result.output);
            }
        }

        if (exec_result.error && exec_result.error_len > 0) {
            result->error_len = exec_result.error_len;
            result->error_message = malloc(result->error_len + 1);
            if (result->error_message) {
                strcpy(result->error_message, exec_result.error);
            }
        }

        // Clean up wrapper result
        python_exec_result_free(&exec_result);
    } else {
        result->success = false;
        result->error_message = malloc(64);
        if (result->error_message) {
            snprintf(result->error_message, 64, "Execution failed with error: 0x%x", ret);
            result->error_len = strlen(result->error_message);
        }
    }

    engine_status = PYTHON_ENGINE_IDLE;
    return ESP_OK;
}

esp_err_t python_engine_execute_file(const char *file_path, python_execution_result_t *result)
{
    if (!file_path || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Executing Python file: %s", file_path);

    // Initialize result
    memset(result, 0, sizeof(python_execution_result_t));

    if (engine_status == PYTHON_ENGINE_RUNNING) {
        ESP_LOGW(TAG, "Engine is busy");
        return ESP_ERR_INVALID_STATE;
    }

    engine_status = PYTHON_ENGINE_RUNNING;

    // Execute using wrapper
    python_exec_result_t exec_result;
    python_exec_options_t options = {
        .capture_output = true,
        .interactive = false,
        .timeout_ms = 15000  // Longer timeout for files
    };

    esp_err_t ret = python_execute_file(file_path, &options, &exec_result);

    if (ret == ESP_OK) {
        // Convert wrapper result to engine result format
        result->success = exec_result.success;

        if (exec_result.output && exec_result.output_len > 0) {
            result->output_len = exec_result.output_len;
            result->output = malloc(result->output_len + 1);
            if (result->output) {
                strcpy(result->output, exec_result.output);
            }
        }

        if (exec_result.error && exec_result.error_len > 0) {
            result->error_len = exec_result.error_len;
            result->error_message = malloc(result->error_len + 1);
            if (result->error_message) {
                strcpy(result->error_message, exec_result.error);
            }
        }

        // Clean up wrapper result
        python_exec_result_free(&exec_result);
    } else {
        result->success = false;
        result->error_message = malloc(128);
        if (result->error_message) {
            snprintf(result->error_message, 128, "Failed to execute file: %s (error: 0x%x)", file_path, ret);
            result->error_len = strlen(result->error_message);
        }
    }

    engine_status = PYTHON_ENGINE_IDLE;
    return ESP_OK;
}

python_engine_status_t python_engine_get_status(void)
{
    return engine_status;
}

void python_engine_stop(void)
{
    if (engine_status == PYTHON_ENGINE_RUNNING) {
        ESP_LOGI(TAG, "Stopping Python execution");
        // TODO: Interrupt MicroPython execution
        engine_status = PYTHON_ENGINE_STOPPED;
    }
}

bool python_engine_is_supported_file(const char *filename)
{
    if (!filename) {
        return false;
    }

    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return false;
    }

    return (strcmp(ext, ".py") == 0 || strcmp(ext, ".pyw") == 0);
}

void python_engine_free_result(python_execution_result_t *result)
{
    if (!result) {
        return;
    }

    if (result->output) {
        free(result->output);
        result->output = NULL;
    }

    if (result->error_message) {
        free(result->error_message);
        result->error_message = NULL;
    }

    result->output_len = 0;
    result->error_len = 0;
    result->success = false;
}

size_t python_engine_get_available_heap(void)
{
    if (!micropython_is_initialized()) {
        return 0;
    }

    // Get actual available heap from MicroPython wrapper
    return micropython_get_heap_free();
}

esp_err_t python_engine_start_repl(void)
{
    if (repl_active) {
        ESP_LOGW(TAG, "REPL already active");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting Python REPL");

    // Start REPL using wrapper
    esp_err_t ret = python_start_repl();
    if (ret == ESP_OK) {
        repl_active = true;
    }

    return ret;
}

esp_err_t python_engine_repl_execute(const char *line, python_execution_result_t *result)
{
    if (!repl_active) {
        ESP_LOGE(TAG, "REPL not active");
        return ESP_ERR_INVALID_STATE;
    }

    if (!line || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "REPL execute: %s", line);

    // Initialize result
    memset(result, 0, sizeof(python_execution_result_t));

    // Execute using wrapper REPL
    python_exec_result_t exec_result;
    esp_err_t ret = python_repl_execute_line(line, &exec_result);

    if (ret == ESP_OK) {
        // Convert wrapper result to engine result format
        result->success = exec_result.success;

        if (exec_result.output && exec_result.output_len > 0) {
            result->output_len = exec_result.output_len;
            result->output = malloc(result->output_len + 1);
            if (result->output) {
                strcpy(result->output, exec_result.output);
            }
        }

        if (exec_result.error && exec_result.error_len > 0) {
            result->error_len = exec_result.error_len;
            result->error_message = malloc(result->error_len + 1);
            if (result->error_message) {
                strcpy(result->error_message, exec_result.error);
            }
        }

        // Clean up wrapper result
        python_exec_result_free(&exec_result);
    } else {
        result->success = false;
        result->error_message = malloc(64);
        if (result->error_message) {
            snprintf(result->error_message, 64, "REPL execution failed: 0x%x", ret);
            result->error_len = strlen(result->error_message);
        }
    }

    return ESP_OK;
}

void python_engine_stop_repl(void)
{
    if (!repl_active) {
        return;
    }

    ESP_LOGI(TAG, "Stopping Python REPL");

    // Stop REPL using wrapper
    python_stop_repl();
    repl_active = false;
}

bool python_engine_is_repl_active(void)
{
    return repl_active;
}