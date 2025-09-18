#include "gui_screen_text_editor.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "gui_screen_python_launcher.h"
#include "sd_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "TEXT_EDITOR";

lv_obj_t *text_editor_screen = NULL;
static lv_obj_t *text_area = NULL;
static lv_obj_t *editor_status_label = NULL;
static lv_obj_t *keyboard = NULL;
static char current_file_path[256] = {0};
static bool file_modified = false;
static bool keyboard_visible = false;

// Forward declarations
static void text_editor_event_handler(lv_event_t *e);
static void keyboard_event_handler(lv_event_t *e);
static void save_button_event_handler(lv_event_t *e);
static void close_button_event_handler(lv_event_t *e);
static void keyboard_toggle_event_handler(lv_event_t *e);
static void python_launch_button_event_handler(lv_event_t *e);
static void toggle_keyboard(void);
static void update_status(const char *message);

text_file_type_t text_editor_get_file_type(const char *filename) {
    if (!filename) return TEXT_FILE_UNKNOWN;

    const char *ext = strrchr(filename, '.');
    if (!ext) return TEXT_FILE_UNKNOWN;

    if (strcmp(ext, ".txt") == 0) return TEXT_FILE_TXT;
    if (strcmp(ext, ".json") == 0) return TEXT_FILE_JSON;
    if (strcmp(ext, ".py") == 0) return TEXT_FILE_PY;
    if (strcmp(ext, ".html") == 0) return TEXT_FILE_HTML;
    if (strcmp(ext, ".css") == 0) return TEXT_FILE_CSS;
    if (strcmp(ext, ".js") == 0) return TEXT_FILE_JS;
    if (strcmp(ext, ".log") == 0) return TEXT_FILE_LOG;
    if (strcmp(ext, ".cfg") == 0 || strcmp(ext, ".conf") == 0) return TEXT_FILE_CONFIG;

    return TEXT_FILE_UNKNOWN;
}

bool text_editor_is_supported_file(const char *filename) {
    return text_editor_get_file_type(filename) != TEXT_FILE_UNKNOWN;
}

