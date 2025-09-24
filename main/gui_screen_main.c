#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "gui_pulldown_menu.h"
#include "gui_screen_tools.h"
#include "gui_screen_wifi_controls.h"
#include "sd_manager.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GUI_MAIN";

lv_obj_t *main_screen = NULL;
// Pulldown menu is now handled by global status bar

// Status bar components are now handled by global status bar

void create_main_screen(void) {
    main_screen = lv_obj_create(NULL);
    lv_obj_add_style(main_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Status bar is now global - no individual creation needed

    // WiFi button in top left corner
    lv_obj_t *wifi_btn = lv_button_create(main_screen);
    lv_obj_set_size(wifi_btn, 80, 40);  // 80px x 40px rectangular
    lv_obj_align(wifi_btn, LV_ALIGN_TOP_LEFT, 5, 45);  // Top left, below status bar
    apply_button_style(wifi_btn);
    lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0x3498db), 0);  // Blue color for WiFi
    lv_obj_add_event_cb(wifi_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)10);

    lv_obj_t *wifi_label = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI " WiFi");
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_12, 0);  // Smaller font to fit
    lv_obj_center(wifi_label);

    // Switches button in top right corner
    lv_obj_t *switches_btn = lv_button_create(main_screen);
    lv_obj_set_size(switches_btn, 80, 40);  // 80px x 40px rectangular
    lv_obj_align(switches_btn, LV_ALIGN_TOP_RIGHT, -5, 45);  // Top right, below status bar
    apply_button_style(switches_btn);
    lv_obj_set_style_bg_color(switches_btn, lv_color_hex(0x8e44ad), 0);  // Purple color
    lv_obj_add_event_cb(switches_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)9);

    lv_obj_t *switches_label = lv_label_create(switches_btn);
    lv_label_set_text(switches_label, "Switches");
    lv_obj_set_style_text_font(switches_label, &lv_font_montserrat_12, 0);  // Smaller font to fit
    lv_obj_center(switches_label);

    // Create centered container for main controls
    lv_obj_t *center_container = lv_obj_create(main_screen);
    lv_obj_set_size(center_container, lv_pct(80), lv_pct(85));
    lv_obj_align(center_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(center_container, 10, 0);
    
    
    // File Manager button
    lv_obj_t *file_mgr_btn = lv_button_create(center_container);
    lv_obj_set_size(file_mgr_btn, lv_pct(95), 70);
    lv_obj_align(file_mgr_btn, LV_ALIGN_CENTER, 0, -140);
    apply_button_style(file_mgr_btn);
    lv_obj_add_event_cb(file_mgr_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);

    lv_obj_t *file_mgr_label = lv_label_create(file_mgr_btn);
    lv_label_set_text(file_mgr_label, LV_SYMBOL_DIRECTORY " File Manager");
    lv_obj_center(file_mgr_label);

    // Tools button - NEW with softer color
    lv_obj_t *tools_btn = lv_button_create(center_container);
    lv_obj_set_size(tools_btn, lv_pct(95), 70);
    lv_obj_align(tools_btn, LV_ALIGN_CENTER, 0, -65);
    apply_button_style(tools_btn);
    lv_obj_set_style_bg_color(tools_btn, lv_color_hex(0x9b59b6), 0);  // Softer purple
    lv_obj_add_event_cb(tools_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)5);

    lv_obj_t *tools_label = lv_label_create(tools_btn);
    lv_label_set_text(tools_label, LV_SYMBOL_SETTINGS " Tools");
    lv_obj_center(tools_label);

    // Firmware Loader button
    lv_obj_t *fw_loader_btn = lv_button_create(center_container);
    lv_obj_set_size(fw_loader_btn, lv_pct(95), 70);
    lv_obj_align(fw_loader_btn, LV_ALIGN_CENTER, 0, 10);
    apply_button_style(fw_loader_btn);
    lv_obj_add_event_cb(fw_loader_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);

    lv_obj_t *fw_loader_label = lv_label_create(fw_loader_btn);
    lv_label_set_text(fw_loader_label, LV_SYMBOL_DOWNLOAD " Firmware Loader");
    lv_obj_center(fw_loader_label);
    
    // Run Firmware button
    lv_obj_t *run_fw_btn = lv_button_create(center_container);
    lv_obj_set_size(run_fw_btn, lv_pct(95), 70);
    lv_obj_align(run_fw_btn, LV_ALIGN_CENTER, 0, 85);
    apply_button_style(run_fw_btn);
    lv_obj_add_event_cb(run_fw_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)2);

    lv_obj_t *run_fw_label = lv_label_create(run_fw_btn);

    // Check if firmware is available
    if (firmware_loader_is_firmware_ready()) {
        lv_label_set_text(run_fw_label, LV_SYMBOL_PLAY " Run Firmware");
        lv_obj_set_style_text_color(run_fw_label, THEME_SUCCESS_COLOR, 0);
    } else {
        lv_label_set_text(run_fw_label, LV_SYMBOL_CLOSE " No Firmware");
        lv_obj_set_style_text_color(run_fw_label, THEME_ERROR_COLOR, 0);
        lv_obj_add_state(run_fw_btn, LV_STATE_DISABLED);
    }
    lv_obj_center(run_fw_label);

    // 4 Eject buttons in a row - SIMPLIFIED LAYOUT
    lv_obj_t *eject_container = lv_obj_create(center_container);
    lv_obj_set_size(eject_container, lv_pct(95), 70);
    lv_obj_align(eject_container, LV_ALIGN_CENTER, 0, 160);
    lv_obj_set_style_bg_opa(eject_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(eject_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(eject_container, 5, 0);
    lv_obj_set_flex_flow(eject_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(eject_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Eject button data - SOFTER COLORS
    struct {
        const char *symbol;
        const char *text;
        lv_color_t color;
        int event_id;
    } eject_buttons[] = {
        {LV_SYMBOL_EJECT, "Eject", lv_color_hex(0xe74c3c), 4},     // Softer red
        {LV_SYMBOL_HOME, "Factory", lv_color_hex(0x3498db), 6},   // Softer blue
        {LV_SYMBOL_SD_CARD, "To SD", lv_color_hex(0x2ecc71), 7},  // Softer green
        {LV_SYMBOL_TRASH, "Clean", lv_color_hex(0xf39c12), 8}     // Softer orange
    };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *eject_btn = lv_button_create(eject_container);
        lv_obj_set_size(eject_btn, 85, 60);
        apply_button_style(eject_btn);
        lv_obj_set_style_bg_color(eject_btn, eject_buttons[i].color, 0);

        // Simple label with symbol and text
        lv_obj_t *btn_label = lv_label_create(eject_btn);
        char label_text[32];
        snprintf(label_text, sizeof(label_text), "%s\n%s", eject_buttons[i].symbol, eject_buttons[i].text);
        lv_label_set_text(btn_label, label_text);
        lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(btn_label);

        // Disable factory button (index 1) completely
        if (i == 1) { // Factory button - always disabled
            lv_obj_add_state(eject_btn, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(eject_btn, lv_color_hex(0x555555), LV_STATE_DISABLED);
            lv_obj_set_style_text_color(btn_label, lv_color_hex(0x999999), LV_STATE_DISABLED);
            // Don't add event callback for factory button
        } else {
            // Add event callback for non-factory buttons
            lv_obj_add_event_cb(eject_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)eject_buttons[i].event_id);

            // Disable if no firmware available for other applicable buttons
            if (i < 3 && !firmware_loader_is_firmware_ready()) { // Eject, To SD need firmware
                lv_obj_add_state(eject_btn, LV_STATE_DISABLED);
                lv_obj_set_style_bg_color(eject_btn, lv_color_hex(0x555555), LV_STATE_DISABLED);
                lv_obj_set_style_text_color(btn_label, lv_color_hex(0x999999), LV_STATE_DISABLED);
            }
        }
    }
    
    // Settings button
    lv_obj_t *settings_btn = lv_button_create(center_container);
    lv_obj_set_size(settings_btn, lv_pct(95), 70);
    lv_obj_align(settings_btn, LV_ALIGN_CENTER, 0, 235);
    apply_button_style(settings_btn);
    lv_obj_add_event_cb(settings_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)3);

    lv_obj_t *settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_center(settings_label);
    
    // Pulldown menu is now created and managed by global status bar
}

// Status bar click handler is now handled by global status bar

void update_status_bar(float voltage, float current_ma, bool charging) {
    // Status bar updates now handled by global status bar in main loop
    // This function kept for compatibility but no longer needed
}

void update_main_screen(void) {
    if (main_screen) {
        lv_obj_clean(main_screen);
        create_main_screen();
    }
}

void destroy_main_screen(void) {
    if (main_screen) {
        ESP_LOGI(TAG, "Destroying main screen...");

        // Disable events on screen to prevent corruption during deletion
        lv_obj_remove_event_cb(main_screen, NULL);

        // Remove all event callbacks from child objects to prevent dangling pointers
        uint32_t child_cnt = lv_obj_get_child_count(main_screen);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(main_screen, i);
            if (child) {
                lv_obj_remove_event_cb(child, NULL);
            }
        }

        // Now safely delete the screen
        lv_obj_del(main_screen);
        main_screen = NULL;
        ESP_LOGI(TAG, "Main screen destroyed");
    }
}