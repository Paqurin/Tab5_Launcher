#include "gui_screen_text_editor.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "gui_screen_python_launcher.h"
#include "syntax_highlighter.h"
#include "sd_manager.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_memory_utils.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "TEXT_EDITOR";

// File size limits and thresholds
#define MAX_FILE_SIZE_BYTES     (1024 * 1024)  // 1MB max file size
#define LARGE_FILE_THRESHOLD    (256 * 1024)   // 256KB - warn user
#define HUGE_FILE_THRESHOLD     (512 * 1024)   // 512KB - show detailed warning
#define OLD_MAX_FILE_SIZE       (32 * 1024)    // Previous 32KB limit

// Memory allocation preferences
#define PREFER_PSRAM_SIZE       (64 * 1024)    // Use PSRAM for buffers > 64KB

lv_obj_t *text_editor_screen = NULL;
static lv_obj_t *text_area = NULL;
static lv_obj_t *editor_status_label = NULL;
static lv_obj_t *keyboard = NULL;
static lv_obj_t *search_toolbar = NULL;
static lv_obj_t *search_input = NULL;
static lv_obj_t *search_results_label = NULL;
static lv_obj_t *syntax_highlight_btn = NULL;
static char current_file_path[256] = {0};
static bool file_modified = false;
static bool keyboard_visible = false;
static bool search_visible = false;
static int current_search_match = -1;
static int total_search_matches = 0;
static char last_search_term[64] = {0};
static bool syntax_highlighting_enabled = true;
static text_file_type_t current_file_type = TEXT_FILE_UNKNOWN;
static size_t current_file_size = 0;
static bool is_large_file = false;
static bool is_read_only = false;

