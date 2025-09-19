#include "gui_events.h"
#include "gui_screens.h"
#include "gui_progress.h"
#include "gui_state.h"
#include "gui_file_browser_v2.h"
#include "gui_screen_settings.h"
#include "gui_screen_tools.h"
#include "gui_screen_text_editor.h"
#include "gui_screen_python_launcher.h"
#include "gui_styles.h"
#include "firmware_loader.h"
#include "sd_manager.h"
#include "file_operations.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

// External screen references
extern lv_obj_t *file_manager_screen;

// Forward declarations for clean and eject handlers
static void clean_cancel_event_handler(lv_event_t *e);
static void clean_confirm_event_handler(lv_event_t *e);
static void clean_result_close_event_handler(lv_event_t *e);
static void eject_info_close_event_handler(lv_event_t *e);

// Forward declarations for keyboard handlers
static void textarea_focus_handler(lv_event_t *e);
static void keyboard_ready_handler(lv_event_t *e);
static void keyboard_cancel_handler(lv_event_t *e);

static const char *TAG = "GUI_EVENTS";

// Helper function to clear file selections
static void clear_file_selections(void);

// File operation helper handlers
static void delete_confirmation_handler(lv_event_t *e);
static void delete_cancel_handler(lv_event_t *e);
static void rename_confirm_handler(lv_event_t *e);
static void rename_cancel_handler(lv_event_t *e);
static void file_open_choice_handler(lv_event_t *e);
static void delete_refresh_callback(lv_timer_t *timer);

// Rename context structure
typedef struct {
    lv_obj_t *msgbox;
    lv_obj_t *textarea;
    lv_obj_t *keyboard;
    int file_index;
} rename_context_t;


// Format functionality temporarily removed

