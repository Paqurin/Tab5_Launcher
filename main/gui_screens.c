#include "gui_screens.h"
#include "gui_events.h"
#include "gui_state.h"
#include "sd_manager.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "GUI_SCREENS";

// Screen objects
lv_obj_t *main_screen = NULL;
lv_obj_t *file_manager_screen = NULL;
lv_obj_t *firmware_loader_screen = NULL;
lv_obj_t *progress_screen = NULL;
lv_obj_t *splash_screen = NULL;

// UI element objects
lv_obj_t *file_list = NULL;
lv_obj_t *firmware_list = NULL;
lv_obj_t *current_path_label = NULL;
lv_obj_t *flash_btn = NULL;
lv_obj_t *status_label = NULL;
lv_obj_t *progress_bar = NULL;
lv_obj_t *progress_label = NULL;
lv_obj_t *progress_step_label = NULL;

void gui_screens_init(void) {
    create_main_screen();
    create_file_manager_screen();
    create_firmware_loader_screen();
    create_progress_screen();
    create_splash_screen();
}

void create_main_screen(void) {
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(main_screen);
    lv_label_set_text(title, "Simplified Launcher");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // SD Card status
    lv_obj_t *sd_status = lv_label_create(main_screen);
    if (sd_manager_is_mounted()) {
        lv_label_set_text(sd_status, LV_SYMBOL_SD_CARD " SD Card: Mounted");
        lv_obj_set_style_text_color(sd_status, lv_color_hex(0x00ff00), 0);
    } else {
        lv_label_set_text(sd_status, LV_SYMBOL_SD_CARD " SD Card: Not Found");
        lv_obj_set_style_text_color(sd_status, lv_color_hex(0xff0000), 0);
    }
    lv_obj_align(sd_status, LV_ALIGN_TOP_MID, 0, 60);
    
    // File Manager button
    lv_obj_t *file_mgr_btn = lv_button_create(main_screen);
    lv_obj_set_size(file_mgr_btn, 200, 60);
    lv_obj_align(file_mgr_btn, LV_ALIGN_CENTER, 0, -40);
    lv_obj_add_event_cb(file_mgr_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *file_mgr_label = lv_label_create(file_mgr_btn);
    lv_label_set_text(file_mgr_label, LV_SYMBOL_DIRECTORY " File Manager");
    lv_obj_center(file_mgr_label);
    
    // Firmware Loader button
    lv_obj_t *fw_loader_btn = lv_button_create(main_screen);
    lv_obj_set_size(fw_loader_btn, 200, 60);
    lv_obj_align(fw_loader_btn, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_event_cb(fw_loader_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *fw_loader_label = lv_label_create(fw_loader_btn);
    lv_label_set_text(fw_loader_label, LV_SYMBOL_DOWNLOAD " Firmware Loader");
    lv_obj_center(fw_loader_label);
}

void create_file_manager_screen(void) {
    file_manager_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(file_manager_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(file_manager_screen);
    lv_label_set_text(title, "File Manager");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Current path
    current_path_label = lv_label_create(file_manager_screen);
    lv_label_set_text(current_path_label, "/sdcard");
    lv_obj_set_style_text_color(current_path_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(current_path_label, LV_ALIGN_TOP_LEFT, 10, 40);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(file_manager_screen);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 35);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // File list container
    file_list = lv_obj_create(file_manager_screen);
    lv_obj_set_size(file_list, lv_pct(95), lv_pct(70));
    lv_obj_align(file_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(file_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(file_list, 5, 0);
}

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

void create_progress_screen(void) {
    progress_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(progress_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(progress_screen);
    lv_label_set_text(title, "Flashing Firmware");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Progress bar
    progress_bar = lv_bar_create(progress_screen);
    lv_obj_set_size(progress_bar, lv_pct(80), 30);
    lv_obj_align(progress_bar, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(progress_bar, 0, 100);
    
    // Progress label
    progress_label = lv_label_create(progress_screen);
    lv_label_set_text(progress_label, "0 / 0 bytes (0%)");
    lv_obj_set_style_text_color(progress_label, lv_color_white(), 0);
    lv_obj_align(progress_label, LV_ALIGN_CENTER, 0, 40);
    
    // Step description
    progress_step_label = lv_label_create(progress_screen);
    lv_label_set_text(progress_step_label, "Preparing...");
    lv_obj_set_style_text_color(progress_step_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(progress_step_label, LV_ALIGN_CENTER, 0, -40);
}

void create_splash_screen(void) {
    splash_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(splash_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(splash_screen);
    lv_label_set_text(title, "New Firmware Detected");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // Message
    lv_obj_t *msg = lv_label_create(splash_screen);
    lv_label_set_text(msg, "Choose an option:");
    lv_obj_set_style_text_color(msg, lv_color_hex(0x00ff00), 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 70);
    
    // Boot firmware button
    lv_obj_t *boot_btn = lv_button_create(splash_screen);
    lv_obj_set_size(boot_btn, 200, 60);
    lv_obj_align(boot_btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_add_event_cb(boot_btn, splash_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *boot_label = lv_label_create(boot_btn);
    lv_label_set_text(boot_label, "Boot New Firmware");
    lv_obj_center(boot_label);
    
    // Stay in launcher button
    lv_obj_t *stay_btn = lv_button_create(splash_screen);
    lv_obj_set_size(stay_btn, 200, 60);
    lv_obj_align(stay_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(stay_btn, splash_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *stay_label = lv_label_create(stay_btn);
    lv_label_set_text(stay_label, "Stay in Launcher");
    lv_obj_center(stay_label);
}

void update_file_list(void) {
    // Clear existing items
    lv_obj_clean(file_list);
    
    // Update path label
    lv_label_set_text(current_path_label, current_directory);
    
    if (!sd_manager_is_mounted()) {
        lv_obj_t *item = lv_button_create(file_list);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, LV_SYMBOL_WARNING " SD Card not mounted");
        lv_obj_set_style_text_color(label, lv_color_hex(0xff0000), 0);
        lv_obj_center(label);
        return;
    }
    
    file_entry_t entries[32];
    int count = sd_manager_scan_directory(current_directory, entries, 32);
    
    if (count <= 0) {
        lv_obj_t *item = lv_button_create(file_list);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, LV_SYMBOL_WARNING " No files found");
        lv_obj_set_style_text_color(label, lv_color_hex(0xffff00), 0);
        lv_obj_center(label);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        lv_obj_t *item = lv_button_create(file_list);
        lv_obj_set_width(item, lv_pct(100));
        lv_obj_add_event_cb(item, file_list_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        
        lv_obj_t *label = lv_label_create(item);
        char item_text[256];
        const char *icon = entries[i].is_directory ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
        
        // Truncate filename if too long to prevent buffer overflow
        char truncated_name[200];
        if (strlen(entries[i].name) > 190) {
            strncpy(truncated_name, entries[i].name, 187);
            truncated_name[187] = '\0';
            strcat(truncated_name, "...");
        } else {
            strcpy(truncated_name, entries[i].name);
        }
        
        snprintf(item_text, sizeof(item_text), "%s %s", icon, truncated_name);
        lv_label_set_text(label, item_text);
        
        if (entries[i].is_directory) {
            lv_obj_set_style_text_color(label, lv_color_hex(0x00ffff), 0);
        } else {
            lv_obj_set_style_text_color(label, lv_color_white(), 0);
        }
        lv_obj_center(label);
    }
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
    
    firmware_count = firmware_loader_scan_firmware_files("/sdcard", firmware_files, 16);
    
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
        
        // Truncate filename if too long to prevent buffer overflow
        char truncated_name[200];
        if (strlen(firmware_files[i].filename) > 180) {
            strncpy(truncated_name, firmware_files[i].filename, 177);
            truncated_name[177] = '\0';
            strcat(truncated_name, "...");
        } else {
            strcpy(truncated_name, firmware_files[i].filename);
        }
        
        // Limit size display to prevent overflow
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