#include "gui_events.h"
#include "gui_screens.h"
#include "gui_progress.h"
#include "gui_state.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "GUI_EVENTS";

void main_menu_event_handler(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uint32_t menu_id = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
        
        switch (menu_id) {
            case 0: // File Manager
                strcpy(current_directory, "/sdcard");
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
                    if (strcmp(current_directory, "/sdcard") == 0) {
                        // SD card root case: "/sdcard/<dirname>"
                        strcpy(temp_path, current_directory);
                        strcat(temp_path, "/");
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
                lv_screen_load(reboot_dialog_screen);
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

void back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        int screen_id = (int)(uintptr_t)lv_event_get_user_data(e);
        
        if (screen_id == 0) { // Reboot dialog back button
            lv_screen_load(main_screen);
        } else if (screen_id == 1) { // File manager back button
            if (strcmp(current_directory, "/sdcard") != 0) {
                // Go up one directory
                char *last_slash = strrchr(current_directory, '/');
                if (last_slash != NULL && last_slash != current_directory) {
                    *last_slash = '\0';
                    // If we end up with just "/sdcard", that's fine
                    if (strlen(current_directory) == 0) {
                        strcpy(current_directory, "/sdcard");
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