#include "python_executor.h"
#include "micropython_wrapper.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "python_executor";

// REPL state
static bool repl_running = false;

// Default execution options
static const python_exec_options_t default_options = {
    .capture_output = true,
    .interactive = false,
    .timeout_ms = 10000  // 10 second timeout
};

// Helper function to read file content
static char* read_file_content(const char *filepath, size_t *file_size)
{
    if (!filepath || !file_size) {
        return NULL;
    }

    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (*file_size == 0) {
        ESP_LOGW(TAG, "File is empty: %s", filepath);
        fclose(file);
        return NULL;
    }

    if (*file_size > 32768) {  // 32KB limit for scripts
        ESP_LOGE(TAG, "Script file too large: %zu bytes", *file_size);
        fclose(file);
        return NULL;
    }

    // Allocate buffer
    char *content = malloc(*file_size + 1);
    if (!content) {
        ESP_LOGE(TAG, "Failed to allocate memory for file content");
        fclose(file);
        return NULL;
    }

    // Read content
    size_t read_size = fread(content, 1, *file_size, file);
    content[read_size] = '\0';
    *file_size = read_size;

    fclose(file);
    return content;
}

// Helper function to create execution result
static void create_exec_result(python_exec_result_t *result, bool success,
                              const char *output, const char *error, int exit_code)
{
    if (!result) return;

    memset(result, 0, sizeof(python_exec_result_t));
    result->success = success;
    result->exit_code = exit_code;

    if (output && strlen(output) > 0) {
        result->output_len = strlen(output);
        result->output = malloc(result->output_len + 1);
        if (result->output) {
            strcpy(result->output, output);
        }
    }

    if (error && strlen(error) > 0) {
        result->error_len = strlen(error);
        result->error = malloc(result->error_len + 1);
        if (result->error) {
            strcpy(result->error, error);
        }
    }
}

#ifdef MICROPYTHON_STUB_ONLY

// Stub implementation for development and testing
esp_err_t python_execute_string(const char *code, const python_exec_options_t *options,
                                python_exec_result_t *result)
{
    if (!code || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!micropython_is_initialized()) {
        ESP_LOGE(TAG, "MicroPython not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    const python_exec_options_t *opts = options ? options : &default_options;

    ESP_LOGI(TAG, "Executing Python code (STUB): %.50s%s",
             code, strlen(code) > 50 ? "..." : "");

    // Simulate script execution
    char output_buffer[512];
    char error_buffer[256];

    // Simple pattern matching for demo
    if (strstr(code, "print(")) {
        // Extract print statement content (very basic)
        const char *start = strstr(code, "print(");
        if (start) {
            start += 6; // Skip "print("
            const char *end = strchr(start, ')');
            if (end) {
                size_t len = end - start;
                if (len > 0 && len < 400) {
                    snprintf(output_buffer, sizeof(output_buffer),
                             "STUB OUTPUT: %.*s\n", (int)len, start);
                } else {
                    snprintf(output_buffer, sizeof(output_buffer), "STUB OUTPUT: print statement\n");
                }
            } else {
                snprintf(output_buffer, sizeof(output_buffer), "STUB OUTPUT: incomplete print\n");
            }
        }

        create_exec_result(result, true, output_buffer, NULL, 0);
    } else if (strstr(code, "error") || strstr(code, "raise")) {
        // Simulate error
        snprintf(error_buffer, sizeof(error_buffer),
                 "STUB ERROR: Simulated Python error in script");
        create_exec_result(result, false, NULL, error_buffer, 1);
    } else {
        // Generic execution
        snprintf(output_buffer, sizeof(output_buffer),
                 "STUB: Executed %zu bytes of Python code\nNo visible output", strlen(code));
        create_exec_result(result, true, output_buffer, NULL, 0);
    }

    ESP_LOGI(TAG, "Python execution completed (stub mode)");
    return ESP_OK;
}

#else

// Real MicroPython implementation
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/stackctrl.h"

// Output capture buffer
#define OUTPUT_BUFFER_SIZE 4096
static char output_buffer[OUTPUT_BUFFER_SIZE];
static size_t output_pos = 0;
static bool capture_output = false;

// Output capture functions
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (capture_output && output_pos + len < OUTPUT_BUFFER_SIZE - 1) {
        memcpy(output_buffer + output_pos, str, len);
        output_pos += len;
        output_buffer[output_pos] = '\0';
    }
    // Also send to console
    fwrite(str, 1, len, stdout);
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

void mp_hal_stdout_tx_char(char c) {
    mp_hal_stdout_tx_strn(&c, 1);
}

esp_err_t python_execute_string(const char *code, const python_exec_options_t *options,
                                python_exec_result_t *result)
{
    if (!code || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!micropython_is_initialized()) {
        ESP_LOGE(TAG, "MicroPython not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    const python_exec_options_t *opts = options ? options : &default_options;

    ESP_LOGI(TAG, "Executing Python code: %.50s%s",
             code, strlen(code) > 50 ? "..." : "");

    // Reset output capture
    if (opts->capture_output) {
        capture_output = true;
        output_pos = 0;
        memset(output_buffer, 0, sizeof(output_buffer));
    }

    // Execute Python code
    nlr_buf_t nlr;
    bool success = false;
    char error_msg[256] = {0};

    if (nlr_push(&nlr) == 0) {
        // Compile the code
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, code, strlen(code), 0);
        if (lex == NULL) {
            snprintf(error_msg, sizeof(error_msg), "Failed to create lexer");
        } else {
            qstr source_name = lex->source_name;
            mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);

            if (parse_tree.root == NULL) {
                snprintf(error_msg, sizeof(error_msg), "Syntax error in Python code");
            } else {
                mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);

                if (MP_OBJ_IS_EXCEPTION_INSTANCE(module_fun)) {
                    snprintf(error_msg, sizeof(error_msg), "Compilation error");
                } else {
                    // Execute the compiled code
                    mp_call_function_0(module_fun);
                    success = true;
                }
            }
        }
        nlr_pop();
    } else {
        // Handle exceptions
        mp_obj_t exc = MP_OBJ_FROM_PTR(nlr.ret_val);
        if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t*)MP_OBJ_TO_PTR(exc))->type),
                                    MP_OBJ_FROM_PTR(&mp_type_SystemExit))) {
            snprintf(error_msg, sizeof(error_msg), "SystemExit");
        } else {
            snprintf(error_msg, sizeof(error_msg), "Runtime error");
        }
    }

    // Stop output capture
    capture_output = false;

    // Create result
    if (success) {
        create_exec_result(result, true, output_buffer, NULL, 0);
    } else {
        create_exec_result(result, false, output_buffer, error_msg, 1);
    }

    ESP_LOGI(TAG, "Python execution %s", success ? "successful" : "failed");
    return ESP_OK;
}

