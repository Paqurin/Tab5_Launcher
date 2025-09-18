#include "gui_screen_tools.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "gui_screen_text_editor.h"
#include "gui_screen_python_launcher.h"
#include "gui_screen_calculator.h"
#include "esp_log.h"

static const char *TAG = "GUI_TOOLS";

lv_obj_t *tools_screen = NULL;

static void tools_menu_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        int tool_id = (int)(uintptr_t)lv_event_get_user_data(e);

        switch (tool_id) {
            case 0: // Text Editor
                ESP_LOGI(TAG, "Text Editor selected");
                create_text_editor_screen();
                lv_screen_load(text_editor_screen);
                break;

            case 1: // Calculator
                ESP_LOGI(TAG, "Calculator selected");
                show_calculator_screen();
                break;

            case 2: // Python Launcher
                ESP_LOGI(TAG, "Python Launcher selected");
                show_python_launcher_screen();
                break;

            case 3: // System Info
                ESP_LOGI(TAG, "System Info selected");
                lv_obj_t *info_mbox = lv_msgbox_create(lv_screen_active());
                lv_obj_t *info_text = lv_label_create(info_mbox);
                lv_label_set_text(info_text, "System Info\n\n"
                    "ESP32-P4 Tab5 Launcher\n"
                    "Version: v0.11\n"
                    "ESP-IDF: 5.4.1\n"
                    "LVGL: 9.3.0");
                lv_obj_center(info_text);
                lv_obj_set_size(info_mbox, 300, 180);
                lv_obj_center(info_mbox);
                break;
        }
    }
}

static void tools_back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        tools_screen_back();
    }
}

void create_tools_screen(void) {
    if (tools_screen) {
        return; // Already created
    }

    tools_screen = lv_obj_create(NULL);
    lv_obj_add_style(tools_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(tools_screen);
    lv_obj_set_size(title_bar, lv_pct(100), 60);
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_opa(title_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(title_bar, 10, 0);

    // Back button
    lv_obj_t *back_btn = lv_button_create(title_bar);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, tools_back_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    // Title
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Tools");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // Create main container for tools grid
    lv_obj_t *tools_container = lv_obj_create(tools_screen);
    lv_obj_set_size(tools_container, lv_pct(90), lv_pct(80));
    lv_obj_align(tools_container, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_opa(tools_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(tools_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(tools_container, 20, 0);
    lv_obj_set_flex_flow(tools_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(tools_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Tool button data
    struct {
        const char *symbol;
        const char *text;
        lv_color_t color;
        int tool_id;
    } tools[] = {
        {LV_SYMBOL_EDIT, "Text\nEditor", lv_color_hex(0x4ecdc4), 0},
        {LV_SYMBOL_KEYBOARD, "Calculator", lv_color_hex(0x44a08d), 1},
        {LV_SYMBOL_FILE, "Python\nLauncher", lv_color_hex(0x3d5a80), 2},
        {LV_SYMBOL_SETTINGS, "System\nInfo", lv_color_hex(0x6c5ce7), 3}
    };
    // Create tool buttons in 2x2 grid
    for (int i = 0; i < 4; i++) {
        lv_obj_t *tool_btn = lv_button_create(tools_container);
        lv_obj_set_size(tool_btn, 140, 100);
        apply_button_style(tool_btn);
        lv_obj_set_style_bg_color(tool_btn, tools[i].color, 0);
        lv_obj_add_event_cb(tool_btn, tools_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)tools[i].tool_id);

        // Create button content container (non-clickable)
        lv_obj_t *btn_content = lv_obj_create(tool_btn);
        lv_obj_set_size(btn_content, lv_pct(100), lv_pct(100));
        lv_obj_center(btn_content);
        lv_obj_set_style_bg_opa(btn_content, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(btn_content, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(btn_content, 5, 0);
        lv_obj_set_flex_flow(btn_content, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        // Make content container non-clickable so button clicks work
        lv_obj_remove_flag(btn_content, LV_OBJ_FLAG_CLICKABLE);

        // Symbol
        lv_obj_t *symbol_label = lv_label_create(btn_content);
        lv_label_set_text(symbol_label, tools[i].symbol);
        lv_obj_set_style_text_color(symbol_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(symbol_label, &lv_font_montserrat_24, 0);

        // Text
        lv_obj_t *text_label = lv_label_create(btn_content);
        lv_label_set_text(text_label, tools[i].text);
        lv_obj_set_style_text_color(text_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(text_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
    }
}

void show_tools_screen(void) {
    if (!tools_screen) {
        create_tools_screen();
    }
    lv_screen_load(tools_screen);
}

void tools_screen_back(void) {
    ESP_LOGI(TAG, "Returning to main screen");
    lv_screen_load(main_screen);
}