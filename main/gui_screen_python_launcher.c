#include "gui_screen_python_launcher.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "gui_screen_tools.h"
#include "sd_manager.h"
#include "python_engine.h"
#include "file_operations.h"
#include "screenshot_util.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

static const char *TAG = "PYTHON_LAUNCHER";

lv_obj_t *python_launcher_screen = NULL;
static lv_obj_t *script_list = NULL;
static lv_obj_t *output_area = NULL;
static lv_obj_t *run_button = NULL;
static char selected_script_path[256] = {0};

// Forward declarations
static void py_back_button_event_handler(lv_event_t *e);
static void screenshot_button_event_handler(lv_event_t *e);
static void run_button_event_handler(lv_event_t *e);
static void script_selection_event_handler(lv_event_t *e);
static void update_script_list(void);
static void update_output(const char *message);

bool python_launcher_is_supported_file(const char *filename) {
    if (!filename) return false;

    const char *ext = strrchr(filename, '.');
    if (!ext) return false;

    return (strcmp(ext, ".py") == 0 || strcmp(ext, ".pyw") == 0);
}

void create_python_launcher_screen(void) {
    if (python_launcher_screen) {
        return; // Already created
    }

    python_launcher_screen = lv_obj_create(NULL);
    lv_obj_add_style(python_launcher_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(python_launcher_screen);
    lv_obj_set_size(title_bar, lv_pct(100), 60);
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_opa(title_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(title_bar, 10, 0);

    // Back button
    lv_obj_t *back_btn = lv_button_create(title_bar);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, py_back_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    // Screenshot button (next to back button)
    lv_obj_t *screenshot_btn = lv_button_create(title_bar);
    lv_obj_set_size(screenshot_btn, 60, 40);
    lv_obj_align(screenshot_btn, LV_ALIGN_LEFT_MID, 70, 0);  // 70px from left (after back button + gap)
    apply_button_style(screenshot_btn);
    lv_obj_set_style_bg_color(screenshot_btn, lv_color_hex(0xe74c3c), 0);  // Red color
    lv_obj_add_event_cb(screenshot_btn, screenshot_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *screenshot_label = lv_label_create(screenshot_btn);
    lv_label_set_text(screenshot_label, LV_SYMBOL_IMAGE);
    lv_obj_center(screenshot_label);

    // Title
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Python Launcher");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // Run button
    run_button = lv_button_create(title_bar);
    lv_obj_set_size(run_button, 60, 40);
    lv_obj_align(run_button, LV_ALIGN_RIGHT_MID, 0, 0);
    apply_button_style(run_button);
    lv_obj_set_style_bg_color(run_button, lv_color_hex(0x27ae60), 0);
    lv_obj_add_event_cb(run_button, run_button_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(run_button, LV_STATE_DISABLED); // Disabled until script selected

    lv_obj_t *run_label = lv_label_create(run_button);
    lv_label_set_text(run_label, LV_SYMBOL_PLAY);
    lv_obj_center(run_label);

    // Create script list - left side (Tab5 screen is 800x480, title bar is 60px)
    script_list = lv_list_create(python_launcher_screen);
    lv_obj_set_size(script_list, 390, 410);  // Half screen width (390px), full remaining height (410px)
    lv_obj_set_pos(script_list, 5, 65);  // Start just below title bar
    lv_obj_set_style_bg_color(script_list, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(script_list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(script_list, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_border_width(script_list, 1, 0);

    // Create output area - right side
    output_area = lv_textarea_create(python_launcher_screen);
    lv_obj_set_size(output_area, 390, 410);  // Half screen width (390px), full remaining height (410px)
    lv_obj_set_pos(output_area, 405, 65);  // Position to right of script list with small gap
    lv_textarea_set_text(output_area, "Python Engine Ready\n\nMicroPython integration in progress...\nSelect a .py script to execute.\n\nFeatures:\n- Script execution with output capture\n- REPL console support\n- Error handling and debugging\n- Memory management for embedded environment");
    lv_obj_set_style_bg_color(output_area, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_text_color(output_area, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(output_area, &lv_font_montserrat_16, 0);
    lv_obj_set_style_border_color(output_area, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_border_width(output_area, 1, 0);
    lv_textarea_set_cursor_click_pos(output_area, false);

    // Initialize Python engine
    esp_err_t ret = python_engine_init();
    if (ret == ESP_OK) {
        update_output("Python engine initialized successfully");
    } else {
        update_output("Failed to initialize Python engine");
    }

    // Update script list
    update_script_list();

    ESP_LOGI(TAG, "Python launcher screen created");
}

static void update_script_list(void) {
    if (!script_list) return;

    // Clear existing items
    lv_obj_clean(script_list);

    if (!sd_manager_is_mounted()) {
        lv_obj_t *item = lv_list_add_text(script_list, "SD card not mounted");
        lv_obj_set_style_text_color(item, lv_color_hex(0xFF6B6B), 0);
        lv_obj_set_style_bg_color(item, lv_color_hex(0x000000), 0);
        return;
    }

    // Scan for .py files in SD card root directory
    const char *scan_path = "/sdcard";
    DIR *dir = opendir(scan_path);
    if (!dir) {
        lv_obj_t *item = lv_list_add_text(script_list, "Cannot access SD card");
        lv_obj_set_style_text_color(item, lv_color_hex(0xFF6B6B), 0);
        lv_obj_set_style_bg_color(item, lv_color_hex(0x000000), 0);
        return;
    }

    int python_files_found = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Regular file
            if (python_engine_is_supported_file(entry->d_name)) {
                // Create file path for user data
                char *file_path = malloc(strlen(scan_path) + strlen(entry->d_name) + 2);
                if (file_path) {
                    sprintf(file_path, "%s/%s", scan_path, entry->d_name);

                    lv_obj_t *item = lv_list_add_button(script_list, LV_SYMBOL_FILE, entry->d_name);
                    lv_obj_add_event_cb(item, script_selection_event_handler, LV_EVENT_CLICKED, file_path);
                    lv_obj_set_style_text_color(item, lv_color_hex(0x3498db), 0);
                    lv_obj_set_style_bg_color(item, lv_color_hex(0x000000), 0);  // Black background for file items
                    python_files_found++;
                }
            }
        }
    }
    closedir(dir);

    if (python_files_found == 0) {
        lv_obj_t *item = lv_list_add_text(script_list, "No Python files found");
        lv_obj_set_style_text_color(item, lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_bg_color(item, lv_color_hex(0x000000), 0);

        lv_obj_t *help_item = lv_list_add_text(script_list, "Place .py files on SD card");
        lv_obj_set_style_text_color(help_item, lv_color_hex(0xBDBDBD), 0);
        lv_obj_set_style_bg_color(help_item, lv_color_hex(0x000000), 0);
    } else {
        char count_msg[64];
        snprintf(count_msg, sizeof(count_msg), "Found %d Python files", python_files_found);
        lv_obj_t *count_item = lv_list_add_text(script_list, count_msg);
        lv_obj_set_style_text_color(count_item, lv_color_hex(0x27ae60), 0);
        lv_obj_set_style_bg_color(count_item, lv_color_hex(0x000000), 0);
    }
}

static void update_output(const char *message) {
    if (!output_area || !message) return;

    const char *current_text = lv_textarea_get_text(output_area);
    char new_text[2048];
    snprintf(new_text, sizeof(new_text), "%s\n%s", current_text, message);
    lv_textarea_set_text(output_area, new_text);

    // Scroll to bottom
    lv_obj_scroll_to_y(output_area, LV_COORD_MAX, LV_ANIM_ON);
}

esp_err_t python_launcher_execute_script(const char *script_path) {
    if (!script_path) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Executing Python script: %s", script_path);

    char exec_msg[128];
    snprintf(exec_msg, sizeof(exec_msg), "Executing: %s", script_path);
    update_output(exec_msg);
    update_output("=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=");

    // Check engine status
    python_engine_status_t status = python_engine_get_status();
    if (status == PYTHON_ENGINE_RUNNING) {
        update_output("ERROR: Python engine is busy");
        return ESP_ERR_INVALID_STATE;
    }

    // Execute the script using our Python engine
    python_execution_result_t result;
    esp_err_t ret = python_engine_execute_file(script_path, &result);

    if (ret == ESP_OK) {
        if (result.success) {
            update_output("SCRIPT OUTPUT:");
            update_output("---------------");
            if (result.output && result.output_len > 0) {
                update_output(result.output);
            } else {
                update_output("(no output)");
            }
            update_output("---------------");
            update_output("Script executed successfully");
        } else {
            update_output("SCRIPT ERROR:");
            update_output("-------------");
            if (result.error_message && result.error_len > 0) {
                update_output(result.error_message);
            } else {
                update_output("Unknown error occurred");
            }
            update_output("-------------");
            update_output("Script execution failed");
        }
    } else {
        update_output("ENGINE ERROR: Failed to execute script");
        char err_msg[64];
        snprintf(err_msg, sizeof(err_msg), "Error code: 0x%x", ret);
        update_output(err_msg);
    }

    // Show memory usage
    size_t available_heap = python_engine_get_available_heap();
    char mem_msg[64];
    snprintf(mem_msg, sizeof(mem_msg), "Available Python heap: %zu bytes", available_heap);
    update_output(mem_msg);

    // Clean up result
    python_engine_free_result(&result);

    return ret;
}

void show_python_launcher_screen(void) {
    // CRITICAL FIX: Clean up file manager screen before switching to prevent LVGL corruption
    extern lv_obj_t *file_manager_screen;
    extern void destroy_file_manager_screen(void);

    if (file_manager_screen) {
        ESP_LOGI(TAG, "Cleaning up file manager screen before Python launcher");
        // Simply call the destruction function - it already has all the defensive checks
        destroy_file_manager_screen();
    }

    if (!python_launcher_screen) {
        create_python_launcher_screen();
    }
    update_script_list();
    lv_screen_load(python_launcher_screen);
}

void python_launcher_screen_back(void) {
    ESP_LOGI(TAG, "Returning to tools screen");

    // CRITICAL FIX: Load new screen BEFORE destroying current screen to prevent NULL active screen
    lv_screen_load(tools_screen);

    // Now safely destroy Python launcher screen AFTER new screen is active
    destroy_python_launcher_screen();
}

void destroy_python_launcher_screen(void) {
    if (python_launcher_screen) {
        ESP_LOGI(TAG, "Destroying Python launcher screen...");

        // DEFENSIVE: Check screen validity before destruction
        if (!lv_obj_is_valid(python_launcher_screen)) {
            ESP_LOGW(TAG, "Python launcher screen is invalid, setting to NULL");
            python_launcher_screen = NULL;
            script_list = NULL;
            output_area = NULL;
            run_button = NULL;
            return;
        }

        // DEFENSIVE: Check if screen is currently active
        if (lv_screen_active() == python_launcher_screen) {
            ESP_LOGW(TAG, "Destroying active screen - ensuring safe deletion");
        }

        // Deinitialize Python engine first
        python_engine_deinit();

        // Disable events on screen to prevent corruption during deletion
        lv_obj_remove_event_cb(python_launcher_screen, NULL);

        // Remove all event callbacks from child objects to prevent dangling pointers
        uint32_t child_cnt = lv_obj_get_child_count(python_launcher_screen);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(python_launcher_screen, i);
            if (child && lv_obj_is_valid(child)) {
                lv_obj_remove_event_cb(child, NULL);
            }
        }

        // DEFENSIVE: Final validation before deletion
        if (lv_obj_is_valid(python_launcher_screen)) {
            lv_obj_del(python_launcher_screen);
            ESP_LOGI(TAG, "Python launcher screen destroyed");
        } else {
            ESP_LOGW(TAG, "Python launcher screen became invalid during cleanup");
        }

        python_launcher_screen = NULL;
        script_list = NULL;
        output_area = NULL;
        run_button = NULL;
    }
}

// Event handlers
static void py_back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        python_launcher_screen_back();
    }
}

static void screenshot_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Taking screenshot...");
        esp_err_t ret = screenshot_take_and_save();
        if (ret == ESP_OK) {
            update_output("Screenshot saved to SD card");
        } else {
            update_output("Failed to save screenshot");
        }
    }
}

static void run_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (strlen(selected_script_path) > 0) {
            ESP_LOGI(TAG, "Running script: %s", selected_script_path);
            python_launcher_execute_script(selected_script_path);
        }
    }
}

static void script_selection_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        const char *script_path = (const char*)lv_event_get_user_data(e);
        if (script_path) {
            strncpy(selected_script_path, script_path, sizeof(selected_script_path) - 1);
            selected_script_path[sizeof(selected_script_path) - 1] = '\0';

            ESP_LOGI(TAG, "Selected script: %s", selected_script_path);

            // Enable run button
            lv_obj_clear_state(run_button, LV_STATE_DISABLED);

            // Update output - extract filename for display
            const char *filename = strrchr(script_path, '/');
            if (filename) {
                filename++; // Skip the '/'
            } else {
                filename = script_path;
            }

            char msg[128];
            snprintf(msg, sizeof(msg), "Selected: %s", filename);
            update_output(msg);

            // Show file size info
            FILE *file = fopen(script_path, "r");
            if (file) {
                fseek(file, 0, SEEK_END);
                size_t file_size = ftell(file);
                fclose(file);

                char size_msg[64];
                snprintf(size_msg, sizeof(size_msg), "File size: %zu bytes", file_size);
                update_output(size_msg);
            }
        }
    }
}