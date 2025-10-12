#include "gui_styles.h"

// Style objects
lv_style_t style_screen;
lv_style_t style_button;
lv_style_t style_button_pressed;
lv_style_t style_list;
lv_style_t style_list_item;
lv_style_t style_list_item_selected;
lv_style_t style_title;
lv_style_t style_text;
lv_style_t style_text_muted;

void gui_styles_init(void) {
    // Screen style
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, THEME_BG_COLOR);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);

    // Button style - Optimized wireframe appearance (no rounded corners, minimal styling)
    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, lv_color_hex(0x2E2E2E));
    lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
    lv_style_set_border_color(&style_button, lv_color_hex(0x4A90E2));
    lv_style_set_border_width(&style_button, 2);
    lv_style_set_border_opa(&style_button, LV_OPA_COVER);
    lv_style_set_radius(&style_button, 0);  // No rounded corners for performance
    lv_style_set_text_color(&style_button, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_button, &lv_font_montserrat_18);  // Smaller font for performance
    lv_style_set_pad_all(&style_button, 12);
    lv_style_set_shadow_width(&style_button, 0);

    // Button pressed style - Simple color change only for performance
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, lv_color_hex(0x4A90E2));
    lv_style_set_bg_opa(&style_button_pressed, LV_OPA_COVER);  // Solid color, no transparency
    // No border width change - just keep same border with different color
    lv_style_set_border_color(&style_button_pressed, lv_color_hex(0x6BB3FF));
    lv_style_set_shadow_width(&style_button_pressed, 0);

    // List style - Optimized wireframe appearance
    lv_style_init(&style_list);
    lv_style_set_bg_color(&style_list, lv_color_hex(0x1E1E1E));
    lv_style_set_bg_opa(&style_list, LV_OPA_COVER);
    lv_style_set_border_color(&style_list, lv_color_hex(0x4A90E2));
    lv_style_set_border_width(&style_list, 1);
    lv_style_set_border_opa(&style_list, LV_OPA_COVER);
    lv_style_set_radius(&style_list, 0);  // No rounded corners for performance
    lv_style_set_pad_all(&style_list, 6);
    lv_style_set_shadow_width(&style_list, 0);

    // List item style - Optimized wireframe appearance
    lv_style_init(&style_list_item);
    lv_style_set_bg_color(&style_list_item, lv_color_hex(0x2E2E2E));
    lv_style_set_bg_opa(&style_list_item, LV_OPA_COVER);
    lv_style_set_border_color(&style_list_item, lv_color_hex(0x404040));
    lv_style_set_border_width(&style_list_item, 1);
    lv_style_set_border_opa(&style_list_item, LV_OPA_COVER);  // No transparency for performance
    lv_style_set_radius(&style_list_item, 0);  // No rounded corners for performance
    lv_style_set_text_color(&style_list_item, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_list_item, &lv_font_montserrat_18);  // Smaller font for performance
    lv_style_set_pad_all(&style_list_item, 10);
    lv_style_set_margin_bottom(&style_list_item, 2);

    // List item selected style - Simple color change for performance
    lv_style_init(&style_list_item_selected);
    lv_style_set_bg_color(&style_list_item_selected, lv_color_hex(0x4A90E2));
    lv_style_set_bg_opa(&style_list_item_selected, LV_OPA_COVER);  // Solid color, no transparency
    lv_style_set_border_color(&style_list_item_selected, lv_color_hex(0x6BB3FF));
    // No border width change for performance

    // Title style
    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, THEME_TEXT_COLOR);
    lv_style_set_text_font(&style_title, THEME_FONT_LARGE);
    lv_style_set_text_align(&style_title, LV_TEXT_ALIGN_CENTER);

    // Normal text style
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, THEME_TEXT_COLOR);
    lv_style_set_text_font(&style_text, THEME_FONT_NORMAL);

    // Muted text style
    lv_style_init(&style_text_muted);
    lv_style_set_text_color(&style_text_muted, THEME_TEXT_MUTED);
    lv_style_set_text_font(&style_text_muted, THEME_FONT_NORMAL);
}

void apply_button_style(lv_obj_t *obj) {
    lv_obj_add_style(obj, &style_button, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_button_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
}

void apply_list_style(lv_obj_t *obj) {
    lv_obj_add_style(obj, &style_list, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void apply_list_item_style(lv_obj_t *obj) {
    lv_obj_add_style(obj, &style_list_item, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_style(obj, &style_list_item_selected, LV_PART_MAIN | LV_STATE_PRESSED);
}

void apply_title_style(lv_obj_t *label) {
    lv_obj_add_style(label, &style_title, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void apply_text_style(lv_obj_t *label) {
    lv_obj_add_style(label, &style_text, LV_PART_MAIN | LV_STATE_DEFAULT);
}

void apply_text_muted_style(lv_obj_t *label) {
    lv_obj_add_style(label, &style_text_muted, LV_PART_MAIN | LV_STATE_DEFAULT);
}