// Forward declarations
static void text_editor_event_handler(lv_event_t *e);
static void keyboard_event_handler(lv_event_t *e);
static void save_button_event_handler(lv_event_t *e);
static void close_button_event_handler(lv_event_t *e);
static void keyboard_toggle_event_handler(lv_event_t *e);
static void python_launch_button_event_handler(lv_event_t *e);
static void search_button_event_handler(lv_event_t *e);
static void search_input_event_handler(lv_event_t *e);
static void search_next_event_handler(lv_event_t *e);
static void search_prev_event_handler(lv_event_t *e);
static void search_close_event_handler(lv_event_t *e);
static void syntax_highlighting_button_event_handler(lv_event_t *e);
static void toggle_keyboard(void);
static void update_status(const char *message);
static void create_search_toolbar(void);
static void toggle_search_toolbar(void);
static void update_search_results(void);
static void highlight_search_match(int match_index);
static int find_all_matches(const char *search_term);
static void clear_search_highlights(void);
static void apply_syntax_highlighting_internal(void);
static void update_syntax_highlighting_button_color(void);
static char* allocate_file_buffer(size_t size);
static void free_file_buffer(char* buffer);
static bool show_large_file_warning(size_t file_size);
static void update_status_with_file_info(const char* filename, size_t file_size, const char* file_type);
static esp_err_t load_file_progressive(FILE* file, size_t file_size, char** content);

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

    // Disable scrolling on the main screen - only text area should scroll
    lv_obj_clear_flag(text_editor_screen, LV_OBJ_FLAG_SCROLLABLE);

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

    // Search button
    lv_obj_t *search_btn = lv_button_create(title_bar);
    lv_obj_set_size(search_btn, 40, 35);
    lv_obj_align(search_btn, LV_ALIGN_LEFT_MID, 95, 0);
    apply_button_style(search_btn);
    lv_obj_set_style_bg_color(search_btn, lv_color_hex(0xf39c12), 0);
    lv_obj_add_event_cb(search_btn, search_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *search_label = lv_label_create(search_btn);
    lv_label_set_text(search_label, LV_SYMBOL_EYE_OPEN);
    lv_obj_center(search_label);

    // Keyboard toggle button
    lv_obj_t *kb_btn = lv_button_create(title_bar);
    lv_obj_set_size(kb_btn, 40, 35);
    lv_obj_align(kb_btn, LV_ALIGN_LEFT_MID, 140, 0);
    apply_button_style(kb_btn);
    lv_obj_set_style_bg_color(kb_btn, lv_color_hex(0x3498db), 0);
    lv_obj_add_event_cb(kb_btn, keyboard_toggle_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *kb_label = lv_label_create(kb_btn);
    lv_label_set_text(kb_label, LV_SYMBOL_KEYBOARD);
    lv_obj_center(kb_label);

    // Syntax highlighting toggle button
    syntax_highlight_btn = lv_button_create(title_bar);
    lv_obj_set_size(syntax_highlight_btn, 40, 35);
    lv_obj_align(syntax_highlight_btn, LV_ALIGN_LEFT_MID, 185, 0);
    apply_button_style(syntax_highlight_btn);
    lv_obj_set_style_bg_color(syntax_highlight_btn, lv_color_hex(0xe67e22), 0); // Orange for syntax highlighting
    lv_obj_add_event_cb(syntax_highlight_btn, syntax_highlighting_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *syntax_label = lv_label_create(syntax_highlight_btn);
    lv_label_set_text(syntax_label, LV_SYMBOL_EDIT);
    lv_obj_center(syntax_label);

    // Python launcher button (only show for .py files)
    lv_obj_t *py_btn = lv_button_create(title_bar);
    lv_obj_set_size(py_btn, 40, 35);
    lv_obj_align(py_btn, LV_ALIGN_LEFT_MID, 230, 0);
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

    // Status bar (made taller to accommodate memory info)
    lv_obj_t *status_bar = lv_obj_create(text_editor_screen);
    lv_obj_set_size(status_bar, lv_pct(100), 35);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_border_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(status_bar, 5, 0);

    editor_status_label = lv_label_create(status_bar);
    lv_label_set_text(editor_status_label, "Ready | Mem: Checking...");
    lv_obj_set_style_text_color(editor_status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(editor_status_label, &lv_font_montserrat_12, 0);
    lv_obj_align(editor_status_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_long_mode(editor_status_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(editor_status_label, lv_pct(100));

    // Create search toolbar (initially hidden)
    create_search_toolbar();

    // Text area (main editing area) - adjusted for larger status bar
    text_area = lv_textarea_create(text_editor_screen);
    lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - 85); // Account for title and larger status bars
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

    // Ensure text area scrolling works properly and doesn't affect parent
    lv_obj_set_scroll_dir(text_area, LV_DIR_VER);

    // Create keyboard (initially hidden)
    keyboard = lv_keyboard_create(text_editor_screen);
    lv_obj_set_size(keyboard, lv_pct(100), 200);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(keyboard, text_area);
    lv_obj_add_event_cb(keyboard, keyboard_event_handler, LV_EVENT_ALL, NULL);

    keyboard_visible = false;
    search_visible = false;
    file_modified = false;
    current_search_match = -1;
    total_search_matches = 0;
    last_search_term[0] = '\0';
    syntax_highlighting_enabled = true;
    current_file_type = TEXT_FILE_UNKNOWN;

    // Initialize syntax highlighter
    if (!syntax_highlighter_init()) {
        ESP_LOGW(TAG, "Failed to initialize syntax highlighter");
        syntax_highlighting_enabled = false;
    }

    update_syntax_highlighting_button_color();

    // Show initial memory status
    size_t internal_free, spiram_free;
    text_editor_get_memory_info(&internal_free, &spiram_free);

    char initial_status[128];
    snprintf(initial_status, sizeof(initial_status),
            "Ready | Mem: RAM %.1fKB, PSRAM %.1fKB",
            (float)internal_free / 1024,
            (float)spiram_free / 1024);

    lv_label_set_text(editor_status_label, initial_status);

    ESP_LOGI(TAG, "Text editor screen created with memory: RAM %zu KB, PSRAM %zu KB",
            internal_free / 1024, spiram_free / 1024);
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

    // Check file size limits
    if (file_size > MAX_FILE_SIZE_BYTES) {
        fclose(file);
        char size_str[32];
        if (file_size > 1024 * 1024) {
            snprintf(size_str, sizeof(size_str), "%.1fMB", (float)file_size / (1024 * 1024));
        } else {
            snprintf(size_str, sizeof(size_str), "%.1fKB", (float)file_size / 1024);
        }
        char msg[128];
        snprintf(msg, sizeof(msg), "File too large (%s, max 1MB)", size_str);
        update_status(msg);
        return ESP_ERR_INVALID_SIZE;
    }

    // Show warning for large files
    if (file_size > LARGE_FILE_THRESHOLD) {
        if (!show_large_file_warning(file_size)) {
            fclose(file);
            update_status("File loading cancelled");
            return ESP_ERR_INVALID_STATE;
        }
    }

    // Read file content with progressive loading for large files
    char *content = NULL;
    esp_err_t load_result = load_file_progressive(file, file_size, &content);
    fclose(file);

    if (load_result != ESP_OK) {
        if (content) {
            free_file_buffer(content);
        }
        return load_result;
    }

    // Set content in text area
    lv_textarea_set_text(text_area, content);
    free_file_buffer(content);

    // Store file information
    current_file_size = file_size;
    is_large_file = (file_size > LARGE_FILE_THRESHOLD);
    is_read_only = false; // Reset read-only status

    // Update current file path
    strncpy(current_file_path, file_path, sizeof(current_file_path) - 1);
    current_file_path[sizeof(current_file_path) - 1] = '\0';

    // Detect file type and setup syntax highlighting
    current_file_type = text_editor_get_file_type(file_path);
    syntax_highlighter_set_file_type(current_file_type);

    // Apply syntax highlighting if enabled
    if (syntax_highlighting_enabled) {
        apply_syntax_highlighting_internal();
    }

    // Update status with file information
    const char *full_filename = strrchr(file_path, '/');
    full_filename = full_filename ? full_filename + 1 : file_path;

    char filename[50];  // Fixed size to prevent overflow
    strncpy(filename, full_filename, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    const char *file_type_name = syntax_highlighter_get_file_type_name(current_file_type);
    update_status_with_file_info(filename, file_size, file_type_name);

    file_modified = false;
    ESP_LOGI(TAG, "File opened successfully: %s (type: %s)", file_path, file_type_name);
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

    // Update status with file information and memory usage
    char status_msg[256];
    const char *full_filename = strrchr(current_file_path, '/');
    full_filename = full_filename ? full_filename + 1 : current_file_path;

    char filename[50];  // Fixed size to prevent overflow
    strncpy(filename, full_filename, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    char size_str[32];
    if (content_len >= 1024) {
        snprintf(size_str, sizeof(size_str), "%.1fKB", (float)content_len / 1024);
    } else {
        snprintf(size_str, sizeof(size_str), "%zu bytes", content_len);
    }

    snprintf(status_msg, sizeof(status_msg), "Saved: %s (%s)", filename, size_str);
    update_status(status_msg);

    // Update stored file size
    current_file_size = content_len;
    is_large_file = (content_len > LARGE_FILE_THRESHOLD);

    file_modified = false;
    ESP_LOGI(TAG, "File saved successfully: %s", current_file_path);
    return ESP_OK;
}

void text_editor_close(void) {
    if (file_modified) {
        // TODO: Add confirmation dialog for unsaved changes
        ESP_LOGW(TAG, "Closing editor with unsaved changes");
    }

    // Clear syntax highlighting
    if (text_area) {
        syntax_highlighter_clear(text_area);
    }

    // Clear current file
    current_file_path[0] = '\0';
    file_modified = false;
    keyboard_visible = false;
    current_file_type = TEXT_FILE_UNKNOWN;
    current_file_size = 0;
    is_large_file = false;
    is_read_only = false;

    // Reset search state
    search_visible = false;
    current_search_match = -1;
    total_search_matches = 0;
    last_search_term[0] = '\0';

    // Return to main screen
    lv_screen_load(main_screen);
    ESP_LOGI(TAG, "Text editor closed");
}

static void toggle_keyboard(void) {
    int base_height = search_visible ? 130 : 85; // Adjusted for larger status bar

    if (keyboard_visible) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - base_height);
        keyboard_visible = false;
        update_status("Keyboard hidden");
    } else {
        lv_obj_clear_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - (base_height + 200)); // Account for keyboard
        keyboard_visible = true;
        update_status("Keyboard shown");
    }
}

static void update_status(const char *message) {
    if (editor_status_label && message) {
        // For large files, append memory usage information
        if (is_large_file || current_file_size > OLD_MAX_FILE_SIZE) {
            size_t internal_free, spiram_free;
            text_editor_get_memory_info(&internal_free, &spiram_free);

            char enhanced_status[512];
            snprintf(enhanced_status, sizeof(enhanced_status),
                    "%s | Mem: RAM %.1fKB, PSRAM %.1fKB",
                    message,
                    (float)internal_free / 1024,
                    (float)spiram_free / 1024);
            lv_label_set_text(editor_status_label, enhanced_status);
        } else {
            // For smaller files, show basic memory info
            size_t internal_free, spiram_free;
            text_editor_get_memory_info(&internal_free, &spiram_free);

            char enhanced_status[256];
            snprintf(enhanced_status, sizeof(enhanced_status),
                    "%s | Mem: %.1fMB",
                    message,
                    (float)(internal_free + spiram_free) / (1024 * 1024));
            lv_label_set_text(editor_status_label, enhanced_status);
        }
    }
}

// Event handlers
static void text_editor_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        file_modified = true;
        update_status("Modified");

        // Reapply syntax highlighting with performance considerations for large files
        if (syntax_highlighting_enabled) {
            // For large files, debounce syntax highlighting to avoid performance issues
            if (is_large_file) {
                // In a full implementation, we would use a timer to debounce this
                ESP_LOGD(TAG, "Deferring syntax highlighting for large file modification");
            } else {
                apply_syntax_highlighting_internal();
            }
        }
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

static void search_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Search button clicked");
        text_editor_toggle_search();
    }
}

static void search_input_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        const char *search_term = lv_textarea_get_text(search_input);
        if (search_term && strlen(search_term) > 0) {
            strncpy(last_search_term, search_term, sizeof(last_search_term) - 1);
            last_search_term[sizeof(last_search_term) - 1] = '\0';

            total_search_matches = find_all_matches(search_term);
            if (total_search_matches > 0) {
                current_search_match = 0;
                highlight_search_match(current_search_match);
            } else {
                current_search_match = -1;
            }
            update_search_results();
        } else {
            clear_search_highlights();
            current_search_match = -1;
            total_search_matches = 0;
            update_search_results();
        }
    }
}