void main_menu_event_handler(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uint32_t menu_id = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
        
        switch (menu_id) {
            case 0: // File Manager
                strcpy(current_directory, "/");  // Changed from "/sdcard" to "/"
                update_file_manager_screen();
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
            case 4: // Eject Firmware (standard)
                if (firmware_loader_is_firmware_ready()) {
                    ESP_LOGI(TAG, "Ejecting firmware...");
                    esp_err_t ret = firmware_loader_unload_firmware();
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "✓ Firmware ejected successfully");
                        // Refresh the main screen to update button states
                        update_main_screen();
                        lv_screen_load(main_screen);
                    } else {
                        ESP_LOGE(TAG, "Failed to eject firmware: %s", esp_err_to_name(ret));
                    }
                } else {
                    ESP_LOGW(TAG, "No firmware available to eject");
                    // Show user feedback for no firmware
                    lv_obj_t *info_mbox = lv_msgbox_create(lv_screen_active());
                    lv_obj_t *info_text = lv_label_create(info_mbox);
                    lv_label_set_text(info_text, "No Firmware\n\nNo firmware is currently loaded\nto eject.");
                    lv_obj_set_style_text_color(info_text, lv_color_hex(0xFFBF00), 0);
                    lv_obj_center(info_text);
                    lv_obj_set_size(info_mbox, 300, 150);
                    lv_obj_center(info_mbox);

                    // Add close button
                    lv_obj_t *close_btn = lv_button_create(info_mbox);
                    lv_obj_set_size(close_btn, 80, 35);
                    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
                    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x666666), 0);
                    lv_obj_add_event_cb(close_btn, eject_info_close_event_handler, LV_EVENT_CLICKED, info_mbox);
                    lv_obj_t *close_label = lv_label_create(close_btn);
                    lv_label_set_text(close_label, "OK");
                    lv_obj_center(close_label);
                }
                break;
            case 5: // Tools
                ESP_LOGI(TAG, "Opening Tools menu");
                show_tools_screen();
                break;
            case 6: // Factory Reset
                ESP_LOGI(TAG, "Factory reset requested");
                lv_obj_t *factory_mbox = lv_msgbox_create(lv_screen_active());
                lv_obj_t *title_text = lv_label_create(factory_mbox);
                lv_label_set_text(title_text, "Factory Reset");
                lv_obj_align(title_text, LV_ALIGN_TOP_MID, 0, 10);
                lv_obj_t *msg_text = lv_label_create(factory_mbox);
                lv_label_set_text(msg_text, "This will restore the launcher to\noriginal state. Continue?");
                lv_obj_center(msg_text);
                lv_obj_set_size(factory_mbox, 300, 150);
                lv_obj_center(factory_mbox);
                // TODO: Add factory reset confirmation handler
                break;
            case 7: // Export to SD
                if (firmware_loader_is_firmware_ready()) {
                    ESP_LOGI(TAG, "Exporting firmware to SD...");
                    // Generate unique filename with timestamp
                    char export_path[64];
                    snprintf(export_path, sizeof(export_path), "/exported_firmware_%lu.bin", (unsigned long)time(NULL));
                    esp_err_t ret = firmware_loader_export_to_sd(export_path);
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "✓ Firmware exported to: %s", export_path);
                        lv_obj_t *export_mbox = lv_msgbox_create(lv_screen_active());
                        lv_obj_t *export_text = lv_label_create(export_mbox);
                        lv_label_set_text(export_text, "Export Complete\n\nFirmware exported to SD card!");
                        lv_obj_center(export_text);
                        lv_obj_set_size(export_mbox, 300, 120);
                        lv_obj_center(export_mbox);
                    } else {
                        ESP_LOGE(TAG, "Failed to export firmware: %s", esp_err_to_name(ret));
                        lv_obj_t *error_mbox = lv_msgbox_create(lv_screen_active());
                        lv_obj_t *error_text = lv_label_create(error_mbox);
                        lv_label_set_text(error_text, "Export Failed\n\nFailed to export firmware\nto SD card.");
                        lv_obj_center(error_text);
                        lv_obj_set_size(error_mbox, 300, 120);
                        lv_obj_center(error_mbox);
                    }
                } else {
                    ESP_LOGW(TAG, "No firmware available to export");
                }
                break;
            case 8: // Clean Partition
                ESP_LOGI(TAG, "Force cleaning partition...");
                lv_obj_t *clean_mbox = lv_msgbox_create(lv_screen_active());
                lv_obj_t *clean_title = lv_label_create(clean_mbox);
                lv_label_set_text(clean_title, "Clean Partition");
                lv_obj_align(clean_title, LV_ALIGN_TOP_MID, 0, 10);
                lv_obj_t *clean_text = lv_label_create(clean_mbox);
                lv_label_set_text(clean_text, "Force clean corrupted partition?\nThis will erase everything.");
                lv_obj_center(clean_text);
                lv_obj_set_size(clean_mbox, 300, 180);
                lv_obj_center(clean_mbox);

                // Create button container
                lv_obj_t *btn_container = lv_obj_create(clean_mbox);
                lv_obj_set_size(btn_container, lv_pct(90), 40);
                lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
                lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
                lv_obj_set_style_border_opa(btn_container, LV_OPA_TRANSP, 0);
                lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
                lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

                // Cancel button
                lv_obj_t *cancel_btn = lv_button_create(btn_container);
                lv_obj_set_size(cancel_btn, 80, 35);
                lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x666666), 0);
                lv_obj_add_event_cb(cancel_btn, clean_cancel_event_handler, LV_EVENT_CLICKED, clean_mbox);
                lv_obj_t *cancel_label = lv_label_create(cancel_btn);
                lv_label_set_text(cancel_label, "Cancel");
                lv_obj_center(cancel_label);

                // Clean button
                lv_obj_t *clean_btn = lv_button_create(btn_container);
                lv_obj_set_size(clean_btn, 80, 35);
                lv_obj_set_style_bg_color(clean_btn, lv_color_hex(0xFF5722), 0);
                lv_obj_add_event_cb(clean_btn, clean_confirm_event_handler, LV_EVENT_CLICKED, clean_mbox);
                lv_obj_t *clean_label = lv_label_create(clean_btn);
                lv_label_set_text(clean_label, "Clean");
                lv_obj_center(clean_label);
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
                        // Clear selections when changing directory
                        clear_file_selections();
                        update_file_list();
                    } else {
                        ESP_LOGW(TAG, "Directory path too long: %s", temp_path);
                    }
                } else {
                    ESP_LOGW(TAG, "Directory path would be too long");
                }
            } else {
                // Check if it's a supported file and determine available options using file type detection
                file_type_t file_type = file_ops_detect_type(current_entries[index].name);
                bool can_edit = file_ops_is_editable(file_type);
                bool can_run_python = (file_type == FILE_TYPE_PYTHON);

                if (can_edit && can_run_python) {
                    // File can be opened in both text editor and Python launcher - show choice dialog
                    char full_path[1024];
                    if (strcmp(current_directory, "/") == 0) {
                        snprintf(full_path, sizeof(full_path), "/%s", current_entries[index].name);
                    } else {
                        snprintf(full_path, sizeof(full_path), "%s/%s",
                                current_directory, current_entries[index].name);
                    }

                    // Create choice dialog
                    lv_obj_t *msgbox = lv_msgbox_create(lv_screen_active());

                    // Title
                    lv_obj_t *title_text = lv_label_create(msgbox);
                    lv_label_set_text(title_text, "Open File");
                    lv_obj_set_style_text_font(title_text, &lv_font_montserrat_16, 0);
                    lv_obj_align(title_text, LV_ALIGN_TOP_MID, 0, 10);

                    // Message
                    lv_obj_t *msg_text = lv_label_create(msgbox);
                    char msg[128];
                    snprintf(msg, sizeof(msg), "How would you like to open\n%s?", current_entries[index].name);
                    lv_label_set_text(msg_text, msg);
                    lv_obj_set_style_text_align(msg_text, LV_TEXT_ALIGN_CENTER, 0);
                    lv_obj_align(msg_text, LV_ALIGN_CENTER, 0, -10);

                    // Text Editor button
                    lv_obj_t *edit_btn = lv_button_create(msgbox);
                    lv_obj_set_size(edit_btn, 100, 40);
                    lv_obj_align(edit_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
                    apply_button_style(edit_btn);
                    lv_obj_set_style_bg_color(edit_btn, lv_color_hex(0x4ecdc4), 0);

                    lv_obj_t *edit_label = lv_label_create(edit_btn);
                    lv_label_set_text(edit_label, LV_SYMBOL_EDIT " Edit");
                    lv_obj_center(edit_label);

                    // Python Launcher button
                    lv_obj_t *run_btn = lv_button_create(msgbox);
                    lv_obj_set_size(run_btn, 100, 40);
                    lv_obj_align(run_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
                    apply_button_style(run_btn);
                    lv_obj_set_style_bg_color(run_btn, lv_color_hex(0x3d5a80), 0);

                    lv_obj_t *run_label = lv_label_create(run_btn);
                    lv_label_set_text(run_label, LV_SYMBOL_PLAY " Run");
                    lv_obj_center(run_label);

                    // Store file path for button handlers - use separate copies to avoid data corruption
                    char *edit_path_copy = malloc(strlen(full_path) + 1);
                    char *run_path_copy = malloc(strlen(full_path) + 1);
                    strcpy(edit_path_copy, full_path);
                    strcpy(run_path_copy, full_path);

                    // Store choice in user data using different pointers
                    lv_obj_add_event_cb(edit_btn, file_open_choice_handler, LV_EVENT_CLICKED,
                                       (void*)(uintptr_t)(((uintptr_t)edit_path_copy & ~0x3UL) | 0x1)); // Edit choice
                    lv_obj_add_event_cb(run_btn, file_open_choice_handler, LV_EVENT_CLICKED,
                                       (void*)(uintptr_t)(((uintptr_t)run_path_copy & ~0x3UL) | 0x2)); // Run choice

                    lv_obj_set_size(msgbox, 300, 200);
                    lv_obj_center(msgbox);

                } else if (can_edit) {
                    // Only text editor available
                    char full_path[1024];
                    if (strcmp(current_directory, "/") == 0) {
                        snprintf(full_path, sizeof(full_path), "/%s", current_entries[index].name);
                    } else {
                        snprintf(full_path, sizeof(full_path), "%s/%s",
                                current_directory, current_entries[index].name);
                    }

                    ESP_LOGI(TAG, "Opening text file in editor: %s", full_path);
                    create_text_editor_screen();
                    esp_err_t ret = text_editor_open_file(full_path);
                    if (ret == ESP_OK) {
                        lv_screen_load(text_editor_screen);
                    } else {
                        ESP_LOGE(TAG, "Failed to open file in text editor: %s", esp_err_to_name(ret));
                    }
                } else if (can_run_python) {
                    // Only Python launcher available
                    ESP_LOGI(TAG, "Opening Python file in launcher: %s", current_entries[index].name);
                    show_python_launcher_screen();
                } else {
                    ESP_LOGI(TAG, "File type not supported: %s", current_entries[index].name);
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
                    // Clear selections when changing directory
                    clear_file_selections();
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

// Helper function to clear file selections
static void clear_file_selections(void) {
    for (int i = 0; i < 32; i++) {
        selected_files[i] = false;
    }
    selected_file_count = 0;
}

void directory_up_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    ESP_LOGI(TAG, "Directory up button clicked from: %s", current_directory);
    
    if (strcmp(current_directory, "/") != 0) {
        // Go up one directory
        char *last_slash = strrchr(current_directory, '/');
        if (last_slash != NULL) {
            if (last_slash == current_directory) {
                // We're at something like "/dirname", go back to root
                strcpy(current_directory, "/");
            } else {
                // Normal case - truncate at last slash
                *last_slash = '\0';
                // If we somehow end up with empty string, set to root
                if (strlen(current_directory) == 0) {
                    strcpy(current_directory, "/");
                }
            }
            // Clear selections when changing directory
            clear_file_selections();
            update_file_list();
            ESP_LOGI(TAG, "Navigated up to directory: %s", current_directory);
        } else {
            // No slash found (shouldn't happen), go to root
            strcpy(current_directory, "/");
            clear_file_selections();
            update_file_list();
            ESP_LOGI(TAG, "Reset to root directory");
        }
    } else {
        // At root, can't go up further
        ESP_LOGI(TAG, "Already at root directory");
    }
}

void file_selection_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;
    
    int file_index = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *checkbox = lv_event_get_target(e);
    bool is_checked = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
    
    if (file_index >= 0 && file_index < current_entry_count) {
        // Update selection state
        bool was_selected = selected_files[file_index];
        selected_files[file_index] = is_checked;
        
        // Update selection count
        if (is_checked && !was_selected) {
            selected_file_count++;
        } else if (!is_checked && was_selected) {
            selected_file_count--;
        }
        
        ESP_LOGI(TAG, "File %d (%s) %s. Total selected: %d", 
                 file_index, current_entries[file_index].name,
                 is_checked ? "selected" : "deselected", selected_file_count);
        
        // Refresh the file list to update visual highlighting
        update_file_list();
    }
}

void toggle_selection_mode_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    // Toggle selection mode
    file_selection_enabled = !file_selection_enabled;
    
    ESP_LOGI(TAG, "Selection mode %s", file_selection_enabled ? "enabled" : "disabled");
    
    if (!file_selection_enabled) {
        // Clear all selections when disabling selection mode
        clear_file_selections();
    }
    
    // Update the select button text
    lv_obj_t *select_btn = lv_event_get_target(e);
    lv_obj_t *select_btn_label = lv_obj_get_child(select_btn, 0);
    if (select_btn_label) {
        if (file_selection_enabled) {
            lv_label_set_text(select_btn_label, LV_SYMBOL_CLOSE " Cancel");
        } else {
            lv_label_set_text(select_btn_label, LV_SYMBOL_OK " Select");
        }
    }
    
    // Refresh the file list to show/hide checkboxes
    update_file_list();
}

// File operation event handlers
void delete_files_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    if (selected_file_count == 0) {
        ESP_LOGW(TAG, "No files selected for deletion");
        return;
    }
    
    // Create confirmation dialog
    lv_obj_t *msgbox = lv_msgbox_create(lv_screen_active());
    
    // Create message text
    lv_obj_t *msg_text = lv_label_create(msgbox);
    char msg[256];
    if (selected_file_count == 1) {
        // Find the selected file
        int selected_idx = -1;
        for (int i = 0; i < current_entry_count; i++) {
            if (selected_files[i]) {
                selected_idx = i;
                break;
            }
        }
        snprintf(msg, sizeof(msg), "Delete '%s'?", 
                 selected_idx >= 0 ? current_entries[selected_idx].name : "file");
    } else {
        snprintf(msg, sizeof(msg), "Delete %d selected items?", selected_file_count);
    }
    lv_label_set_text(msg_text, msg);
    lv_obj_center(msg_text);
    
    // Create Yes button
    lv_obj_t *yes_btn = lv_button_create(msgbox);
    lv_obj_t *yes_label = lv_label_create(yes_btn);
    lv_label_set_text(yes_label, "Yes");
    lv_obj_center(yes_label);
    lv_obj_align(yes_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_set_style_bg_color(yes_btn, lv_color_hex(0xFF5722), 0);
    
    // Create No button
    lv_obj_t *no_btn = lv_button_create(msgbox);
    lv_obj_t *no_label = lv_label_create(no_btn);
    lv_label_set_text(no_label, "No");
    lv_obj_center(no_label);
    lv_obj_align(no_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    
    // Add event handlers for deletion confirmation
    lv_obj_add_event_cb(yes_btn, delete_confirmation_handler, LV_EVENT_CLICKED, msgbox);
    lv_obj_add_event_cb(no_btn, delete_cancel_handler, LV_EVENT_CLICKED, msgbox);
    
    lv_obj_set_size(msgbox, 300, 200);
    lv_obj_center(msgbox);
}

static void delete_confirmation_handler(lv_event_t *e) {
    lv_obj_t *msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    
    ESP_LOGI(TAG, "Deleting %d selected files", selected_file_count);
    
    int deleted_count = 0;
    int failed_count = 0;
    
    // Delete each selected file
    for (int i = 0; i < current_entry_count; i++) {
        if (selected_files[i]) {
            char full_path[3072];  // Large buffer to satisfy compiler warnings
            if (strcmp(current_directory, "/") == 0) {
                snprintf(full_path, sizeof(full_path), "/%s", current_entries[i].name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s/%s", 
                        current_directory, current_entries[i].name);
            }
            
            esp_err_t ret;
            if (current_entries[i].is_directory) {
                ret = file_ops_delete_directory(full_path);
            } else {
                ret = file_ops_delete_file(full_path);
            }
            
            if (ret == ESP_OK) {
                deleted_count++;
                ESP_LOGI(TAG, "Deleted: %s", full_path);
            } else {
                failed_count++;
                ESP_LOGE(TAG, "Failed to delete: %s", full_path);
            }
        }
    }
    
    // Close dialog
    lv_obj_del(msgbox);

    // Clear selections and exit selection mode
    clear_file_selections();
    file_selection_enabled = false;

    // CRITICAL FIX: Use delayed update to avoid stack corruption during delete
    // Schedule file list update for 100ms later to avoid immediate stack issues
    static lv_timer_t *delete_refresh_timer = NULL;
    if (delete_refresh_timer) {
        lv_timer_del(delete_refresh_timer);
    }

    delete_refresh_timer = lv_timer_create(delete_refresh_callback, 100, NULL);
    lv_timer_set_repeat_count(delete_refresh_timer, 1);

    ESP_LOGI(TAG, "Deletion complete. Deleted: %d, Failed: %d", deleted_count, failed_count);
}

static void delete_refresh_callback(lv_timer_t *timer) {
    update_file_list();
    update_toolbar_button_states(); // Update button states including select button text
    lv_timer_del(timer);
}

static void delete_cancel_handler(lv_event_t *e) {
    lv_obj_t *msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_del(msgbox);
    ESP_LOGI(TAG, "Deletion cancelled");
}

void copy_files_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    if (selected_file_count == 0) {
        ESP_LOGW(TAG, "No files selected for copy");
        return;
    }
    
    // For now, only support single file copy (file_operations module limitation)
    // Find first selected file
    int selected_idx = -1;
    for (int i = 0; i < current_entry_count; i++) {
        if (selected_files[i]) {
            selected_idx = i;
            break;
        }
    }
    
    if (selected_idx >= 0) {
        char full_path[3072];  // Large buffer to satisfy compiler warnings
        if (strcmp(current_directory, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", current_entries[selected_idx].name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", 
                    current_directory, current_entries[selected_idx].name);
        }
        
        // Copy to clipboard
        esp_err_t ret = file_ops_copy_to_clipboard(full_path, false);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Copied to clipboard: %s", full_path);
        } else {
            ESP_LOGE(TAG, "Failed to copy to clipboard: %s", full_path);
        }
    }

    // Clear selections and exit selection mode
    clear_file_selections();
    file_selection_enabled = false;

    // CRITICAL FIX: Use delayed update to avoid stack corruption during copy
    static lv_timer_t *copy_refresh_timer = NULL;
    if (copy_refresh_timer) {
        lv_timer_del(copy_refresh_timer);
    }
    copy_refresh_timer = lv_timer_create(delete_refresh_callback, 100, NULL);
    lv_timer_set_repeat_count(copy_refresh_timer, 1);
}

