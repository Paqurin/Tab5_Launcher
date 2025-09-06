#ifndef GUI_PULLDOWN_MENU_H
#define GUI_PULLDOWN_MENU_H

#include "lvgl.h"
#include <stdbool.h>

/**
 * @brief Pull-down menu for quick settings access
 */
typedef struct {
    lv_obj_t *container;
    lv_obj_t *backdrop;
    lv_obj_t *brightness_slider;
    lv_obj_t *brightness_label;
    lv_obj_t *wifi_toggle;
    lv_obj_t *sd_toggle;
    lv_obj_t *close_btn;
    bool is_open;
    lv_anim_t slide_anim;
} gui_pulldown_menu_t;

/**
 * @brief Create pull-down menu (initially hidden)
 * @param parent Parent screen to attach menu to
 * @return Pointer to menu structure
 */
gui_pulldown_menu_t* gui_pulldown_menu_create(lv_obj_t *parent);

/**
 * @brief Destroy pull-down menu
 * @param menu Menu to destroy
 */
void gui_pulldown_menu_destroy(gui_pulldown_menu_t *menu);

/**
 * @brief Show the pull-down menu with animation
 * @param menu Menu to show
 */
void gui_pulldown_menu_show(gui_pulldown_menu_t *menu);

/**
 * @brief Hide the pull-down menu with animation
 * @param menu Menu to hide
 */
void gui_pulldown_menu_hide(gui_pulldown_menu_t *menu);

/**
 * @brief Toggle menu visibility
 * @param menu Menu to toggle
 */
void gui_pulldown_menu_toggle(gui_pulldown_menu_t *menu);

/**
 * @brief Check if menu is currently open
 * @param menu Menu to check
 * @return true if open, false if closed
 */
bool gui_pulldown_menu_is_open(gui_pulldown_menu_t *menu);

#endif // GUI_PULLDOWN_MENU_H