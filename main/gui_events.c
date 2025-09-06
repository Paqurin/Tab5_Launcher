#include "gui_events.h"
#include "gui_screens.h"
#include "gui_progress.h"
#include "gui_state.h"
#include "gui_file_browser_v2.h"
#include "gui_screen_settings.h"
#include "firmware_loader.h"
#include "sd_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "GUI_EVENTS";

// Format functionality temporarily removed

void main_menu_event_handler(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uint32_t menu_id = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
        
        switch (menu_id) {
            case 0: // File Manager
                strcpy(current_directory, "/");  // Changed from "/sdcard" to "/"
                update_file_list();
                lv_screen_load(file_manager_screen);
                break;
            case 1: // Firmware Loader
                update_firmware_list();
                lv_screen_load(firmware_loader_screen);
                break;
            case 2: // Run Firmware
                if (firmware_loader_is_firmware_ready()) {
                    // Show splash screen for user choice
                    lv_screen_load(splash_screen);
                } else {
                    ESP_LOGW(TAG, "No firmware available to run");
                }
                break;
            case 3: // Settings
                create_settings_screen();
                lv_screen_load(get_settings_screen());
                break;
        }
    }
}

void file_list_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int index = (int)(uintptr_t)lv_event_get_user_data(e);
        
        if (index >= 0 && index < current_entry_count) {
            if (current_entries[index].is_directory) {
                // Navigate to directory
                char temp_path[512];
                size_t dir_len = strlen(current_directory);
                size_t name_len = strlen(current_entries[index].name);
                
                // Check if the combined path would fit
                if (dir_len + name_len + 2 < sizeof(temp_path)) {
                    if (strcmp(current_directory, "/") == 0) {  // Changed from "/sdcard" to "/"
                        // SD card root case: "/<dirname>"
                        strcpy(temp_path, current_directory);
                        strcat(temp_path, current_entries[index].name);
                    } else {
                        // Non-root directory case: "<directory>/<dirname>"
                        strcpy(temp_path, current_directory);
                        strcat(temp_path, "/");
                        strcat(temp_path, current_entries[index].name);
                    }
                    
                    // Check if the path fits in current_directory
                    if (strlen(temp_path) < sizeof(current_directory)) {
                        strcpy(current_directory, temp_path);
                        update_file_list();
                    } else {
                        ESP_LOGW(TAG, "Directory path too long: %s", temp_path);
                    }
                } else {
                    ESP_LOGW(TAG, "Directory path would be too long");
                }
            }
        }
    }
}

void back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int screen_id = (int)(uintptr_t)lv_event_get_user_data(e);
        
        if (screen_id == 0) { // Reboot dialog back button
            lv_screen_load(main_screen);
        } else if (screen_id == 1) { // File manager back button
            if (strcmp(current_directory, "/") != 0) {  // Changed from "/sdcard" to "/"
                // Go up one directory
                char *last_slash = strrchr(current_directory, '/');
                if (last_slash != NULL && last_slash != current_directory) {
                    *last_slash = '\0';
                    // If we end up with just "/", that's fine
                    if (strlen(current_directory) == 0) {
                        strcpy(current_directory, "/");  // Changed from "/sdcard" to "/"
                    }
                    update_file_list();
                } else {
                    // Already at SD card root, go back to main screen
                    lv_screen_load(main_screen);
                }
            } else {
                // At SD card root, go back to main screen
                lv_screen_load(main_screen);
            }
        } else if (screen_id == 2) { // Firmware loader back button
            lv_screen_load(main_screen);
        }
    }
}

void firmware_list_event_handler(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uint32_t index = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
        
        if (index < firmware_count) {
            selected_firmware = index;
            lv_obj_remove_flag(flash_btn, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Selected firmware: %s", firmware_files[index].filename);
        }
    }
}

void flash_firmware_event_handler(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && selected_firmware >= 0 && !is_flashing_in_progress()) {
        set_flashing_state(true);
        lv_obj_add_flag(flash_btn, LV_OBJ_FLAG_HIDDEN);
        
        // Show progress screen
        lv_screen_load(progress_screen);
        
        // Create a copy of the firmware path for the task
        char *firmware_path = malloc(strlen(firmware_files[selected_firmware].full_path) + 1);
        strcpy(firmware_path, firmware_files[selected_firmware].full_path);
        
        // Start flashing task pinned to CPU1 to avoid conflicts with LVGL on CPU0
        ESP_LOGI(TAG, "Starting firmware flashing task on CPU1 to avoid LVGL conflicts");
        BaseType_t result = xTaskCreatePinnedToCore(
            flash_firmware_task,    // Task function
            "flash_task",          // Task name
            8192,                  // Stack size
            firmware_path,         // Task parameter
            5,                     // Priority
            NULL,                  // Task handle (not needed)
            1                      // Pin to CPU1 (ESP32-P4 has cores 0 and 1)
        );
        
        if (result != pdPASS) {
            ESP_LOGE(TAG, "Failed to create flashing task on CPU1");
            // Cleanup and reset state
            free(firmware_path);
            set_flashing_state(false);
            lv_obj_remove_flag(flash_btn, LV_OBJ_FLAG_HIDDEN);
            lv_screen_load(firmware_loader_screen);
        }
    }
}

