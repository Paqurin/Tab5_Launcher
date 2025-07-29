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

#endif // GUI_EVENTS_H