void move_files_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    if (selected_file_count == 0) {
        ESP_LOGW(TAG, "No files selected for move");
        return;
    }
    
    // For now, only support single file move (file_operations module limitation)
    // Find first selected file
    int selected_idx = -1;
    for (int i = 0; i < current_entry_count; i++) {
        if (selected_files[i]) {
            selected_idx = i;
            break;
        }
    }
    
    if (selected_idx >= 0) {
        char full_path[3072];  // Large buffer to satisfy compiler warnings
        if (strcmp(current_directory, "/") == 0) {
            snprintf(full_path, sizeof(full_path), "/%s", current_entries[selected_idx].name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s/%s", 
                    current_directory, current_entries[selected_idx].name);
        }
        
        // Copy to clipboard for cut operation
        esp_err_t ret = file_ops_copy_to_clipboard(full_path, true);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Prepared for move: %s", full_path);
        } else {
            ESP_LOGE(TAG, "Failed to prepare for move: %s", full_path);
        }
    }
    
    // Clear selections and exit selection mode
    clear_file_selections();
    file_selection_enabled = false;

    // CRITICAL FIX: Use delayed update to avoid stack corruption during move
    static lv_timer_t *move_refresh_timer = NULL;
    if (move_refresh_timer) {
        lv_timer_del(move_refresh_timer);
    }
    move_refresh_timer = lv_timer_create(delete_refresh_callback, 100, NULL);
    lv_timer_set_repeat_count(move_refresh_timer, 1);
}

