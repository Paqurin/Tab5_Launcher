#ifndef GUI_SCREEN_SETTINGS_H
#define GUI_SCREEN_SETTINGS_H

#include "lvgl.h"

/**
 * @brief Create the settings screen
 */
void create_settings_screen(void);

/**
 * @brief Get the settings screen object
 * @return Pointer to the settings screen object, NULL if not created
 */
lv_obj_t* get_settings_screen(void);

/**
 * @brief Destroy the settings screen and free resources
 */
void destroy_settings_screen(void);

#endif // GUI_SCREEN_SETTINGS_H