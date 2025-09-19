#ifndef PYTHON_EXECUTOR_H
#define PYTHON_EXECUTOR_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Python execution result
 */
typedef struct {
    bool success;
    char *output;           // Script output (must be freed)
    char *error;            // Error message (must be freed)
    size_t output_len;
    size_t error_len;
    int exit_code;
} python_exec_result_t;

/**
 * @brief Python execution options
 */
typedef struct {
    bool capture_output;    // Capture stdout/stderr
    bool interactive;       // Interactive mode (REPL)
    size_t timeout_ms;      // Execution timeout (0 = no timeout)
} python_exec_options_t;

/**
 * @brief Execute Python code string
 * @param code Python code to execute
 * @param options Execution options (NULL for defaults)
 * @param result Execution result
 * @return ESP_OK on success
 */
esp_err_t python_execute_string(const char *code, const python_exec_options_t *options,
                                python_exec_result_t *result);

/**
 * @brief Execute Python script file
 * @param filepath Path to Python script
 * @param options Execution options (NULL for defaults)
 * @param result Execution result
 * @return ESP_OK on success
 */
esp_err_t python_execute_file(const char *filepath, const python_exec_options_t *options,
                              python_exec_result_t *result);

/**
 * @brief Free execution result
 * @param result Result to free
 */
void python_exec_result_free(python_exec_result_t *result);

/**
 * @brief Start Python REPL (Read-Eval-Print Loop)
 * @return ESP_OK on success
 */
esp_err_t python_start_repl(void);

/**
 * @brief Stop Python REPL
 */
void python_stop_repl(void);

/**
 * @brief Check if REPL is running
 * @return true if REPL is active
 */
bool python_is_repl_running(void);

/**
 * @brief Execute line in REPL context
 * @param line Line to execute
 * @param result Execution result
 * @return ESP_OK on success
 */
esp_err_t python_repl_execute_line(const char *line, python_exec_result_t *result);

#ifdef __cplusplus
}
#endif

#endif // PYTHON_EXECUTOR_H