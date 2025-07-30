#include "gui_screens.h"
#include "gui_events.h"
#include "gui_state.h"
#include "sd_manager.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "GUI_FIRMWARE";

lv_obj_t *firmware_loader_screen = NULL;
lv_obj_t *firmware_list = NULL;
lv_obj_t *flash_btn = NULL;
lv_obj_t *status_label = NULL;

void create_firmware_loader_screen(void) {
    firmware_loader_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(firmware_loader_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(firmware_loader_screen);
    lv_label_set_text(title, "Firmware Loader");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(firmware_loader_screen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 35);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)2);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Firmware list container
    firmware_list = lv_obj_create(firmware_loader_screen);
    lv_obj_set_size(firmware_list, lv_pct(95), lv_pct(60));
    lv_obj_align(firmware_list, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_flex_flow(firmware_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(firmware_list, 5, 0);
    
    // Flash button
    flash_btn = lv_button_create(firmware_loader_screen);
    lv_obj_set_size(flash_btn, 150, 50);
    lv_obj_align(flash_btn, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_add_flag(flash_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(flash_btn, flash_firmware_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *flash_label = lv_label_create(flash_btn);
    lv_label_set_text(flash_label, "Flash Firmware");
    lv_obj_center(flash_label);
    
    // Status label
    status_label = lv_label_create(firmware_loader_screen);
    lv_label_set_text(status_label, "Select a firmware file to flash");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xffff00), 0);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void update_firmware_list(void) {
    // Clear existing items
    lv_obj_clean(firmware_list);
    selected_firmware = -1;
    lv_obj_add_flag(flash_btn, LV_OBJ_FLAG_HIDDEN);
    
    if (!sd_manager_is_mounted()) {
        lv_obj_t *item = lv_button_create(firmware_list);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, LV_SYMBOL_WARNING " SD Card not mounted");
        lv_obj_set_style_text_color(label, lv_color_hex(0xff0000), 0);
        lv_obj_center(label);
        lv_label_set_text(status_label, "SD Card not available");
        return;
    }
    
    firmware_count = firmware_loader_scan_firmware_files("/", firmware_files, 16);
    
    if (firmware_count <= 0) {
        lv_obj_t *item = lv_button_create(firmware_list);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, LV_SYMBOL_WARNING " No firmware files found");
        lv_obj_set_style_text_color(label, lv_color_hex(0xffff00), 0);
        lv_obj_center(label);
        lv_label_set_text(status_label, "No .bin files found on SD card");
        return;
    }
    
    for (int i = 0; i < firmware_count; i++) {
        lv_obj_t *item = lv_button_create(firmware_list);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_add_event_cb(item, firmware_list_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        
        lv_obj_t *label = lv_label_create(item);
        char item_text[256];
        
        // Truncate filename if too long
        char truncated_name[200];
        if (strlen(firmware_files[i].filename) > 180) {
            strncpy(truncated_name, firmware_files[i].filename, 177);
            truncated_name[177] = '\0';
            strcat(truncated_name, "...");
        } else {
            strcpy(truncated_name, firmware_files[i].filename);
        }
        
        size_t size_kb = firmware_files[i].size / 1024;
        if (size_kb > 9999) {
            snprintf(item_text, sizeof(item_text), "%s %s (>9MB)", 
                    LV_SYMBOL_FILE, truncated_name);
        } else {
            snprintf(item_text, sizeof(item_text), "%s %s (%zuKB)", 
                    LV_SYMBOL_FILE, truncated_name, size_kb);
        }
        
        lv_label_set_text(label, item_text);
        lv_obj_set_style_text_color(label, lv_color_hex(0x00ff00), 0);
        lv_obj_center(label);
    }
    
    lv_label_set_text(status_label, "Select a firmware file to flash");
}