void splash_button_event_handler(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uint32_t choice = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
        
        // Disable boot screen timeout
        extern bool boot_screen_active;
        boot_screen_active = false;
        
        if (choice == 0) {
            // Configure firmware for boot and show manual reboot dialog
            ESP_LOGI(TAG, "User selected to boot firmware");
            esp_err_t ret = firmware_loader_boot_firmware_once();
            if (ret == ESP_OK) {
                // Show the manual reboot dialog instead of automatically rebooting
                // lv_screen_load(reboot_dialog_screen); removed since the boot function just reboots the machine
            } else {
                ESP_LOGE(TAG, "Failed to configure firmware boot: %s", esp_err_to_name(ret));
                // Stay on splash screen or go back to main
                lv_screen_load(main_screen);
            }
        } else {
            // Stay in launcher
            ESP_LOGI(TAG, "User selected to stay in launcher");
            lv_screen_load(main_screen);
        }
    }
}

// Global variables for format dialog management
static lv_obj_t *current_format_msgbox = NULL;

// Forward declarations for format functionality
static void format_confirmation_event_handler(lv_event_t *e);
static void format_result_event_handler(lv_event_t *e);

static void format_confirmation_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    // Get user choice from event user data
    int choice = (int)(uintptr_t)lv_event_get_user_data(e);
    
    // Close confirmation dialog
    if (current_format_msgbox) {
        lv_obj_del(current_format_msgbox);
        current_format_msgbox = NULL;
    }
    
    if (choice == 1) {  // Yes clicked
        ESP_LOGI(TAG, "User confirmed format operation");
        
        // Show progress message
        lv_obj_t *progress_msgbox = lv_msgbox_create(lv_screen_active());
        lv_obj_t *progress_text = lv_label_create(progress_msgbox);
        lv_label_set_text(progress_text, "Formatting SD card...\nPlease wait...");
        lv_obj_center(progress_text);
        lv_obj_set_size(progress_msgbox, 300, 150);
        lv_obj_center(progress_msgbox);
        
        // Force screen update
        lv_refr_now(NULL);
        
        // Perform format operation
        esp_err_t result = sd_manager_format();
        
        // Close progress dialog
        lv_obj_del(progress_msgbox);
        
        // Show result dialog
        lv_obj_t *result_msgbox = lv_msgbox_create(lv_screen_active());
        lv_obj_t *result_text = lv_label_create(result_msgbox);
        
        if (result == ESP_OK) {
            lv_label_set_text(result_text, "SD card formatted successfully!\nClick OK to continue.");
            ESP_LOGI(TAG, "SD card format completed successfully");
            
            // Refresh file list after successful format
            strcpy(current_directory, "/");
            update_file_list();
        } else {
            lv_label_set_text(result_text, "Format failed!\nPlease check SD card and try again.");
            ESP_LOGE(TAG, "SD card format failed with error: %s", esp_err_to_name(result));
        }
        
        // Add OK button to result dialog
        lv_obj_t *ok_btn = lv_button_create(result_msgbox);
        lv_obj_t *ok_label = lv_label_create(ok_btn);
        lv_label_set_text(ok_label, "OK");
        lv_obj_center(ok_label);
        lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_add_event_cb(ok_btn, format_result_event_handler, LV_EVENT_CLICKED, result_msgbox);
        
        lv_obj_center(result_text);
        lv_obj_set_size(result_msgbox, 300, 200);
        lv_obj_center(result_msgbox);
        
    } else {  // No clicked
        ESP_LOGI(TAG, "User cancelled format operation");
    }
}

static void format_result_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    // Get the result dialog from user data and close it
    lv_obj_t *result_msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (result_msgbox) {
        lv_obj_del(result_msgbox);
    }
}

