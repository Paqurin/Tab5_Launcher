#include "gui_screen_python_launcher.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "sd_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "PYTHON_LAUNCHER";

lv_obj_t *python_launcher_screen = NULL;
static lv_obj_t *script_list = NULL;
static lv_obj_t *output_area = NULL;
static lv_obj_t *run_button = NULL;
static char selected_script_path[256] = {0};

// Forward declarations
static void py_back_button_event_handler(lv_event_t *e);
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
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
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

    // Create main container
    lv_obj_t *main_container = lv_obj_create(python_launcher_screen);
    lv_obj_set_size(main_container, lv_pct(100), lv_pct(100) - 60);
    lv_obj_align(main_container, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(main_container, 10, 0);

    // Create left panel for script list
    lv_obj_t *left_panel = lv_obj_create(main_container);
    lv_obj_set_size(left_panel, lv_pct(45), lv_pct(100));
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(left_panel, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_border_color(left_panel, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_border_width(left_panel, 1, 0);
    lv_obj_set_style_pad_all(left_panel, 10, 0);

    // Script list label
    lv_obj_t *list_label = lv_label_create(left_panel);
    lv_label_set_text(list_label, "Python Scripts:");
    lv_obj_set_style_text_color(list_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(list_label, &lv_font_montserrat_14, 0);
    lv_obj_align(list_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Create script list
    script_list = lv_list_create(left_panel);
    lv_obj_set_size(script_list, lv_pct(100), lv_pct(85));
    lv_obj_align(script_list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(script_list, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_text_color(script_list, lv_color_hex(0xFFFFFF), 0);

    // Create right panel for output
    lv_obj_t *right_panel = lv_obj_create(main_container);
    lv_obj_set_size(right_panel, lv_pct(50), lv_pct(100));
    lv_obj_align(right_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(right_panel, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_border_color(right_panel, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_border_width(right_panel, 1, 0);
    lv_obj_set_style_pad_all(right_panel, 10, 0);

    // Output label
    lv_obj_t *output_label = lv_label_create(right_panel);
    lv_label_set_text(output_label, "Output:");
    lv_obj_set_style_text_color(output_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(output_label, &lv_font_montserrat_14, 0);
    lv_obj_align(output_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Create output area
    output_area = lv_textarea_create(right_panel);
    lv_obj_set_size(output_area, lv_pct(100), lv_pct(85));
    lv_obj_align(output_area, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_textarea_set_text(output_area, "Ready to execute Python scripts...\n\nNote: This is a simplified Python launcher.\nFull Python interpreter integration would\nrequire MicroPython or PyScript integration.");
    lv_obj_set_style_bg_color(output_area, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_text_color(output_area, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(output_area, &lv_font_montserrat_12, 0);
    lv_textarea_set_cursor_click_pos(output_area, false);

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
        return;
    }

    // TODO: Scan for .py files in current directory
    // For now, add some example entries
    lv_obj_t *item1 = lv_list_add_button(script_list, LV_SYMBOL_FILE, "example.py");
    lv_obj_add_event_cb(item1, script_selection_event_handler, LV_EVENT_CLICKED, (void*)"example.py");

    lv_obj_t *item2 = lv_list_add_button(script_list, LV_SYMBOL_FILE, "test_script.py");
    lv_obj_add_event_cb(item2, script_selection_event_handler, LV_EVENT_CLICKED, (void*)"test_script.py");

    lv_obj_t *info_item = lv_list_add_text(script_list, "Scan SD card for .py files");
    lv_obj_set_style_text_color(info_item, lv_color_hex(0xFFFFFF), 0);
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
    update_output("Starting script execution...");

    // TODO: Implement actual Python script execution
    // This would require integrating MicroPython or a similar interpreter

    update_output("ERROR: Python interpreter not available");
    update_output("This feature requires MicroPython integration");
    update_output("Script execution completed with error");

    return ESP_ERR_NOT_SUPPORTED;
}

void show_python_launcher_screen(void) {
    if (!python_launcher_screen) {
        create_python_launcher_screen();
    }
    update_script_list();
    lv_screen_load(python_launcher_screen);
}

void python_launcher_screen_back(void) {
    ESP_LOGI(TAG, "Returning to tools screen");
    lv_screen_load(tools_screen);
}

// Event handlers
static void py_back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        python_launcher_screen_back();
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
        const char *script_name = (const char*)lv_event_get_user_data(e);
        if (script_name) {
            strncpy(selected_script_path, script_name, sizeof(selected_script_path) - 1);
            selected_script_path[sizeof(selected_script_path) - 1] = '\0';

            ESP_LOGI(TAG, "Selected script: %s", selected_script_path);

            // Enable run button
            lv_obj_clear_state(run_button, LV_STATE_DISABLED);

            // Update output
            char msg[128];
            snprintf(msg, sizeof(msg), "Selected: %s", script_name);
            update_output(msg);
        }
    }
}