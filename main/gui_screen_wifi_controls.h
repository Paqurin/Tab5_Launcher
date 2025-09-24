#ifndef GUI_SCREEN_WIFI_CONTROLS_H
#define GUI_SCREEN_WIFI_CONTROLS_H

#include "lvgl.h"

// External screen object declaration
extern lv_obj_t *wifi_controls_screen;

/**
 * @brief Create WiFi controls screen
 */
void create_wifi_controls_screen(void);

/**
 * @brief Show WiFi controls screen
 */
void show_wifi_controls_screen(void);

/**
 * @brief Handle back button from WiFi controls screen
 */
void wifi_controls_screen_back(void);

/**
 * @brief Destroy WiFi controls screen and free memory
 */
void destroy_wifi_controls_screen(void);

#endif // GUI_SCREEN_WIFI_CONTROLS_H