void paste_files_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    if (!file_ops_clipboard_has_content()) {
        ESP_LOGW(TAG, "No files in clipboard");
        return;
    }
    
    ESP_LOGI(TAG, "Pasting to %s", current_directory);
    
    // Paste from clipboard to current directory
    esp_err_t ret = file_ops_paste_from_clipboard(current_directory);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Paste operation completed successfully");
    } else {
        ESP_LOGE(TAG, "Paste operation failed");
    }
    
    // Refresh file list
    update_file_list();
}

void rename_file_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
    
    if (selected_file_count != 1) {
        ESP_LOGW(TAG, "Rename requires exactly one file selected");
        return;
    }
    
    // Find the selected file
    int selected_idx = -1;
    for (int i = 0; i < current_entry_count; i++) {
        if (selected_files[i]) {
            selected_idx = i;
            break;
        }
    }
    
    if (selected_idx < 0) return;
    
    // Create rename dialog
    lv_obj_t *msgbox = lv_msgbox_create(lv_screen_active());
    
    lv_obj_t *title = lv_label_create(msgbox);
    lv_label_set_text(title, "Rename File/Folder");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create text area for new name
    lv_obj_t *ta = lv_textarea_create(msgbox);
    lv_textarea_set_text(ta, current_entries[selected_idx].name);
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_width(ta, 250);
    lv_obj_align(ta, LV_ALIGN_CENTER, 0, -10);
    
    // Create OK button
    lv_obj_t *ok_btn = lv_button_create(msgbox);
    lv_obj_t *ok_label = lv_label_create(ok_btn);
    lv_label_set_text(ok_label, "OK");
    lv_obj_center(ok_label);
    lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    
    // Create Cancel button
    lv_obj_t *cancel_btn = lv_button_create(msgbox);
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    
    // Store context for the rename operation
    rename_context_t *ctx = malloc(sizeof(rename_context_t));
    ctx->msgbox = msgbox;
    ctx->textarea = ta;
    ctx->file_index = selected_idx;

    lv_obj_add_event_cb(ok_btn, rename_confirm_handler, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(cancel_btn, rename_cancel_handler, LV_EVENT_CLICKED, ctx);

    // Create keyboard for text input
    lv_obj_t *keyboard = lv_keyboard_create(lv_screen_active());
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_set_size(keyboard, lv_pct(100), lv_pct(50));
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    // Add focus event to textarea to show keyboard
    lv_obj_add_event_cb(ta, textarea_focus_handler, LV_EVENT_FOCUSED, keyboard);
    lv_obj_add_event_cb(ta, textarea_focus_handler, LV_EVENT_DEFOCUSED, keyboard);

    // Add keyboard events for rename context
    lv_obj_add_event_cb(keyboard, keyboard_ready_handler, LV_EVENT_READY, ctx);
    lv_obj_add_event_cb(keyboard, keyboard_cancel_handler, LV_EVENT_CANCEL, ctx);

    // Store keyboard in context
    ctx->keyboard = keyboard;

    lv_obj_set_size(msgbox, 300, 200);
    lv_obj_align(msgbox, LV_ALIGN_CENTER, 0, -100); // Move up 100px to avoid keyboard
}

