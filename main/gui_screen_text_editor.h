#ifndef GUI_SCREEN_TEXT_EDITOR_H
#define GUI_SCREEN_TEXT_EDITOR_H

#include "lvgl.h"
#include "esp_err.h"

// Text editor screen object
extern lv_obj_t *text_editor_screen;

// File type support
typedef enum {
    TEXT_FILE_TXT = 0,
    TEXT_FILE_JSON,
    TEXT_FILE_PY,
    TEXT_FILE_HTML,
    TEXT_FILE_CSS,
    TEXT_FILE_JS,
    TEXT_FILE_LOG,
    TEXT_FILE_CONFIG,
    TEXT_FILE_UNKNOWN
} text_file_type_t;

/**
 * @brief Create text editor screen
 */
void create_text_editor_screen(void);

/**
 * @brief Open file in text editor
 * @param file_path Full path to file
 * @return ESP_OK on success
 */
esp_err_t text_editor_open_file(const char *file_path);

/**
 * @brief Save current file
 * @return ESP_OK on success
 */
esp_err_t text_editor_save_file(void);

/**
 * @brief Close text editor and return to file manager
 */
void text_editor_close(void);

/**
 * @brief Get file type from extension
 * @param filename File name with extension
 * @return File type enum
 */
text_file_type_t text_editor_get_file_type(const char *filename);

/**
 * @brief Check if file is supported for editing
 * @param filename File name with extension
 * @return true if supported
 */
bool text_editor_is_supported_file(const char *filename);

#endif // GUI_SCREEN_TEXT_EDITOR_H