static void search_next_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Search next clicked");
        text_editor_search(last_search_term, true);
    }
}

static void search_prev_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Search previous clicked");
        text_editor_search(last_search_term, false);
    }
}

static void search_close_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Search close clicked");
        text_editor_toggle_search();
    }
}

static void syntax_highlighting_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Syntax highlighting toggle clicked");
        text_editor_toggle_syntax_highlighting();
    }
}

static void create_search_toolbar(void) {
    search_toolbar = lv_obj_create(text_editor_screen);
    lv_obj_set_size(search_toolbar, lv_pct(100), 45);
    lv_obj_align(search_toolbar, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_color(search_toolbar, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_border_color(search_toolbar, lv_color_hex(0x7f8c8d), 0);
    lv_obj_set_style_border_width(search_toolbar, 1, 0);
    lv_obj_set_style_pad_all(search_toolbar, 5, 0);
    lv_obj_add_flag(search_toolbar, LV_OBJ_FLAG_HIDDEN);

    // Search input field
    search_input = lv_textarea_create(search_toolbar);
    lv_obj_set_size(search_input, 200, 35);
    lv_obj_align(search_input, LV_ALIGN_LEFT_MID, 5, 0);
    lv_textarea_set_placeholder_text(search_input, "Search...");
    lv_textarea_set_one_line(search_input, true);
    lv_obj_set_style_bg_color(search_input, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_text_color(search_input, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_color(search_input, lv_color_hex(0x3498db), 0);
    lv_obj_add_event_cb(search_input, search_input_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Previous button
    lv_obj_t *prev_btn = lv_button_create(search_toolbar);
    lv_obj_set_size(prev_btn, 35, 35);
    lv_obj_align(prev_btn, LV_ALIGN_LEFT_MID, 210, 0);
    apply_button_style(prev_btn);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x95a5a6), 0);
    lv_obj_add_event_cb(prev_btn, search_prev_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *prev_label = lv_label_create(prev_btn);
    lv_label_set_text(prev_label, LV_SYMBOL_UP);
    lv_obj_center(prev_label);

    // Next button
    lv_obj_t *next_btn = lv_button_create(search_toolbar);
    lv_obj_set_size(next_btn, 35, 35);
    lv_obj_align(next_btn, LV_ALIGN_LEFT_MID, 250, 0);
    apply_button_style(next_btn);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x95a5a6), 0);
    lv_obj_add_event_cb(next_btn, search_next_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next_label = lv_label_create(next_btn);
    lv_label_set_text(next_label, LV_SYMBOL_DOWN);
    lv_obj_center(next_label);

    // Results counter
    search_results_label = lv_label_create(search_toolbar);
    lv_label_set_text(search_results_label, "0/0");
    lv_obj_set_style_text_color(search_results_label, lv_color_hex(0xecf0f1), 0);
    lv_obj_set_style_text_font(search_results_label, &lv_font_montserrat_12, 0);
    lv_obj_align(search_results_label, LV_ALIGN_LEFT_MID, 290, 0);

    // Close button
    lv_obj_t *close_btn = lv_button_create(search_toolbar);
    lv_obj_set_size(close_btn, 35, 35);
    lv_obj_align(close_btn, LV_ALIGN_RIGHT_MID, -5, 0);
    apply_button_style(close_btn);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_add_event_cb(close_btn, search_close_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_center(close_label);
}

static void toggle_search_toolbar(void) {
    if (!search_toolbar) return;

    if (search_visible) {
        lv_obj_add_flag(search_toolbar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - 85); // Adjusted for larger status bar
        lv_obj_align(text_area, LV_ALIGN_TOP_MID, 0, 50);
        search_visible = false;
        clear_search_highlights();
        update_status("Search closed");
    } else {
        lv_obj_clear_flag(search_toolbar, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_size(text_area, lv_pct(100), lv_pct(100) - 130); // Adjusted for larger status bar
        lv_obj_align(text_area, LV_ALIGN_TOP_MID, 0, 95);
        search_visible = true;
        lv_obj_add_state(search_input, LV_STATE_FOCUSED);
        update_status("Search opened");
    }
}

static void update_search_results(void) {
    if (!search_results_label) return;

    char result_text[32];
    if (total_search_matches > 0) {
        snprintf(result_text, sizeof(result_text), "%d/%d", current_search_match + 1, total_search_matches);
    } else {
        snprintf(result_text, sizeof(result_text), "0/0");
    }
    lv_label_set_text(search_results_label, result_text);
}

static int find_all_matches(const char *search_term) {
    if (!search_term || !text_area || strlen(search_term) == 0) {
        return 0;
    }

    const char *text = lv_textarea_get_text(text_area);
    if (!text) return 0;

    // For large files, limit search to prevent UI freezing
    if (is_large_file && current_file_size > HUGE_FILE_THRESHOLD) {
        ESP_LOGW(TAG, "Search in very large file may be slow");
        update_status("Searching large file...");
    }

    int matches = 0;
    const char *pos = text;
    size_t search_len = strlen(search_term);
    size_t text_len = strlen(text);

    // Optimize search for large files by processing in chunks
    if (is_large_file && text_len > HUGE_FILE_THRESHOLD) {
        const size_t chunk_size = 32 * 1024; // 32KB chunks for search
        size_t processed = 0;

        while (processed < text_len && matches < 1000) { // Limit to 1000 matches for performance
            size_t chunk_end = (processed + chunk_size < text_len) ? processed + chunk_size : text_len;

            // Extend chunk to avoid splitting search terms
            while (chunk_end < text_len && chunk_end > processed + search_len &&
                   memcmp(text + chunk_end - search_len + 1, search_term, search_len - 1) == 0) {
                chunk_end++;
            }

            const char *chunk_start = text + processed;
            const char *chunk_pos = chunk_start;

            while ((chunk_pos = strstr(chunk_pos, search_term)) != NULL &&
                   chunk_pos < text + chunk_end) {
                matches++;
                chunk_pos += search_len;

                if (matches >= 1000) break; // Performance limit
            }

            processed = chunk_end;
        }

        if (matches >= 1000) {
            ESP_LOGW(TAG, "Search limited to 1000 matches for performance");
            update_status("1000+ matches found (limited)");
        }
    } else {
        // Standard search for smaller files
        while ((pos = strstr(pos, search_term)) != NULL) {
            matches++;
            pos += search_len;
        }
    }

    return matches;
}

static void highlight_search_match(int match_index) {
    if (!text_area || !last_search_term[0] || match_index < 0) {
        return;
    }

    const char *text = lv_textarea_get_text(text_area);
    if (!text) return;

    const char *pos = text;
    size_t search_len = strlen(last_search_term);
    int current_match = 0;

    // Find the specific match
    while ((pos = strstr(pos, last_search_term)) != NULL) {
        if (current_match == match_index) {
            uint32_t start_pos = pos - text;

            // Set cursor to highlight the match
            lv_textarea_set_cursor_pos(text_area, start_pos);
            lv_textarea_add_text(text_area, ""); // Trigger cursor update

            break;
        }
        current_match++;
        pos += search_len;
    }
}

static void clear_search_highlights(void) {
    // Clear any highlighting by resetting cursor
    if (text_area) {
        lv_textarea_set_cursor_pos(text_area, 0);
    }
}

static void apply_syntax_highlighting_internal(void) {
    if (!text_area || !syntax_highlighting_enabled) {
        return;
    }

    if (!syntax_highlighter_supports_file_type(current_file_type)) {
        ESP_LOGD(TAG, "Syntax highlighting not supported for file type: %s",
                syntax_highlighter_get_file_type_name(current_file_type));
        return;
    }

    // Performance optimization for large files
    if (is_large_file) {
        if (current_file_size > HUGE_FILE_THRESHOLD) {
            ESP_LOGW(TAG, "Skipping syntax highlighting for very large file (%.1fKB)",
                    (float)current_file_size / 1024);
            update_status("Syntax highlighting disabled for large file");
            return;
        } else {
            ESP_LOGI(TAG, "Applying syntax highlighting for large file (%.1fKB)",
                    (float)current_file_size / 1024);
            update_status("Applying syntax highlighting...");
        }
    }

    bool success = syntax_highlighter_apply(text_area, !is_large_file); // Simplified highlighting for large files
    if (success) {
        ESP_LOGI(TAG, "Applied syntax highlighting for %s (%.1fKB)",
                syntax_highlighter_get_file_type_name(current_file_type),
                (float)current_file_size / 1024);
        if (is_large_file) {
            update_status("Syntax highlighting applied (simplified)");
        }
    } else {
        ESP_LOGW(TAG, "Failed to apply syntax highlighting");
        update_status("Syntax highlighting failed");
    }
}

static void update_syntax_highlighting_button_color(void) {
    if (!syntax_highlight_btn) return;

    if (syntax_highlighting_enabled && syntax_highlighter_supports_file_type(current_file_type)) {
        // Active state - bright orange
        lv_obj_set_style_bg_color(syntax_highlight_btn, lv_color_hex(0xe67e22), 0);
    } else if (syntax_highlighting_enabled) {
        // Enabled but not supported - dimmed orange
        lv_obj_set_style_bg_color(syntax_highlight_btn, lv_color_hex(0xa0522d), 0);
    } else {
        // Disabled - gray
        lv_obj_set_style_bg_color(syntax_highlight_btn, lv_color_hex(0x7f8c8d), 0);
    }
}

// Public API functions
void text_editor_toggle_search(void) {
    toggle_search_toolbar();
}

bool text_editor_search(const char *search_term, bool forward) {
    if (!search_term || !text_area || strlen(search_term) == 0) {
        return false;
    }

    // Update search term if different
    if (strcmp(last_search_term, search_term) != 0) {
        strncpy(last_search_term, search_term, sizeof(last_search_term) - 1);
        last_search_term[sizeof(last_search_term) - 1] = '\0';
        total_search_matches = find_all_matches(search_term);
        current_search_match = forward ? 0 : total_search_matches - 1;
    } else {
        // Navigate to next/previous match
        if (total_search_matches > 0) {
            if (forward) {
                current_search_match = (current_search_match + 1) % total_search_matches;
            } else {
                current_search_match = (current_search_match - 1 + total_search_matches) % total_search_matches;
            }
        }
    }

    if (total_search_matches > 0) {
        highlight_search_match(current_search_match);
        update_search_results();
        return true;
    }

    update_search_results();
    return false;
}

void text_editor_get_search_info(int *current_match, int *total_matches) {
    if (current_match) {
        *current_match = current_search_match;
    }
    if (total_matches) {
        *total_matches = total_search_matches;
    }
}

void text_editor_toggle_syntax_highlighting(void) {
    syntax_highlighting_enabled = !syntax_highlighting_enabled;
    syntax_highlighter_set_enabled(syntax_highlighting_enabled);

    if (syntax_highlighting_enabled) {
        apply_syntax_highlighting_internal();
        update_status("Syntax highlighting enabled");
    } else {
        syntax_highlighter_clear(text_area);
        update_status("Syntax highlighting disabled");
    }

    update_syntax_highlighting_button_color();
    ESP_LOGI(TAG, "Syntax highlighting %s", syntax_highlighting_enabled ? "enabled" : "disabled");
}

bool text_editor_is_syntax_highlighting_enabled(void) {
    return syntax_highlighting_enabled && syntax_highlighter_is_enabled();
}

void text_editor_apply_syntax_highlighting(void) {
    if (syntax_highlighting_enabled) {
        apply_syntax_highlighting_internal();
    }
}

void text_editor_clear_syntax_highlighting(void) {
    syntax_highlighter_clear(text_area);
}

void destroy_text_editor_screen(void) {
    if (text_editor_screen) {
        // Clean up syntax highlighter
        syntax_highlighter_destroy();

        lv_obj_del(text_editor_screen);
        text_editor_screen = NULL;
        search_toolbar = NULL;
        search_input = NULL;
        search_results_label = NULL;
        syntax_highlight_btn = NULL;
        ESP_LOGI(TAG, "Text editor screen destroyed");
    }
}

// Helper functions for large file handling

/**
 * @brief Allocate file buffer with preference for PSRAM for large files
 * @param size Size of buffer to allocate
 * @return Allocated buffer or NULL on failure
 */
static char* allocate_file_buffer(size_t size) {
    char* buffer = NULL;

    // For large buffers, prefer PSRAM if available
    if (size >= PREFER_PSRAM_SIZE) {
        buffer = (char*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        if (buffer) {
            ESP_LOGI(TAG, "Allocated %zu bytes in PSRAM for file buffer", size);
            return buffer;
        } else {
            ESP_LOGW(TAG, "PSRAM allocation failed, trying internal RAM");
        }
    }

    // Fallback to internal RAM or for smaller buffers
    buffer = (char*)malloc(size);
    if (buffer) {
        ESP_LOGI(TAG, "Allocated %zu bytes in internal RAM for file buffer", size);
    } else {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes for file buffer", size);
    }

    return buffer;
}

/**
 * @brief Free file buffer allocated with allocate_file_buffer
 * @param buffer Buffer to free
 */
static void free_file_buffer(char* buffer) {
    if (buffer) {
        // Check if buffer is in PSRAM or internal RAM
        if (esp_ptr_external_ram(buffer)) {
            ESP_LOGD(TAG, "Freeing PSRAM buffer");
        } else {
            ESP_LOGD(TAG, "Freeing internal RAM buffer");
        }
        free(buffer);
    }
}

/**
 * @brief Show warning dialog for large files (simulated with status message)
 * @param file_size Size of the file
 * @return true to continue loading, false to cancel
 */
static bool show_large_file_warning(size_t file_size) {
    char warning_msg[256];

    if (file_size > HUGE_FILE_THRESHOLD) {
        snprintf(warning_msg, sizeof(warning_msg),
                "Very large file (%.1fKB). Performance may be affected. Loading...",
                (float)file_size / 1024);
    } else {
        snprintf(warning_msg, sizeof(warning_msg),
                "Large file (%.1fKB). May take time to load...",
                (float)file_size / 1024);
    }

    update_status(warning_msg);
    ESP_LOGW(TAG, "Loading large file: %zu bytes", file_size);

    // In a full implementation, this would show a dialog
    // For now, we automatically continue but log the warning
    return true;
}

/**
 * @brief Update status bar with comprehensive file information
 * @param filename Name of the file
 * @param file_size Size of the file in bytes
 * @param file_type File type string
 */
static void update_status_with_file_info(const char* filename, size_t file_size, const char* file_type) {
    char status_msg[256];
    char size_str[32];

    // Format file size appropriately
    if (file_size >= 1024 * 1024) {
        snprintf(size_str, sizeof(size_str), "%.2fMB", (float)file_size / (1024 * 1024));
    } else if (file_size >= 1024) {
        snprintf(size_str, sizeof(size_str), "%.1fKB", (float)file_size / 1024);
    } else {
        snprintf(size_str, sizeof(size_str), "%zu bytes", file_size);
    }

    // Add performance indicators for large files
    const char* perf_indicator = "";
    if (file_size > HUGE_FILE_THRESHOLD) {
        perf_indicator = " [LARGE]";
    } else if (file_size > LARGE_FILE_THRESHOLD) {
        perf_indicator = " [BIG]";
    }

    snprintf(status_msg, sizeof(status_msg), "Opened: %s (%s, %s)%s",
             filename, file_type, size_str, perf_indicator);
    update_status(status_msg);
}

/**
 * @brief Load file content progressively with memory management
 * @param file Open file handle
 * @param file_size Size of file to load
 * @param content Pointer to store allocated content
 * @return ESP_OK on success
 */
static esp_err_t load_file_progressive(FILE* file, size_t file_size, char** content) {
    if (!file || !content) {
        return ESP_ERR_INVALID_ARG;
    }

    // Allocate buffer with extra space for null terminator
    *content = allocate_file_buffer(file_size + 1);
    if (!*content) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes for file content", file_size + 1);
        update_status("Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }

    // For very large files, read in chunks with progress updates
    if (file_size > HUGE_FILE_THRESHOLD) {
        const size_t chunk_size = 64 * 1024; // 64KB chunks
        size_t total_read = 0;
        char* write_pos = *content;

        while (total_read < file_size) {
            size_t to_read = (file_size - total_read > chunk_size) ? chunk_size : (file_size - total_read);
            size_t read_size = fread(write_pos, 1, to_read, file);

            if (read_size == 0) {
                ESP_LOGE(TAG, "File read error at position %zu", total_read);
                update_status("File read error");
                return ESP_FAIL;
            }

            total_read += read_size;
            write_pos += read_size;

            // Update progress for large files
            if (file_size > HUGE_FILE_THRESHOLD) {
                int progress = (total_read * 100) / file_size;
                char progress_msg[64];
                snprintf(progress_msg, sizeof(progress_msg), "Loading... %d%%", progress);
                update_status(progress_msg);
            }
        }

        (*content)[file_size] = '\0';
        ESP_LOGI(TAG, "Loaded large file in chunks: %zu bytes", file_size);
    } else {
        // Standard read for smaller files
        size_t read_size = fread(*content, 1, file_size, file);
        (*content)[read_size] = '\0';

        if (read_size != file_size) {
            ESP_LOGW(TAG, "File size mismatch: expected %zu, read %zu", file_size, read_size);
        }
    }

    return ESP_OK;
}

// Public API additions for large file support

/**
 * @brief Get current file size
 * @return File size in bytes, 0 if no file loaded
 */
size_t text_editor_get_file_size(void) {
    return current_file_size;
}

/**
 * @brief Check if current file is considered large
 * @return true if file is above large file threshold
 */
bool text_editor_is_large_file(void) {
    return is_large_file;
}

/**
 * @brief Get memory usage information
 * @param internal_free Pointer to store free internal RAM
 * @param spiram_free Pointer to store free SPIRAM
 */
void text_editor_get_memory_info(size_t* internal_free, size_t* spiram_free) {
    if (internal_free) {
        *internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    }
    if (spiram_free) {
        *spiram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    }
}