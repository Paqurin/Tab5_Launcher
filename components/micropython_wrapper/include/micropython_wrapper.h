#ifndef MICROPYTHON_WRAPPER_H
#define MICROPYTHON_WRAPPER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MicroPython wrapper configuration
 */
typedef struct {
    size_t heap_size;       // Python heap size in bytes
    size_t stack_size;      // Python task stack size
    int task_priority;      // FreeRTOS task priority
    bool use_psram;         // Use PSRAM for heap if available
} micropython_config_t;

/**
 * @brief Initialize MicroPython runtime
 * @param config Configuration (NULL for defaults)
 * @return ESP_OK on success
 */
esp_err_t micropython_init(const micropython_config_t *config);

/**
 * @brief Deinitialize MicroPython runtime
 */
void micropython_deinit(void);

/**
 * @brief Check if MicroPython is initialized
 * @return true if initialized
 */
bool micropython_is_initialized(void);

/**
 * @brief Get MicroPython version string
 * @return Version string
 */
const char* micropython_get_version(void);

/**
 * @brief Get available Python heap memory
 * @return Available bytes
 */
size_t micropython_get_heap_free(void);

/**
 * @brief Get total Python heap size
 * @return Total heap bytes
 */
size_t micropython_get_heap_total(void);

#ifdef __cplusplus
}
#endif

#endif // MICROPYTHON_WRAPPER_H