static void rename_confirm_handler(lv_event_t *e) {
    rename_context_t *ctx = (rename_context_t*)lv_event_get_user_data(e);
    
    const char *new_name = lv_textarea_get_text(ctx->textarea);
    
    if (strlen(new_name) == 0) {
        ESP_LOGW(TAG, "New name cannot be empty");
        lv_obj_del(ctx->msgbox);
        free(ctx);
        return;
    }
    
    // Build old and new paths
    char old_path[3072];  // Large buffer to satisfy compiler warnings
    char new_path[3072];  // Large buffer to satisfy compiler warnings
    
    if (strcmp(current_directory, "/") == 0) {
        snprintf(old_path, sizeof(old_path), "/%s", current_entries[ctx->file_index].name);
        snprintf(new_path, sizeof(new_path), "/%s", new_name);
    } else {
        snprintf(old_path, sizeof(old_path), "%s/%s", 
                current_directory, current_entries[ctx->file_index].name);
        snprintf(new_path, sizeof(new_path), "%s/%s", 
                current_directory, new_name);
    }
    
    // Perform rename
    esp_err_t ret = file_ops_rename(old_path, new_path);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Renamed: %s -> %s", old_path, new_path);
    } else {
        ESP_LOGE(TAG, "Failed to rename: %s", old_path);
    }
    
    // Clean up - hide keyboard first
    if (ctx->keyboard) {
        lv_obj_add_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del(ctx->keyboard);
    }
    lv_obj_del(ctx->msgbox);
    free(ctx);
    
    // Clear selections and refresh
    clear_file_selections();
    file_selection_enabled = false;
    update_file_list();
}

static void rename_cancel_handler(lv_event_t *e) {
    rename_context_t *ctx = (rename_context_t*)lv_event_get_user_data(e);
    // Hide and delete keyboard if it exists
    if (ctx->keyboard) {
        lv_obj_add_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del(ctx->keyboard);
    }
    lv_obj_del(ctx->msgbox);
    free(ctx);
    ESP_LOGI(TAG, "Rename cancelled");
}

static void file_open_choice_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    // Extract choice and file path from user data
    uintptr_t data = (uintptr_t)lv_event_get_user_data(e);
    int choice = data & 0x3; // Last 2 bits contain choice
    char *file_path = (char*)(data & ~0x3UL); // Remove choice bits to get pointer

    lv_obj_t *msgbox = lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target(e)));
    lv_obj_del(msgbox);

    if (choice == 1) {
        // Open in text editor
        ESP_LOGI(TAG, "Opening file in text editor: %s", file_path);
        create_text_editor_screen();
        esp_err_t ret = text_editor_open_file(file_path);
        if (ret == ESP_OK) {
            lv_screen_load(text_editor_screen);
        } else {
            ESP_LOGE(TAG, "Failed to open file in text editor: %s", esp_err_to_name(ret));
        }
    } else if (choice == 2) {
        // Open in Python launcher
        ESP_LOGI(TAG, "Opening file in Python launcher: %s", file_path);
        show_python_launcher_screen();
    }

    // Clean up allocated path
    free(file_path);
}

// Clean partition confirmation handlers
static void clean_cancel_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_t *msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (msgbox) {
        lv_obj_del(msgbox);
    }
    ESP_LOGI("GUI_EVENTS", "Clean partition canceled");
}

