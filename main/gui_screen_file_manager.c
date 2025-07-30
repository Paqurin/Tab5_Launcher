#include "gui_screens.h"
#include "gui_events.h"
#include "gui_state.h"
#include "sd_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "GUI_FILE_MGR";

lv_obj_t *file_manager_screen = NULL;
lv_obj_t *file_list = NULL;
lv_obj_t *current_path_label = NULL;

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
        
        // Truncate filename if too long
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