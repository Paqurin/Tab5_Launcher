#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include "lvgl.h"
#include "esp_err.h"

/**
 * @brief Initialize GUI manager
 * @param disp LVGL display object
 * @return ESP_OK on success
 */
esp_err_t gui_manager_init(lv_display_t *disp);

/**
 * @brief Update GUI (call this in main loop)
 */
void gui_manager_update(void);

#endif // GUI_MANAGER_H