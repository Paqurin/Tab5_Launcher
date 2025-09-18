#ifndef GUI_STATUS_BAR_H
#define GUI_STATUS_BAR_H

#include "lvgl.h"
#include <stdbool.h>

/**
 * @brief Status bar component for displaying power, network, and system status
 */
typedef struct {
    lv_obj_t *container;
    lv_obj_t *voltage_label;
    lv_obj_t *voltage_unit_label;
    lv_obj_t *current_label;
    lv_obj_t *current_unit_label;
    lv_obj_t *charging_label;
    lv_obj_t *sdcard_label;
    lv_obj_t *wifi_label;
} gui_status_bar_t;

/**
 * @brief Create persistent status bar component
 * @param parent Parent object to attach status bar to
 * @return Pointer to status bar structure
 */
gui_status_bar_t* gui_status_bar_create(lv_obj_t *parent);

/**
 * @brief Destroy status bar component
 * @param status_bar Status bar to destroy
 */
void gui_status_bar_destroy(gui_status_bar_t *status_bar);

/**
 * @brief Update power monitoring data in status bar
 * @param status_bar Status bar to update
 * @param voltage Battery voltage in volts
 * @param current_ma Current consumption in mA
 * @param charging Whether device is charging
 */
void gui_status_bar_update_power(gui_status_bar_t *status_bar, float voltage, float current_ma, bool charging);

/**
 * @brief Update WiFi status in status bar
 * @param status_bar Status bar to update
 * @param connected Whether WiFi is connected
 * @param rssi Signal strength (if connected)
 */
void gui_status_bar_update_wifi(gui_status_bar_t *status_bar, bool connected, int8_t rssi);

/**
 * @brief Update SD card status in status bar
 * @param status_bar Status bar to update
 */
void gui_status_bar_update_sdcard(gui_status_bar_t *status_bar);

/**
 * @brief Set status bar visibility
 * @param status_bar Status bar to modify
 * @param visible Whether status bar should be visible
 */
void gui_status_bar_set_visible(gui_status_bar_t *status_bar, bool visible);

/**
 * @brief Get status bar container object for layout purposes
 * @param status_bar Status bar instance
 * @return LVGL container object
 */
lv_obj_t* gui_status_bar_get_container(gui_status_bar_t *status_bar);

#endif // GUI_STATUS_BAR_H