static void clean_confirm_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_t *msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (msgbox) {
        lv_obj_del(msgbox);
    }

    ESP_LOGI("GUI_EVENTS", "Performing force clean partition...");
    esp_err_t ret = firmware_loader_clean_partition();

    // Show result dialog
    lv_obj_t *result_mbox = lv_msgbox_create(lv_screen_active());
    lv_obj_t *result_text = lv_label_create(result_mbox);

    if (ret == ESP_OK) {
        lv_label_set_text(result_text, "Success\n\nPartition cleaned successfully.\nCorrupted data has been erased.");
        lv_obj_set_style_text_color(result_text, lv_color_hex(0x4CAF50), 0);
    } else {
        char error_msg[200];
        snprintf(error_msg, sizeof(error_msg), "Failed\n\nClean partition failed:\n%s", esp_err_to_name(ret));
        lv_label_set_text(result_text, error_msg);
        lv_obj_set_style_text_color(result_text, lv_color_hex(0xFF5722), 0);
    }

    lv_obj_center(result_text);
    lv_obj_set_size(result_mbox, 300, 180);
    lv_obj_center(result_mbox);

    // Add close button
    lv_obj_t *close_btn = lv_button_create(result_mbox);
    lv_obj_set_size(close_btn, 80, 35);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x666666), 0);
    lv_obj_add_event_cb(close_btn, clean_result_close_event_handler, LV_EVENT_CLICKED, result_mbox);
    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "OK");
    lv_obj_center(close_label);
}

static void clean_result_close_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_t *msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (msgbox) {
        lv_obj_del(msgbox);
    }
    ESP_LOGI("GUI_EVENTS", "Clean result dialog closed");
}

static void eject_info_close_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_t *msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (msgbox) {
        lv_obj_del(msgbox);
    }
    ESP_LOGI("GUI_EVENTS", "Eject info dialog closed");
}

// File/folder creation context structure
typedef struct {
    lv_obj_t *msgbox;
    lv_obj_t *textarea;
    lv_obj_t *keyboard;
    bool is_folder;
} create_context_t;

// Forward declarations for creation handlers
static void create_confirm_handler(lv_event_t *e);
static void create_cancel_handler(lv_event_t *e);
static void simple_dialog_close_handler(lv_event_t *e);

void create_file_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    ESP_LOGI(TAG, "Create file dialog requested");

    // Create file creation dialog
    lv_obj_t *msgbox = lv_msgbox_create(lv_screen_active());

    // Title
    lv_obj_t *title = lv_label_create(msgbox);
    lv_label_set_text(title, "Create New File");
    lv_obj_set_style_text_font(title, THEME_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, THEME_PRIMARY_COLOR, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Instruction text
    lv_obj_t *instruction = lv_label_create(msgbox);
    lv_label_set_text(instruction, "Enter filename:");
    lv_obj_set_style_text_color(instruction, THEME_TEXT_COLOR, 0);
    lv_obj_align(instruction, LV_ALIGN_TOP_LEFT, 20, 50);

    // Text area for filename input
    lv_obj_t *ta = lv_textarea_create(msgbox);
    lv_textarea_set_text(ta, "NewFile.txt");
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_width(ta, 280);
    lv_obj_set_height(ta, 50);
    lv_obj_align(ta, LV_ALIGN_CENTER, 0, 0);

    // Style the text area for touch-friendly usage
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_color(ta, THEME_PRIMARY_COLOR, 0);
    lv_obj_set_style_border_width(ta, 2, 0);
    lv_obj_set_style_text_color(ta, THEME_TEXT_COLOR, 0);
    lv_obj_set_style_radius(ta, 5, 0);
    lv_obj_set_style_pad_all(ta, 8, 0);

    // Create button
    lv_obj_t *create_btn = lv_button_create(msgbox);
    lv_obj_set_size(create_btn, 100, 45);
    lv_obj_align(create_btn, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    apply_button_style(create_btn);
    lv_obj_set_style_bg_color(create_btn, THEME_SUCCESS_COLOR, 0);
    lv_obj_set_style_text_color(create_btn, THEME_BG_COLOR, 0);

    lv_obj_t *create_label = lv_label_create(create_btn);
    lv_label_set_text(create_label, LV_SYMBOL_PLUS " Create");
    lv_obj_center(create_label);

    // Cancel button
    lv_obj_t *cancel_btn = lv_button_create(msgbox);
    lv_obj_set_size(cancel_btn, 100, 45);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
    apply_button_style(cancel_btn);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x666666), 0);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, LV_SYMBOL_CLOSE " Cancel");
    lv_obj_center(cancel_label);

    // Store context for the creation operation
    create_context_t *ctx = malloc(sizeof(create_context_t));
    ctx->msgbox = msgbox;
    ctx->textarea = ta;
    ctx->is_folder = false;

    lv_obj_add_event_cb(create_btn, create_confirm_handler, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(cancel_btn, create_cancel_handler, LV_EVENT_CLICKED, ctx);

    // Create keyboard for text input
    lv_obj_t *keyboard = lv_keyboard_create(lv_screen_active());
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_set_size(keyboard, lv_pct(100), lv_pct(50));
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    // Add focus event to textarea to show keyboard
    lv_obj_add_event_cb(ta, textarea_focus_handler, LV_EVENT_FOCUSED, keyboard);
    lv_obj_add_event_cb(ta, textarea_focus_handler, LV_EVENT_DEFOCUSED, keyboard);

    // Add keyboard events
    lv_obj_add_event_cb(keyboard, keyboard_ready_handler, LV_EVENT_READY, ctx);
    lv_obj_add_event_cb(keyboard, keyboard_cancel_handler, LV_EVENT_CANCEL, ctx);

    // Store keyboard in context
    ctx->keyboard = keyboard;

    // Style and position the dialog higher for M5Stack Tab5 touch screen to avoid keyboard overlap
    lv_obj_set_size(msgbox, 360, 220);
    lv_obj_align(msgbox, LV_ALIGN_CENTER, 0, -100); // Move up 100px to avoid keyboard
    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(msgbox, 2, 0);
    lv_obj_set_style_border_color(msgbox, THEME_PRIMARY_COLOR, 0);
    lv_obj_set_style_radius(msgbox, 10, 0);
}

