#ifndef PYTHON_ENGINE_H
#define PYTHON_ENGINE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Python engine status
typedef enum {
    PYTHON_ENGINE_IDLE = 0,
    PYTHON_ENGINE_RUNNING,
    PYTHON_ENGINE_ERROR,
    PYTHON_ENGINE_STOPPED
} python_engine_status_t;

// Python script execution result
typedef struct {
    bool success;
    char *output;           // Script output (malloc'd, must be freed)
    char *error_message;    // Error message if failed (malloc'd, must be freed)
    size_t output_len;
    size_t error_len;
} python_execution_result_t;

/**
 * @brief Initialize MicroPython engine
 * @return ESP_OK on success
 */
esp_err_t python_engine_init(void);

/**
 * @brief Deinitialize MicroPython engine and free resources
 */
void python_engine_deinit(void);

/**
 * @brief Execute Python script from string
 * @param script Python script code
 * @param result Execution result (output must be freed by caller)
 * @return ESP_OK on success
 */
esp_err_t python_engine_execute_string(const char *script, python_execution_result_t *result);

/**
 * @brief Execute Python script from file
 * @param file_path Path to Python script file
 * @param result Execution result (output must be freed by caller)
 * @return ESP_OK on success
 */
esp_err_t python_engine_execute_file(const char *file_path, python_execution_result_t *result);

/**
 * @brief Get current engine status
 * @return Current status
 */
python_engine_status_t python_engine_get_status(void);

/**
 * @brief Stop running script execution
 */
void python_engine_stop(void);

/**
 * @brief Check if Python file is supported
 * @param filename File name to check
 * @return true if supported (.py, .pyw files)
 */
bool python_engine_is_supported_file(const char *filename);

/**
 * @brief Free execution result memory
 * @param result Result structure to free
 */
void python_engine_free_result(python_execution_result_t *result);

/**
 * @brief Get available heap memory for Python
 * @return Available bytes
 */
size_t python_engine_get_available_heap(void);

/**
 * @brief Start interactive REPL mode
 * @return ESP_OK on success
 */
esp_err_t python_engine_start_repl(void);

/**
 * @brief Execute REPL line
 * @param line Input line
 * @param result Execution result
 * @return ESP_OK on success
 */
esp_err_t python_engine_repl_execute(const char *line, python_execution_result_t *result);

/**
 * @brief Stop REPL mode
 */
void python_engine_stop_repl(void);

/**
 * @brief Check if REPL is active
 * @return true if REPL is running
 */
bool python_engine_is_repl_active(void);

#ifdef __cplusplus
}
#endif

#endif // PYTHON_ENGINE_H