void create_text_editor_screen(void) {
    if (text_editor_screen) {
        return; // Already created
    }

    text_editor_screen = lv_obj_create(NULL);
    lv_obj_add_style(text_editor_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(text_editor_screen);
    lv_obj_set_size(title_bar, lv_pct(100), 50);
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_border_opa(title_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(title_bar, 5, 0);

    // Close button
    lv_obj_t *close_btn = lv_button_create(title_bar);
    lv_obj_set_size(close_btn, 40, 35);
    lv_obj_align(close_btn, LV_ALIGN_LEFT_MID, 5, 0);
    apply_button_style(close_btn);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_add_event_cb(close_btn, close_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_center(close_label);

    // Save button
    lv_obj_t *save_btn = lv_button_create(title_bar);
    lv_obj_set_size(save_btn, 40, 35);
    lv_obj_align(save_btn, LV_ALIGN_LEFT_MID, 50, 0);
    apply_button_style(save_btn);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x27ae60), 0);
    lv_obj_add_event_cb(save_btn, save_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, LV_SYMBOL_SAVE);
    lv_obj_center(save_label);

    // Keyboard toggle button
    lv_obj_t *kb_btn = lv_button_create(title_bar);
    lv_obj_set_size(kb_btn, 40, 35);
    lv_obj_align(kb_btn, LV_ALIGN_LEFT_MID, 95, 0);
    apply_button_style(kb_btn);
    lv_obj_set_style_bg_color(kb_btn, lv_color_hex(0x3498db), 0);
    lv_obj_add_event_cb(kb_btn, keyboard_toggle_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *kb_label = lv_label_create(kb_btn);
    lv_label_set_text(kb_label, LV_SYMBOL_KEYBOARD);
    lv_obj_center(kb_label);

    // Python launcher button (only show for .py files)
    lv_obj_t *py_btn = lv_button_create(title_bar);
    lv_obj_set_size(py_btn, 40, 35);
    lv_obj_align(py_btn, LV_ALIGN_LEFT_MID, 140, 0);
    apply_button_style(py_btn);
    lv_obj_set_style_bg_color(py_btn, lv_color_hex(0x9b59b6), 0);
    lv_obj_add_event_cb(py_btn, python_launch_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *py_label = lv_label_create(py_btn);
    lv_label_set_text(py_label, LV_SYMBOL_PLAY);
    lv_obj_center(py_label);

    // Title
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Text Editor");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_18, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // Status bar
    lv_obj_t *status_bar = lv_obj_create(text_editor_screen);
    lv_obj_set_size(status_bar, lv_pct(100), 30);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_border_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(status_bar, 5, 0);

    editor_status_label = lv_label_create(status_bar);
    lv_label_set_text(editor_status_label, "Ready");
    lv_obj_set_style_text_color(editor_status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(editor_status_label, &lv_font_montserrat_12, 0);
    lv_obj_align(editor_status_label, LV_ALIGN_LEFT_MID, 0, 0);

    // Text area (main editing area)
    text_area = lv_textarea_create(text_editor_screen);
    lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - 80); // Account for title and status bars
    lv_obj_align(text_area, LV_ALIGN_TOP_MID, 0, 50);

    // Style the text area
    lv_obj_set_style_bg_color(text_area, lv_color_hex(0x1e1e1e), 0);  // Dark background
    lv_obj_set_style_text_color(text_area, lv_color_hex(0xf8f8f2), 0); // Light text
    lv_obj_set_style_border_color(text_area, lv_color_hex(0x44475a), 0);
    lv_obj_set_style_border_width(text_area, 1, 0);
    lv_obj_set_style_pad_all(text_area, 10, 0);
    lv_obj_set_style_text_font(text_area, &lv_font_montserrat_14, 0);

    // Configure text area
    lv_textarea_set_placeholder_text(text_area, "Start typing or open a file...");
    lv_obj_add_event_cb(text_area, text_editor_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Create keyboard (initially hidden)
    keyboard = lv_keyboard_create(text_editor_screen);
    lv_obj_set_size(keyboard, lv_pct(100), 200);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(keyboard, text_area);
    lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_ALL, NULL);

    keyboard_visible = false;
    file_modified = false;

    ESP_LOGI(TAG, "Text editor screen created");
}

esp_err_t text_editor_open_file(const char *file_path) {
    if (!file_path || !text_area) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!sd_manager_is_mounted()) {
        update_status("SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    FILE *file = sd_manager_open_file(file_path, "r");
    if (!file) {
        update_status("Failed to open file");
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        return ESP_ERR_NOT_FOUND;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size > 32768) { // 32KB limit
        fclose(file);
        update_status("File too large (>32KB)");
        return ESP_ERR_INVALID_SIZE;
    }

    // Read file content
    char *content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        update_status("Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }

    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);

    // Set content in text area
    lv_textarea_set_text(text_area, content);
    free(content);

    // Update current file path
    strncpy(current_file_path, file_path, sizeof(current_file_path) - 1);
    current_file_path[sizeof(current_file_path) - 1] = '\0';

    // Update status with truncated filename to prevent buffer overflow
    char status_msg[256];
    const char *full_filename = strrchr(file_path, '/');
    full_filename = full_filename ? full_filename + 1 : file_path;

    char filename[50];  // Fixed size to prevent overflow
    strncpy(filename, full_filename, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    snprintf(status_msg, sizeof(status_msg), "Opened: %s (%ld bytes)", filename, file_size);
    update_status(status_msg);

    file_modified = false;
    ESP_LOGI(TAG, "File opened successfully: %s", file_path);
    return ESP_OK;
}

esp_err_t text_editor_save_file(void) {
    if (!text_area || strlen(current_file_path) == 0) {
        update_status("No file to save");
        return ESP_ERR_INVALID_STATE;
    }

    if (!sd_manager_is_mounted()) {
        update_status("SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    const char *content = lv_textarea_get_text(text_area);
    if (!content) {
        update_status("No content to save");
        return ESP_ERR_INVALID_ARG;
    }

    FILE *file = sd_manager_open_file(current_file_path, "w");
    if (!file) {
        update_status("Failed to open file for writing");
        ESP_LOGE(TAG, "Failed to open file for writing: %s", current_file_path);
        return ESP_ERR_NOT_FOUND;
    }

    size_t content_len = strlen(content);
    size_t written = fwrite(content, 1, content_len, file);
    fclose(file);

    if (written != content_len) {
        update_status("Write error");
        return ESP_FAIL;
    }

    // Update status with truncated filename to prevent buffer overflow
    char status_msg[256];
    const char *full_filename = strrchr(current_file_path, '/');
    full_filename = full_filename ? full_filename + 1 : current_file_path;

    char filename[50];  // Fixed size to prevent overflow
    strncpy(filename, full_filename, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    snprintf(status_msg, sizeof(status_msg), "Saved: %s (%zu bytes)", filename, content_len);
    update_status(status_msg);

    file_modified = false;
    ESP_LOGI(TAG, "File saved successfully: %s", current_file_path);
    return ESP_OK;
}

void text_editor_close(void) {
    if (file_modified) {
        // TODO: Add confirmation dialog for unsaved changes
        ESP_LOGW(TAG, "Closing editor with unsaved changes");
    }

    // Clear current file
    current_file_path[0] = '\0';
    file_modified = false;
    keyboard_visible = false;

    // Return to main screen
    lv_screen_load(main_screen);
    ESP_LOGI(TAG, "Text editor closed");
}

static void toggle_keyboard(void) {
    if (keyboard_visible) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - 80);
        keyboard_visible = false;
        update_status("Keyboard hidden");
    } else {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - 280); // Account for keyboard
        keyboard_visible = true;
        update_status("Keyboard shown");
    }
}

static void update_status(const char *message) {
    if (editor_status_label && message) {
        lv_label_set_text(editor_status_label, message);
    }
}

// Event handlers
static void text_editor_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        file_modified = true;
        update_status("Modified");
    }
}

static void keyboard_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        toggle_keyboard();
    }
}

static void save_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Save button clicked");
        esp_err_t ret = text_editor_save_file();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save file: %s", esp_err_to_name(ret));
        }
    }
}

static void close_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Close button clicked");
        text_editor_close();
    }
}

static void keyboard_toggle_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Keyboard toggle clicked");
        toggle_keyboard();
    }
}

static void python_launch_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Python launcher button clicked");

        // Check if current file is a Python file
        if (strlen(current_file_path) > 0 && python_launcher_is_supported_file(current_file_path)) {
            ESP_LOGI(TAG, "Opening current Python file in launcher: %s", current_file_path);
            show_python_launcher_screen();
        } else {
            ESP_LOGW(TAG, "Current file is not a Python file or no file loaded");
            update_status("Not a Python file");
        }
    }
}

void destroy_text_editor_screen(void) {
    if (text_editor_screen) {
        lv_obj_del(text_editor_screen);
        text_editor_screen = NULL;
        ESP_LOGI(TAG, "Text editor screen destroyed");
    }
}