#ifndef GUI_SCREEN_WIFI_SETUP_H
#define GUI_SCREEN_WIFI_SETUP_H

#include "lvgl.h"

// External screen object declaration
extern lv_obj_t *wifi_setup_screen;

// UI elements for WiFi setup
extern lv_obj_t *wifi_network_list;
extern lv_obj_t *wifi_password_textarea;
extern lv_obj_t *wifi_connect_btn;
extern lv_obj_t *wifi_status_label;
extern lv_obj_t *wifi_scan_btn;

/**
 * @brief Create WiFi setup screen
 */
void create_wifi_setup_screen(void);

/**
 * @brief Update WiFi network list with scan results
 */
void update_wifi_network_list(void);

/**
 * @brief Update WiFi connection status display
 * @param status WiFi connection status
 * @param message Status message to display
 */
void update_wifi_status_display(const char *status, const char *message);

/**
 * @brief Show/hide password input based on network security
 * @param show_password Whether to show password input
 */
void wifi_setup_show_password_input(bool show_password);

/**
 * @brief Clear password input field
 */
void wifi_setup_clear_password(void);

#endif // GUI_SCREEN_WIFI_SETUP_H