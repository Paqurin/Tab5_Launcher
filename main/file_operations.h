#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

// File type enumeration for icon display
typedef enum {
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_BINARY,     // .bin files
    FILE_TYPE_TEXT,       // .txt, .cfg, .conf, .json
    FILE_TYPE_PYTHON,     // .py files
    FILE_TYPE_WEB,        // .html, .css, .js
    FILE_TYPE_IMAGE,      // .jpg, .png, .bmp
    FILE_TYPE_OTHER
} file_type_t;

// Extended file information structure
typedef struct {
    char name[256];
    size_t size;
    bool is_directory;
    file_type_t type;
    time_t modified_time;
} file_info_t;

/**
 * @brief Create a new directory
 * @param path Directory path (relative to SD root)
 * @return ESP_OK on success
 */
esp_err_t file_ops_create_directory(const char *path);

/**
 * @brief Delete a file
 * @param path File path (relative to SD root)
 * @return ESP_OK on success
 */
esp_err_t file_ops_delete_file(const char *path);

/**
 * @brief Delete a directory recursively
 * @param path Directory path (relative to SD root)
 * @return ESP_OK on success
 */
esp_err_t file_ops_delete_directory(const char *path);

/**
 * @brief Rename a file or directory
 * @param old_path Current path (relative to SD root)
 * @param new_path New path (relative to SD root)
 * @return ESP_OK on success
 */
esp_err_t file_ops_rename(const char *old_path, const char *new_path);

/**
 * @brief Copy a file
 * @param src_path Source file path (relative to SD root)
 * @param dst_path Destination file path (relative to SD root)
 * @return ESP_OK on success
 */
esp_err_t file_ops_copy_file(const char *src_path, const char *dst_path);

/**
 * @brief Copy a directory recursively
 * @param src_path Source directory path (relative to SD root)
 * @param dst_path Destination directory path (relative to SD root)
 * @return ESP_OK on success
 */
esp_err_t file_ops_copy_directory(const char *src_path, const char *dst_path);

/**
 * @brief Copy file/directory path to clipboard for later paste
 * @param path File/directory path (relative to SD root)
 * @param cut If true, will delete source after paste (cut operation)
 * @return ESP_OK on success
 */
esp_err_t file_ops_copy_to_clipboard(const char *path, bool cut);

/**
 * @brief Paste file/directory from clipboard to destination
 * @param dst_dir Destination directory path (relative to SD root)
 * @return ESP_OK on success
 */
esp_err_t file_ops_paste_from_clipboard(const char *dst_dir);

/**
 * @brief Check if clipboard has content
 * @return true if clipboard has content
 */
bool file_ops_clipboard_has_content(void);

/**
 * @brief Get current clipboard path
 * @return Path in clipboard or NULL if empty
 */
const char* file_ops_get_clipboard_path(void);

/**
 * @brief Get detailed file information
 * @param path File path (relative to SD root)
 * @param info Pointer to file_info_t structure to fill
 * @return ESP_OK on success
 */
esp_err_t file_ops_get_file_info(const char *path, file_info_t *info);

#endif // FILE_OPERATIONS_H