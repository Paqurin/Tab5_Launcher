#ifndef GUI_EVENTS_H
#define GUI_EVENTS_H

#include "lvgl.h"

/**
 * @brief Main menu button event handler
 */
void main_menu_event_handler(lv_event_t *e);

/**
 * @brief File list item event handler
 */
void file_list_event_handler(lv_event_t *e);

/**
 * @brief Firmware list item event handler
 */
void firmware_list_event_handler(lv_event_t *e);

/**
 * @brief Flash firmware button event handler
 */
void flash_firmware_event_handler(lv_event_t *e);

/**
 * @brief Back button event handler
 */
void back_button_event_handler(lv_event_t *e);

/**
 * @brief Splash screen button event handler
 */
void splash_button_event_handler(lv_event_t *e);

/**
 * @brief Format SD card button event handler
 */
void format_button_event_handler(lv_event_t *e);

/**
 * @brief Mount/Unmount SD card button event handler
 */
void mount_unmount_button_event_handler(lv_event_t *e);

/**
 * @brief Directory up navigation event handler
 */
void directory_up_event_handler(lv_event_t *e);

/**
 * @brief File selection checkbox event handler
 */
void file_selection_event_handler(lv_event_t *e);

/**
 * @brief Toggle file selection mode event handler
 */
void toggle_selection_mode_event_handler(lv_event_t *e);

/**
 * @brief File operation event handlers
 */
void delete_files_event_handler(lv_event_t *e);
void copy_files_event_handler(lv_event_t *e);
void move_files_event_handler(lv_event_t *e);
void paste_files_event_handler(lv_event_t *e);
void rename_file_event_handler(lv_event_t *e);

/**
 * @brief File browser v2 navigation event handlers
 */
void file_browser_v2_up_event_handler(lv_event_t *e);
void file_browser_v2_prev_page_handler(lv_event_t *e);
void file_browser_v2_next_page_handler(lv_event_t *e);
void file_browser_v2_item_click_handler(lv_event_t *e);
void file_browser_v2_multi_select_handler(lv_event_t *e);

#endif // GUI_EVENTS_H