void create_folder_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    ESP_LOGI(TAG, "Create folder dialog requested");

    // Create folder creation dialog
    lv_obj_t *msgbox = lv_msgbox_create(lv_screen_active());

    // Title
    lv_obj_t *title = lv_label_create(msgbox);
    lv_label_set_text(title, "Create New Folder");
    lv_obj_set_style_text_font(title, THEME_FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title, THEME_PRIMARY_COLOR, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);

    // Instruction text
    lv_obj_t *instruction = lv_label_create(msgbox);
    lv_label_set_text(instruction, "Enter folder name:");
    lv_obj_set_style_text_color(instruction, THEME_TEXT_COLOR, 0);
    lv_obj_align(instruction, LV_ALIGN_TOP_LEFT, 20, 50);

    // Text area for folder name input
    lv_obj_t *ta = lv_textarea_create(msgbox);
    lv_textarea_set_text(ta, "NewFolder");
    lv_textarea_set_one_line(ta, true);
    lv_obj_set_width(ta, 280);
    lv_obj_set_height(ta, 50);
    lv_obj_align(ta, LV_ALIGN_CENTER, 0, 0);

    // Style the text area for touch-friendly usage
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_color(ta, THEME_PRIMARY_COLOR, 0);
    lv_obj_set_style_border_width(ta, 2, 0);
    lv_obj_set_style_text_color(ta, THEME_TEXT_COLOR, 0);
    lv_obj_set_style_radius(ta, 5, 0);
    lv_obj_set_style_pad_all(ta, 8, 0);

    // Create button
    lv_obj_t *create_btn = lv_button_create(msgbox);
    lv_obj_set_size(create_btn, 100, 45);
    lv_obj_align(create_btn, LV_ALIGN_BOTTOM_LEFT, 30, -20);
    apply_button_style(create_btn);
    lv_obj_set_style_bg_color(create_btn, THEME_SUCCESS_COLOR, 0);
    lv_obj_set_style_text_color(create_btn, THEME_BG_COLOR, 0);

    lv_obj_t *create_label = lv_label_create(create_btn);
    lv_label_set_text(create_label, LV_SYMBOL_DIRECTORY " Create");
    lv_obj_center(create_label);

    // Cancel button
    lv_obj_t *cancel_btn = lv_button_create(msgbox);
    lv_obj_set_size(cancel_btn, 100, 45);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -30, -20);
    apply_button_style(cancel_btn);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0x666666), 0);

    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, LV_SYMBOL_CLOSE " Cancel");
    lv_obj_center(cancel_label);

    // Store context for the creation operation
    create_context_t *ctx = malloc(sizeof(create_context_t));
    ctx->msgbox = msgbox;
    ctx->textarea = ta;
    ctx->is_folder = true;

    lv_obj_add_event_cb(create_btn, create_confirm_handler, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(cancel_btn, create_cancel_handler, LV_EVENT_CLICKED, ctx);

    // Create keyboard for text input
    lv_obj_t *keyboard = lv_keyboard_create(lv_screen_active());
    lv_keyboard_set_textarea(keyboard, ta);
    lv_obj_set_size(keyboard, lv_pct(100), lv_pct(50));
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    // Add focus event to textarea to show keyboard
    lv_obj_add_event_cb(ta, textarea_focus_handler, LV_EVENT_FOCUSED, keyboard);
    lv_obj_add_event_cb(ta, textarea_focus_handler, LV_EVENT_DEFOCUSED, keyboard);

    // Add keyboard events
    lv_obj_add_event_cb(keyboard, keyboard_ready_handler, LV_EVENT_READY, ctx);
    lv_obj_add_event_cb(keyboard, keyboard_cancel_handler, LV_EVENT_CANCEL, ctx);

    // Store keyboard in context
    ctx->keyboard = keyboard;

    // Style and position the dialog higher for M5Stack Tab5 touch screen to avoid keyboard overlap
    lv_obj_set_size(msgbox, 360, 220);
    lv_obj_align(msgbox, LV_ALIGN_CENTER, 0, -100); // Move up 100px to avoid keyboard
    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(msgbox, 2, 0);
    lv_obj_set_style_border_color(msgbox, THEME_PRIMARY_COLOR, 0);
    lv_obj_set_style_radius(msgbox, 10, 0);
}