void format_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    ESP_LOGI(TAG, "Format button clicked");
    
    // Create confirmation dialog using LVGL v9 approach
    current_format_msgbox = lv_msgbox_create(lv_screen_active());
    
    // Add warning text
    lv_obj_t *warning_text = lv_label_create(current_format_msgbox);
    lv_label_set_text(warning_text, "WARNING: This will erase all data!\n\nFormat SD card as FAT32?\n\nThis operation cannot be undone.");
    lv_obj_center(warning_text);
    
    // Create Yes button
    lv_obj_t *yes_btn = lv_button_create(current_format_msgbox);
    lv_obj_t *yes_label = lv_label_create(yes_btn);
    lv_label_set_text(yes_label, "Yes");
    lv_obj_center(yes_label);
    lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(yes_btn, lv_color_hex(0xFF5722), 0);  // Red for danger
    lv_obj_add_event_cb(yes_btn, format_confirmation_event_handler, LV_EVENT_CLICKED, (void*)1);
    
    // Create No button  
    lv_obj_t *no_btn = lv_button_create(current_format_msgbox);
    lv_obj_t *no_label = lv_label_create(no_btn);
    lv_label_set_text(no_label, "No");
    lv_obj_center(no_label);
    lv_obj_align(no_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(no_btn, format_confirmation_event_handler, LV_EVENT_CLICKED, (void*)0);
    
    // Style and position the dialog
    lv_obj_set_size(current_format_msgbox, 350, 250);
    lv_obj_center(current_format_msgbox);
    lv_obj_set_style_bg_color(current_format_msgbox, lv_color_white(), 0);
    lv_obj_set_style_border_width(current_format_msgbox, 2, 0);
    lv_obj_set_style_border_color(current_format_msgbox, lv_color_hex(0xFF5722), 0);
}

void mount_unmount_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    ESP_LOGI(TAG, "Mount/Unmount button clicked");
    
    if (sd_manager_is_mounted()) {
        // Unmount SD card
        ESP_LOGI(TAG, "Unmounting SD card");
        esp_err_t ret = sd_manager_unmount();
        
        // Show result message
        lv_obj_t *result_msgbox = lv_msgbox_create(lv_screen_active());
        lv_obj_t *result_text = lv_label_create(result_msgbox);
        
        if (ret == ESP_OK) {
            lv_label_set_text(result_text, "SD card unmounted successfully!");
            ESP_LOGI(TAG, "SD card unmounted successfully");
        } else {
            lv_label_set_text(result_text, "Failed to unmount SD card!");
            ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        }
        
        // Add OK button
        lv_obj_t *ok_btn = lv_button_create(result_msgbox);
        lv_obj_t *ok_label = lv_label_create(ok_btn);
        lv_label_set_text(ok_label, "OK");
        lv_obj_center(ok_label);
        lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_add_event_cb(ok_btn, format_result_event_handler, LV_EVENT_CLICKED, result_msgbox);
        
        lv_obj_center(result_text);
        lv_obj_set_size(result_msgbox, 300, 150);
        lv_obj_center(result_msgbox);
        
    } else {
        // Mount SD card
        ESP_LOGI(TAG, "Mounting SD card");
        esp_err_t ret = sd_manager_mount();
        
        // Show result message
        lv_obj_t *result_msgbox = lv_msgbox_create(lv_screen_active());
        lv_obj_t *result_text = lv_label_create(result_msgbox);
        
        if (ret == ESP_OK) {
            lv_label_set_text(result_text, "SD card mounted successfully!");
            ESP_LOGI(TAG, "SD card mounted successfully");
        } else {
            lv_label_set_text(result_text, "Failed to mount SD card!\\nCheck if card is inserted.");
            ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        }
        
        // Add OK button
        lv_obj_t *ok_btn = lv_button_create(result_msgbox);
        lv_obj_t *ok_label = lv_label_create(ok_btn);
        lv_label_set_text(ok_label, "OK");
        lv_obj_center(ok_label);
        lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_add_event_cb(ok_btn, format_result_event_handler, LV_EVENT_CLICKED, result_msgbox);
        
        lv_obj_center(result_text);
        lv_obj_set_size(result_msgbox, 300, 150);
        lv_obj_center(result_msgbox);
    }
    
    // Refresh file list after mount/unmount
    update_file_list();
}

// File browser v2 event handlers
void file_browser_v2_up_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    ESP_LOGI(TAG, "File browser v2: Up directory clicked");
    gui_file_browser_v2_go_up();
}

void file_browser_v2_prev_page_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    ESP_LOGI(TAG, "File browser v2: Previous page clicked");
    gui_file_browser_v2_prev_page();
}

void file_browser_v2_next_page_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    ESP_LOGI(TAG, "File browser v2: Next page clicked");
    gui_file_browser_v2_next_page();
}

void file_browser_v2_item_click_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    int file_index = (int)(intptr_t)lv_event_get_user_data(e);
    ESP_LOGI(TAG, "File browser v2: Item %d clicked", file_index);
    
    // TODO: Handle file/directory selection and navigation
    // This will be implemented when we integrate with the existing file navigation
}

void file_browser_v2_multi_select_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;
    
    lv_obj_t *btn = lv_event_get_target(e);
    bool is_checked = lv_obj_has_state(btn, LV_STATE_CHECKED);
    
    ESP_LOGI(TAG, "File browser v2: Multi-select %s", is_checked ? "enabled" : "disabled");
    gui_file_browser_v2_set_multi_select(is_checked);
}