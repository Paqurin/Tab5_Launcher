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
 * @brief Destroy text editor screen and free memory
 */
void destroy_text_editor_screen(void);

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

/**
 * @brief Toggle search toolbar visibility
 */
void text_editor_toggle_search(void);

/**
 * @brief Perform search in text area
 * @param search_term Text to search for
 * @param forward Search direction (true = forward, false = backward)
 * @return true if match found
 */
bool text_editor_search(const char *search_term, bool forward);

/**
 * @brief Get current search result count and position
 * @param current_match Current match index (0-based)
 * @param total_matches Total number of matches
 */
void text_editor_get_search_info(int *current_match, int *total_matches);

/**
 * @brief Toggle syntax highlighting on/off
 */
void text_editor_toggle_syntax_highlighting(void);

/**
 * @brief Check if syntax highlighting is enabled
 * @return true if enabled
 */
bool text_editor_is_syntax_highlighting_enabled(void);

/**
 * @brief Apply syntax highlighting to current file
 */
void text_editor_apply_syntax_highlighting(void);

/**
 * @brief Clear syntax highlighting from text area
 */
void text_editor_clear_syntax_highlighting(void);

/**
 * @brief Get current file size in bytes
 * @return File size in bytes, 0 if no file loaded
 */
size_t text_editor_get_file_size(void);

/**
 * @brief Check if current file is considered large
 * @return true if file is above large file threshold (256KB)
 */
bool text_editor_is_large_file(void);

/**
 * @brief Get memory usage information for monitoring
 * @param internal_free Pointer to store free internal RAM bytes
 * @param spiram_free Pointer to store free SPIRAM bytes
 */
void text_editor_get_memory_info(size_t* internal_free, size_t* spiram_free);

#endif // GUI_SCREEN_TEXT_EDITOR_H