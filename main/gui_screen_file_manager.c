#include "gui_screens.h"
#include "gui_events.h"
#include "gui_state.h"
#include "gui_styles.h"
#include "gui_status_bar.h"
#include "sd_manager.h"
#include "file_operations.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "GUI_FILE_MGR";

lv_obj_t *file_manager_screen = NULL;
lv_obj_t *file_list = NULL;
lv_obj_t *current_path_label = NULL;
static lv_obj_t *mount_button_label = NULL;
static gui_status_bar_t *status_bar = NULL;

// Toolbar button references
static lv_obj_t *select_btn = NULL;
static lv_obj_t *delete_btn = NULL;
static lv_obj_t *copy_btn = NULL;
static lv_obj_t *move_btn = NULL;
static lv_obj_t *rename_btn = NULL;
static lv_obj_t *paste_btn = NULL;

void create_file_manager_screen(void) {
    file_manager_screen = lv_obj_create(NULL);
    lv_obj_add_style(file_manager_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create status bar at top
    status_bar = gui_status_bar_create(file_manager_screen);
    
    // Create a centered container for the screen (below status bar)
    lv_obj_t *center_container = lv_obj_create(file_manager_screen);
    lv_obj_set_size(center_container, lv_pct(80), lv_pct(85));
    lv_obj_align(center_container, LV_ALIGN_CENTER, 0, 20);
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
    
    // Up directory button
    lv_obj_t *up_btn = lv_button_create(center_container);
    lv_obj_set_size(up_btn, 80, 40);
    lv_obj_align(up_btn, LV_ALIGN_TOP_LEFT, 350, 55);
    apply_button_style(up_btn);
    lv_obj_add_event_cb(up_btn, directory_up_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *up_label = lv_label_create(up_btn);
    lv_label_set_text(up_label, LV_SYMBOL_UP " Up");
    lv_obj_center(up_label);
    
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
    
    // Simple file operations toolbar
    lv_obj_t *toolbar = lv_obj_create(center_container);
    lv_obj_set_size(toolbar, lv_pct(95), 50);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_color(toolbar, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(toolbar, 1, 0);
    lv_obj_set_style_border_color(toolbar, THEME_PRIMARY_COLOR, 0);
    lv_obj_set_style_radius(toolbar, 5, 0);
    lv_obj_set_flex_flow(toolbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(toolbar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Select button (toggle selection mode)
    select_btn = lv_button_create(toolbar);
    lv_obj_set_size(select_btn, 80, 35);
    apply_button_style(select_btn);
    lv_obj_t *select_btn_label = lv_label_create(select_btn);
    lv_label_set_text(select_btn_label, LV_SYMBOL_OK " Select");
    lv_obj_center(select_btn_label);
    lv_obj_add_event_cb(select_btn, toggle_selection_mode_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Create file button
    lv_obj_t *new_file_btn = lv_button_create(toolbar);
    lv_obj_set_size(new_file_btn, 80, 35);
    apply_button_style(new_file_btn);
    lv_obj_t *file_btn_label = lv_label_create(new_file_btn);
    lv_label_set_text(file_btn_label, LV_SYMBOL_FILE " New");
    lv_obj_center(file_btn_label);
    
    // Create folder button
    lv_obj_t *new_folder_btn = lv_button_create(toolbar);
    lv_obj_set_size(new_folder_btn, 80, 35);
    apply_button_style(new_folder_btn);
    lv_obj_t *folder_btn_label = lv_label_create(new_folder_btn);
    lv_label_set_text(folder_btn_label, LV_SYMBOL_DIRECTORY " New");
    lv_obj_center(folder_btn_label);
    
    // Delete button
    delete_btn = lv_button_create(toolbar);
    lv_obj_set_size(delete_btn, 80, 35);
    apply_button_style(delete_btn);
    lv_obj_t *delete_btn_label = lv_label_create(delete_btn);
    lv_label_set_text(delete_btn_label, LV_SYMBOL_TRASH " Del");
    lv_obj_center(delete_btn_label);
    lv_obj_add_event_cb(delete_btn, delete_files_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Copy button
    copy_btn = lv_button_create(toolbar);
    lv_obj_set_size(copy_btn, 80, 35);
    apply_button_style(copy_btn);
    lv_obj_t *copy_btn_label = lv_label_create(copy_btn);
    lv_label_set_text(copy_btn_label, LV_SYMBOL_COPY " Copy");
    lv_obj_center(copy_btn_label);
    lv_obj_add_event_cb(copy_btn, copy_files_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Move button
    move_btn = lv_button_create(toolbar);
    lv_obj_set_size(move_btn, 80, 35);
    apply_button_style(move_btn);
    lv_obj_t *move_btn_label = lv_label_create(move_btn);
    lv_label_set_text(move_btn_label, LV_SYMBOL_EDIT " Move");
    lv_obj_center(move_btn_label);
    lv_obj_add_event_cb(move_btn, move_files_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Rename button
    rename_btn = lv_button_create(toolbar);
    lv_obj_set_size(rename_btn, 80, 35);
    apply_button_style(rename_btn);
    lv_obj_t *rename_btn_label = lv_label_create(rename_btn);
    lv_label_set_text(rename_btn_label, LV_SYMBOL_SETTINGS " Ren");
    lv_obj_center(rename_btn_label);
    lv_obj_add_event_cb(rename_btn, rename_file_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Paste button
    paste_btn = lv_button_create(toolbar);
    lv_obj_set_size(paste_btn, 80, 35);
    apply_button_style(paste_btn);
    lv_obj_t *paste_btn_label = lv_label_create(paste_btn);
    lv_label_set_text(paste_btn_label, LV_SYMBOL_PASTE " Paste");
    lv_obj_center(paste_btn_label);
    lv_obj_add_event_cb(paste_btn, paste_files_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Native LVGL list for files (adjusted position)
    file_list = lv_list_create(center_container);
    lv_obj_set_size(file_list, lv_pct(95), lv_pct(60));
    lv_obj_align(file_list, LV_ALIGN_BOTTOM_MID, 0, -10);
    apply_list_style(file_list);
    
    // Initialize toolbar button states (disabled by default)
    update_toolbar_button_states();
}

void update_file_manager_status_bar(float voltage, float current_ma, bool charging) {
    if (status_bar) {
        gui_status_bar_update_power(status_bar, voltage, current_ma, charging);
        gui_status_bar_update_sdcard(status_bar);
        // WiFi update would go here when WiFi manager is available
        // gui_status_bar_update_wifi(status_bar, wifi_connected, rssi);
    }
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
    int count = sd_manager_scan_directory(current_directory, entries, 32, true); // Show hidden files by default
    
    // Update global current_entries for navigation
    current_entry_count = count;
    if (count > 0) {
        memcpy(current_entries, entries, count * sizeof(file_entry_t));
    }
    
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
        
        // Create the list item
        lv_obj_t *item = lv_list_add_button(file_list, icon, truncated_name);
        apply_list_item_style(item);
        
        // If selection mode is enabled, add a checkbox
        if (file_selection_enabled) {
            lv_obj_t *checkbox = lv_checkbox_create(item);
            lv_checkbox_set_text(checkbox, "");
            lv_obj_align(checkbox, LV_ALIGN_RIGHT_MID, -10, 0);
            lv_obj_set_size(checkbox, 30, 30);
            
            // Set checkbox state based on selection
            if (selected_files[i]) {
                lv_obj_add_state(checkbox, LV_STATE_CHECKED);
            }
            
            // Add event handler for checkbox toggle
            lv_obj_add_event_cb(checkbox, file_selection_event_handler, LV_EVENT_VALUE_CHANGED, (void*)(uintptr_t)i);
        }
        
        lv_obj_add_event_cb(item, file_list_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)i);
        
        // Highlight selected items
        if (file_selection_enabled && selected_files[i]) {
            lv_obj_set_style_bg_color(item, lv_color_hex(0x333333), 0);
            lv_obj_set_style_bg_opa(item, LV_OPA_50, 0);
        }
        
        if (entries[i].is_directory) {
            lv_obj_set_style_text_color(item, lv_color_hex(0x00ffff), 0);
        } else {
            lv_obj_set_style_text_color(item, THEME_TEXT_COLOR, 0);
        }
    }
    
    // Update toolbar button states after refreshing file list
    update_toolbar_button_states();
}

void update_toolbar_button_states(void) {
    // Update button states based on selection
    if (rename_btn) {
        if (selected_file_count > 1) {
            // Disable rename button when multiple files are selected
            lv_obj_add_state(rename_btn, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(rename_btn, lv_color_hex(0x666666), LV_STATE_DISABLED);
        } else {
            // Enable rename button when 0 or 1 file is selected
            lv_obj_remove_state(rename_btn, LV_STATE_DISABLED);
        }
    }
    
    // Enable/disable other operation buttons based on selection
    bool has_selection = selected_file_count > 0;
    
    if (delete_btn) {
        if (has_selection) {
            lv_obj_remove_state(delete_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(delete_btn, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(delete_btn, lv_color_hex(0x666666), LV_STATE_DISABLED);
        }
    }
    
    if (copy_btn) {
        if (has_selection) {
            lv_obj_remove_state(copy_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(copy_btn, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(copy_btn, lv_color_hex(0x666666), LV_STATE_DISABLED);
        }
    }
    
    if (move_btn) {
        if (has_selection) {
            lv_obj_remove_state(move_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(move_btn, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(move_btn, lv_color_hex(0x666666), LV_STATE_DISABLED);
        }
    }
    
    // Enable/disable paste button based on clipboard content
    if (paste_btn) {
        if (clipboard_has_content) {
            lv_obj_remove_state(paste_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(paste_btn, LV_STATE_DISABLED);
            lv_obj_set_style_bg_color(paste_btn, lv_color_hex(0x666666), LV_STATE_DISABLED);
        }
    }
}

void update_file_manager_screen(void) {
    if (file_manager_screen) {
        lv_obj_clean(file_manager_screen);
        create_file_manager_screen();
    }
}

void destroy_file_manager_screen(void) {
    if (file_manager_screen) {
        lv_obj_del(file_manager_screen);
        file_manager_screen = NULL;
        ESP_LOGI(TAG, "File manager screen destroyed");
    }
}