#ifndef GUI_PROGRESS_H
#define GUI_PROGRESS_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Initialize progress handling
 */
void gui_progress_init(void);

/**
 * @brief Update progress UI (call from main thread)
 */
void update_progress_ui(void);

/**
 * @brief Firmware progress callback
 */
void firmware_progress_callback(size_t bytes_written, size_t total_bytes, const char *step_description);

/**
 * @brief Flash firmware task
 */
void flash_firmware_task(void *pvParameters);

/**
 * @brief Check if flashing is in progress
 */
bool is_flashing_in_progress(void);

/**
 * @brief Set flashing state
 */
void set_flashing_state(bool state);

#endif // GUI_PROGRESS_H