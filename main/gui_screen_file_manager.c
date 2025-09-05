#include "gui_screens.h"
#include "gui_events.h"
#include "gui_state.h"
#include "gui_styles.h"
#include "sd_manager.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "GUI_FILE_MGR";

lv_obj_t *file_manager_screen = NULL;
lv_obj_t *file_list = NULL;
lv_obj_t *current_path_label = NULL;
static lv_obj_t *mount_button_label = NULL;

void create_file_manager_screen(void) {
    file_manager_screen = lv_obj_create(NULL);
    lv_obj_add_style(file_manager_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create a centered container for the screen
    lv_obj_t *center_container = lv_obj_create(file_manager_screen);
    lv_obj_set_size(center_container, lv_pct(80), lv_pct(100));
    lv_obj_align(center_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(center_container, 10, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(center_container);
    lv_label_set_text(title, "File Manager");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Current path
    current_path_label = lv_label_create(center_container);
    lv_label_set_text(current_path_label, "/sdcard");
    lv_obj_set_style_text_color(current_path_label, THEME_SUCCESS_COLOR, 0);
    lv_obj_set_style_text_font(current_path_label, THEME_FONT_NORMAL, 0);
    lv_obj_align(current_path_label, LV_ALIGN_TOP_LEFT, 10, 60);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(center_container);
    lv_obj_set_size(back_btn, 100, 50);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 35);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Mount/Unmount button
    lv_obj_t *mount_btn = lv_button_create(center_container);
    lv_obj_set_size(mount_btn, 120, 50);
    lv_obj_align(mount_btn, LV_ALIGN_TOP_RIGHT, -250, 35);
    apply_button_style(mount_btn);
    lv_obj_add_event_cb(mount_btn, mount_unmount_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    mount_button_label = lv_label_create(mount_btn);
    if (sd_manager_is_mounted()) {
        lv_label_set_text(mount_button_label, LV_SYMBOL_EJECT " Unmount");
    } else {
        lv_label_set_text(mount_button_label, LV_SYMBOL_SD_CARD " Mount");
    }
    lv_obj_center(mount_button_label);
    
    // Format button
    lv_obj_t *format_btn = lv_button_create(center_container);
    lv_obj_set_size(format_btn, 120, 50);
    lv_obj_align(format_btn, LV_ALIGN_TOP_RIGHT, -120, 35);
    apply_button_style(format_btn);
    lv_obj_add_event_cb(format_btn, format_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *format_label = lv_label_create(format_btn);
    lv_label_set_text(format_label, LV_SYMBOL_REFRESH " Format");
    lv_obj_center(format_label);
    
    // Native LVGL list for files
    file_list = lv_list_create(center_container);
    lv_obj_set_size(file_list, lv_pct(95), lv_pct(70));
    lv_obj_align(file_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    apply_list_style(file_list);
}

void update_file_list(void) {
    // Clear existing items
    lv_obj_clean(file_list);
    
    // Update path label
    lv_label_set_text(current_path_label, current_directory);
    
    // Update mount button text based on current SD card state
    // Only update if the button label exists and is valid
    if (mount_button_label && lv_obj_is_valid(mount_button_label)) {
        if (sd_manager_is_mounted()) {
            lv_label_set_text(mount_button_label, LV_SYMBOL_EJECT " Unmount");
        } else {
            lv_label_set_text(mount_button_label, LV_SYMBOL_SD_CARD " Mount");
        }
    }
    
    if (!sd_manager_is_mounted()) {
        lv_obj_t *item = lv_list_add_button(file_list, LV_SYMBOL_WARNING, "SD Card not mounted");
        lv_obj_set_style_text_color(item, THEME_ERROR_COLOR, 0);
        return;
    }
    
    file_entry_t entries[32];
    int count = sd_manager_scan_directory(current_directory, entries, 32);
    
    if (count <= 0) {
        lv_obj_t *item = lv_list_add_button(file_list, LV_SYMBOL_WARNING, "No files found");
        lv_obj_set_style_text_color(item, THEME_WARNING_COLOR, 0);
        return;
    }
    
    for (int i = 0; i < count; i++) {
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
        
        lv_obj_t *item = lv_list_add_button(file_list, icon, truncated_name);
        apply_list_item_style(item);
        lv_obj_add_event_cb(item, file_list_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        
        if (entries[i].is_directory) {
            lv_obj_set_style_text_color(item, lv_color_hex(0x00ffff), 0);
        } else {
            lv_obj_set_style_text_color(item, THEME_TEXT_COLOR, 0);
        }
    }
}