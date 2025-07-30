#ifndef GUI_STYLES_H
#define GUI_STYLES_H

#include "lvgl.h"

// Color definitions for black-green theme
#define THEME_BG_COLOR          lv_color_hex(0x000000)    // Pure black background
#define THEME_PRIMARY_COLOR     lv_color_hex(0x00ff00)    // Bright green
#define THEME_SECONDARY_COLOR   lv_color_hex(0x008000)    // Dark green
#define THEME_TEXT_COLOR        lv_color_hex(0x00ff00)    // Green text
#define THEME_TEXT_MUTED        lv_color_hex(0x808080)    // Gray text
#define THEME_BORDER_COLOR      lv_color_hex(0x00ff00)    // Green borders
#define THEME_ERROR_COLOR       lv_color_hex(0xff0000)    // Red for errors
#define THEME_WARNING_COLOR     lv_color_hex(0xffff00)    // Yellow for warnings
#define THEME_SUCCESS_COLOR     lv_color_hex(0x00ff00)    // Green for success

// Font sizes
#define THEME_FONT_LARGE        &lv_font_montserrat_28
#define THEME_FONT_MEDIUM       &lv_font_montserrat_24
#define THEME_FONT_NORMAL       &lv_font_montserrat_20
#define THEME_FONT_SMALL        &lv_font_montserrat_16

// Style objects (extern declarations)
extern lv_style_t style_screen;
extern lv_style_t style_button;
extern lv_style_t style_button_pressed;
extern lv_style_t style_list;
extern lv_style_t style_list_item;
extern lv_style_t style_list_item_selected;
extern lv_style_t style_title;
extern lv_style_t style_text;
extern lv_style_t style_text_muted;

/**
 * @brief Initialize all GUI styles
 */
void gui_styles_init(void);

/**
 * @brief Apply button style to an object
 */
void apply_button_style(lv_obj_t *obj);

/**
 * @brief Apply list style to an object
 */
void apply_list_style(lv_obj_t *obj);

/**
 * @brief Apply list item style to an object
 */
void apply_list_item_style(lv_obj_t *obj);

/**
 * @brief Apply title style to a label
 */
void apply_title_style(lv_obj_t *label);

/**
 * @brief Apply normal text style to a label
 */
void apply_text_style(lv_obj_t *label);

/**
 * @brief Apply muted text style to a label
 */
void apply_text_muted_style(lv_obj_t *label);

#endif // GUI_STYLES_H