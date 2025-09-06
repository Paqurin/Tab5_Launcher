#ifndef GUI_FILE_BROWSER_V2_H
#define GUI_FILE_BROWSER_V2_H

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

// File browser configuration (from config manager later)
#define FILES_PER_PAGE 8
#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_DISPLAY 40

// File browser state
typedef struct {
    char current_path[MAX_PATH_LENGTH];
    int current_page;
    int total_pages;
    int total_files;
    int selected_index;
    bool multi_select_mode;
    bool *selected_items;  // Array for multi-selection
    int items_per_page;
} file_browser_state_t;

// Enhanced file entry with more metadata
typedef struct {
    char name[256];
    char display_name[MAX_FILENAME_DISPLAY + 4]; // +4 for "..."
    bool is_directory;
    size_t size;
    uint32_t modified_time;
    const char *icon;
    lv_color_t color;
    char size_str[32];  // Human readable size
    char date_str[32];  // Human readable date
} file_entry_enhanced_t;

/**
 * @brief Create enhanced file browser screen
 * @return Pointer to the created screen object
 */
lv_obj_t* gui_file_browser_v2_create(void);

/**
 * @brief Update file list with current directory contents
 * @param screen The file browser screen
 */
void gui_file_browser_v2_update(lv_obj_t *screen);

/**
 * @brief Navigate to a directory
 * @param path The directory path to navigate to
 */
void gui_file_browser_v2_navigate(const char *path);

/**
 * @brief Go up one directory level
 */
void gui_file_browser_v2_go_up(void);

/**
 * @brief Navigate to next page of files
 */
void gui_file_browser_v2_next_page(void);

/**
 * @brief Navigate to previous page of files
 */
void gui_file_browser_v2_prev_page(void);

/**
 * @brief Enable/disable multi-selection mode
 * @param enable true to enable, false to disable
 */
void gui_file_browser_v2_set_multi_select(bool enable);

/**
 * @brief Get selected files in multi-select mode
 * @param indices Array to store selected indices
 * @return Number of selected files
 */
int gui_file_browser_v2_get_selected(int *indices);

/**
 * @brief Show file context menu (copy, paste, delete, etc.)
 * @param file_index Index of the file to show menu for
 */
void gui_file_browser_v2_show_context_menu(int file_index);

/**
 * @brief Get current file browser state
 * @return Pointer to current state
 */
file_browser_state_t* gui_file_browser_v2_get_state(void);

/**
 * @brief Set view mode (list, grid, detailed)
 * @param mode View mode from config
 */
void gui_file_browser_v2_set_view_mode(int mode);

/**
 * @brief Set sort mode (name, size, date, type)
 * @param mode Sort mode from config
 * @param ascending true for ascending, false for descending
 */
void gui_file_browser_v2_set_sort_mode(int mode, bool ascending);

/**
 * @brief Show/hide hidden files (starting with .)
 * @param show true to show, false to hide
 */
void gui_file_browser_v2_show_hidden_files(bool show);

/**
 * @brief Get icon for file type
 * @param filename File name to determine type
 * @param is_directory true if directory
 * @return LVGL symbol string for icon
 */
const char* gui_file_browser_v2_get_icon(const char *filename, bool is_directory);

/**
 * @brief Get color for file type
 * @param filename File name to determine type
 * @param is_directory true if directory
 * @return LVGL color for the file type
 */
lv_color_t gui_file_browser_v2_get_color(const char *filename, bool is_directory);

/**
 * @brief Format file size to human readable string
 * @param size Size in bytes
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 */
void gui_file_browser_v2_format_size(size_t size, char *buffer, size_t buffer_size);

/**
 * @brief Format timestamp to human readable string
 * @param timestamp Unix timestamp
 * @param buffer Buffer to store formatted string
 * @param buffer_size Size of buffer
 */
void gui_file_browser_v2_format_date(uint32_t timestamp, char *buffer, size_t buffer_size);

#endif // GUI_FILE_BROWSER_V2_H