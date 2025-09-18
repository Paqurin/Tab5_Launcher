#ifndef GUI_SCREEN_TOOLS_H
#define GUI_SCREEN_TOOLS_H

#include "lvgl.h"
#include "esp_err.h"

// Tools screen object
extern lv_obj_t *tools_screen;

/**
 * @brief Create tools menu screen
 */
void create_tools_screen(void);

/**
 * @brief Show tools screen
 */
void show_tools_screen(void);

/**
 * @brief Handle back navigation from tools screen
 */
void tools_screen_back(void);

/**
 * @brief Destroy tools screen and free memory
 */
void destroy_tools_screen(void);

#endif // GUI_SCREEN_TOOLS_H