#endif // MICROPYTHON_STUB_ONLY

esp_err_t python_execute_file(const char *filepath, const python_exec_options_t *options,
                              python_exec_result_t *result)
{
    if (!filepath || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Executing Python file: %s", filepath);

    // Read file content
    size_t file_size;
    char *content = read_file_content(filepath, &file_size);
    if (!content) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "Failed to read file: %s", filepath);
        create_exec_result(result, false, NULL, error_msg, 2);
        return ESP_ERR_NOT_FOUND;
    }

    // Execute the content
    esp_err_t ret = python_execute_string(content, options, result);

    // Add file info to output if successful
    if (ret == ESP_OK && result->success && result->output) {
        char *new_output = malloc(result->output_len + 256);
        if (new_output) {
            snprintf(new_output, result->output_len + 256,
                     "# Executed: %s (%zu bytes)\n%s",
                     filepath, file_size, result->output);
            free(result->output);
            result->output = new_output;
            result->output_len = strlen(new_output);
        }
    }

    free(content);
    return ret;
}

void python_exec_result_free(python_exec_result_t *result)
{
    if (!result) return;

    if (result->output) {
        free(result->output);
        result->output = NULL;
    }

    if (result->error) {
        free(result->error);
        result->error = NULL;
    }

    result->output_len = 0;
    result->error_len = 0;
    result->success = false;
    result->exit_code = 0;
}

esp_err_t python_start_repl(void)
{
    if (!micropython_is_initialized()) {
        ESP_LOGE(TAG, "MicroPython not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (repl_running) {
        ESP_LOGW(TAG, "REPL already running");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting Python REPL");

    // Initialize REPL
    repl_running = true;

    return ESP_OK;
}

void python_stop_repl(void)
{
    if (!repl_running) {
        return;
    }

    ESP_LOGI(TAG, "Stopping Python REPL");
    repl_running = false;
}

bool python_is_repl_running(void)
{
    return repl_running;
}

esp_err_t python_repl_execute_line(const char *line, python_exec_result_t *result)
{
    if (!line || !result) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!repl_running) {
        ESP_LOGE(TAG, "REPL not running");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "REPL execute: %s", line);

    // Use interactive execution for REPL
    python_exec_options_t repl_options = {
        .capture_output = true,
        .interactive = true,
        .timeout_ms = 5000  // Shorter timeout for REPL
    };

    return python_execute_string(line, &repl_options, result);
}