static void create_confirm_handler(lv_event_t *e) {
    create_context_t *ctx = (create_context_t*)lv_event_get_user_data(e);

    const char *name = lv_textarea_get_text(ctx->textarea);

    // Validate the name
    if (!file_ops_validate_filename(name)) {
        ESP_LOGW(TAG, "Invalid filename: %s", name);

        // Show error dialog
        lv_obj_t *error_msgbox = lv_msgbox_create(lv_screen_active());
        lv_obj_t *error_text = lv_label_create(error_msgbox);
        lv_label_set_text(error_text, "Invalid Name\n\nName contains forbidden characters\nor is too long. Please use only\nletters, numbers, and basic\npunctuation.");
        lv_obj_set_style_text_color(error_text, THEME_ERROR_COLOR, 0);
        lv_obj_set_style_text_align(error_text, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(error_text);

        lv_obj_t *ok_btn = lv_button_create(error_msgbox);
        lv_obj_set_size(ok_btn, 80, 35);
        lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -15);
        apply_button_style(ok_btn);
        lv_obj_add_event_cb(ok_btn, simple_dialog_close_handler, LV_EVENT_CLICKED, error_msgbox);

        lv_obj_t *ok_label = lv_label_create(ok_btn);
        lv_label_set_text(ok_label, "OK");
        lv_obj_center(ok_label);

        lv_obj_set_size(error_msgbox, 320, 180);
        lv_obj_center(error_msgbox);
        lv_obj_set_style_bg_color(error_msgbox, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_color(error_msgbox, THEME_ERROR_COLOR, 0);
        return;
    }

    // Build the full path - using larger buffer and length checks
    char full_path[1024];
    if (strcmp(current_directory, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "/%s", name);
    } else {
        // Check if path would be too long before formatting
        size_t dir_len = strlen(current_directory);
        size_t name_len = strlen(name);
        if (dir_len + name_len + 2 >= sizeof(full_path)) {
            ESP_LOGE(TAG, "Path too long: %s/%s", current_directory, name);
            lv_obj_del(ctx->msgbox);
            free(ctx);
            return;
        }
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(full_path, sizeof(full_path), "%s/%s", current_directory, name);
        #pragma GCC diagnostic pop
    }

    // Create the file or folder
    esp_err_t ret;
    if (ctx->is_folder) {
        ret = file_ops_create_directory(full_path);
    } else {
        ret = file_ops_create_file(full_path);
    }

    // Close the creation dialog and hide keyboard
    if (ctx->keyboard) {
        lv_obj_add_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del(ctx->keyboard);
    }
    lv_obj_del(ctx->msgbox);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Created %s: %s", ctx->is_folder ? "folder" : "file", full_path);

        // Show success message
        lv_obj_t *success_msgbox = lv_msgbox_create(lv_screen_active());
        lv_obj_t *success_text = lv_label_create(success_msgbox);
        char success_msg[256];
        snprintf(success_msg, sizeof(success_msg), "%s Created\n\n%s '%s' created successfully!",
                ctx->is_folder ? "Folder" : "File", ctx->is_folder ? "Folder" : "File", name);
        lv_label_set_text(success_text, success_msg);
        lv_obj_set_style_text_color(success_text, THEME_SUCCESS_COLOR, 0);
        lv_obj_set_style_text_align(success_text, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(success_text);

        lv_obj_t *ok_btn = lv_button_create(success_msgbox);
        lv_obj_set_size(ok_btn, 80, 35);
        lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -15);
        apply_button_style(ok_btn);
        lv_obj_add_event_cb(ok_btn, simple_dialog_close_handler, LV_EVENT_CLICKED, success_msgbox);

        lv_obj_t *ok_label = lv_label_create(ok_btn);
        lv_label_set_text(ok_label, "OK");
        lv_obj_center(ok_label);

        lv_obj_set_size(success_msgbox, 300, 150);
        lv_obj_center(success_msgbox);
        lv_obj_set_style_bg_color(success_msgbox, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_color(success_msgbox, THEME_SUCCESS_COLOR, 0);

        // CRITICAL FIX: Don't update file list immediately - causes stack corruption
        // The file list will be updated when the user closes this dialog or navigates away
    } else {
        ESP_LOGE(TAG, "Failed to create %s: %s", ctx->is_folder ? "folder" : "file", full_path);

        // Show error dialog
        lv_obj_t *error_msgbox = lv_msgbox_create(lv_screen_active());
        lv_obj_t *error_text = lv_label_create(error_msgbox);
        char error_msg[256];
        if (ret == ESP_ERR_INVALID_STATE) {
            snprintf(error_msg, sizeof(error_msg), "Creation Failed\n\n%s already exists or\nSD card error.",
                    ctx->is_folder ? "Folder" : "File");
        } else {
            snprintf(error_msg, sizeof(error_msg), "Creation Failed\n\nUnable to create %s.\nCheck SD card permissions.",
                    ctx->is_folder ? "folder" : "file");
        }
        lv_label_set_text(error_text, error_msg);
        lv_obj_set_style_text_color(error_text, THEME_ERROR_COLOR, 0);
        lv_obj_set_style_text_align(error_text, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(error_text);

        lv_obj_t *ok_btn = lv_button_create(error_msgbox);
        lv_obj_set_size(ok_btn, 80, 35);
        lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -15);
        apply_button_style(ok_btn);
        lv_obj_add_event_cb(ok_btn, simple_dialog_close_handler, LV_EVENT_CLICKED, error_msgbox);

        lv_obj_t *ok_label = lv_label_create(ok_btn);
        lv_label_set_text(ok_label, "OK");
        lv_obj_center(ok_label);

        lv_obj_set_size(error_msgbox, 300, 160);
        lv_obj_center(error_msgbox);
        lv_obj_set_style_bg_color(error_msgbox, lv_color_hex(0x1a1a1a), 0);
        lv_obj_set_style_border_color(error_msgbox, THEME_ERROR_COLOR, 0);
    }

    // Clean up
    free(ctx);
}

static void create_cancel_handler(lv_event_t *e) {
    create_context_t *ctx = (create_context_t*)lv_event_get_user_data(e);
    if (ctx) {
        ESP_LOGI(TAG, "Closing creation dialog with context cleanup");
        // Hide and delete keyboard if it exists
        if (ctx->keyboard && lv_obj_is_valid(ctx->keyboard)) {
            lv_obj_add_flag(ctx->keyboard, LV_OBJ_FLAG_HIDDEN);
            lv_obj_del(ctx->keyboard);
        }
        if (ctx->msgbox && lv_obj_is_valid(ctx->msgbox)) {
            lv_obj_del(ctx->msgbox);
        }
        free(ctx);
    }
}

static void simple_dialog_close_handler(lv_event_t *e) {
    lv_obj_t *msgbox = (lv_obj_t*)lv_event_get_user_data(e);
    if (msgbox && lv_obj_is_valid(msgbox)) {
        ESP_LOGI(TAG, "Closing simple dialog");
        lv_obj_del(msgbox);

        // Update file list after dialog is closed to show any new files
        // This is safe to do here since the dialog cleanup is complete
        if (lv_screen_active() == file_manager_screen) {
            ESP_LOGI(TAG, "Refreshing file list after dialog close");
            update_file_list();
        }
    }
}

// Keyboard event handlers for text input dialogs
static void textarea_focus_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *keyboard = (lv_obj_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        // Show keyboard when textarea is focused
        if (keyboard) {
            lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Keyboard shown for text input");
        }
    } else if (code == LV_EVENT_DEFOCUSED) {
        // Hide keyboard when textarea loses focus
        if (keyboard) {
            lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
            ESP_LOGI(TAG, "Keyboard hidden after losing focus");
        }
    }
}

static void keyboard_ready_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_READY) {
        // User pressed "OK" on keyboard - hide keyboard
        ESP_LOGI(TAG, "Keyboard OK pressed, hiding keyboard");

        // Hide the keyboard
        lv_obj_t *keyboard = lv_event_get_target(e);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

static void keyboard_cancel_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CANCEL) {
        // User pressed "Cancel" on keyboard - hide keyboard
        lv_obj_t *keyboard = lv_event_get_target(e);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(TAG, "Keyboard cancelled, hiding keyboard");
    }
}