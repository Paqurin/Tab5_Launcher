#include "gui_screens.h"
#include "gui_events.h"
#include "gui_state.h"
#include "gui_styles.h"
#include "gui_status_bar.h"
#include "sd_manager.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "GUI_FIRMWARE";

lv_obj_t *firmware_loader_screen = NULL;
lv_obj_t *firmware_list = NULL;
lv_obj_t *flash_btn = NULL;
lv_obj_t *status_label = NULL;
static gui_status_bar_t *status_bar = NULL;

void create_firmware_loader_screen(void) {
    firmware_loader_screen = lv_obj_create(NULL);
    lv_obj_add_style(firmware_loader_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create status bar at top
    status_bar = gui_status_bar_create(firmware_loader_screen);
    
    // Create a centered container for the screen (below status bar)
    lv_obj_t *center_container = lv_obj_create(firmware_loader_screen);
    lv_obj_set_size(center_container, lv_pct(80), lv_pct(85));
    lv_obj_align(center_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(center_container, 10, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(center_container);
    lv_label_set_text(title, "Firmware Loader");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(center_container);
    lv_obj_set_size(back_btn, 100, 50);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 35);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)2);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Native LVGL list for firmware files
    firmware_list = lv_list_create(center_container);
    lv_obj_set_size(firmware_list, lv_pct(95), lv_pct(60));
    lv_obj_align(firmware_list, LV_ALIGN_TOP_MID, 0, 90);
    apply_list_style(firmware_list);
    
    // Flash button
    flash_btn = lv_button_create(center_container);
    lv_obj_set_size(flash_btn, lv_pct(80), 60);
    lv_obj_align(flash_btn, LV_ALIGN_BOTTOM_MID, 0, -70);
    apply_button_style(flash_btn);
    lv_obj_add_flag(flash_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(flash_btn, flash_firmware_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *flash_label = lv_label_create(flash_btn);
    lv_label_set_text(flash_label, "Flash Firmware");
    lv_obj_center(flash_label);
    
    // Status label
    status_label = lv_label_create(center_container);
    lv_label_set_text(status_label, "Select a firmware file to flash");
    lv_obj_set_style_text_color(status_label, THEME_WARNING_COLOR, 0);
    lv_obj_set_style_text_font(status_label, THEME_FONT_NORMAL, 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void update_firmware_status_bar(float voltage, float current_ma, bool charging) {
    if (status_bar) {
        gui_status_bar_update_power(status_bar, voltage, current_ma, charging);
        gui_status_bar_update_sdcard(status_bar);
        // WiFi update would go here when WiFi manager is available
        // gui_status_bar_update_wifi(status_bar, wifi_connected, rssi);
    }
}

void update_firmware_list(void) {
    // Clear existing items
    lv_obj_clean(firmware_list);
    selected_firmware = -1;
    lv_obj_add_flag(flash_btn, LV_OBJ_FLAG_HIDDEN);
    
    if (!sd_manager_is_mounted()) {
        lv_obj_t *item = lv_list_add_button(firmware_list, LV_SYMBOL_WARNING, "SD Card not mounted");
        lv_obj_set_style_text_color(item, THEME_ERROR_COLOR, 0);
        lv_label_set_text(status_label, "SD Card not available");
        return;
    }
    
    firmware_count = firmware_loader_scan_firmware_files("/", firmware_files, 16);
    
    if (firmware_count <= 0) {
        lv_obj_t *item = lv_list_add_button(firmware_list, LV_SYMBOL_WARNING, "No firmware files found");
        lv_obj_set_style_text_color(item, THEME_WARNING_COLOR, 0);
        lv_label_set_text(status_label, "No .bin files found on SD card");
        return;
    }
    
    for (int i = 0; i < firmware_count; i++) {
        // Truncate filename if too long
        char truncated_name[200];
        if (strlen(firmware_files[i].filename) > 180) {
            strncpy(truncated_name, firmware_files[i].filename, 177);
            truncated_name[177] = '\0';
            strcat(truncated_name, "...");
        } else {
            strcpy(truncated_name, firmware_files[i].filename);
        }
        
        char item_text[256];
        size_t size_kb = firmware_files[i].size / 1024;
        if (size_kb > 9999) {
            snprintf(item_text, sizeof(item_text), "%s (>9MB)", truncated_name);
        } else {
            snprintf(item_text, sizeof(item_text), "%s (%zuKB)", truncated_name, size_kb);
        }
        
        lv_obj_t *item = lv_list_add_button(firmware_list, LV_SYMBOL_FILE, item_text);
        apply_list_item_style(item);
        lv_obj_add_event_cb(item, firmware_list_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
    }
    
    lv_label_set_text(status_label, "Select a firmware file to flash");
}