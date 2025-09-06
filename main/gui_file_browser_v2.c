#include "gui_file_browser_v2.h"
#include "gui_styles.h"
#include "gui_events.h"
#include "sd_manager.h"
#include "file_operations.h"
#include "config_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

static const char *TAG = "FILE_BROWSER_V2";

// Static state
static file_browser_state_t browser_state = {
    .current_path = "/",
    .current_page = 0,
    .total_pages = 1,
    .total_files = 0,
    .selected_index = -1,
    .multi_select_mode = false,
    .selected_items = NULL,
    .items_per_page = FILES_PER_PAGE
};

// Static UI elements
static lv_obj_t *file_browser_screen = NULL;
static lv_obj_t *path_label = NULL;
static lv_obj_t *file_container = NULL;
static lv_obj_t *page_label = NULL;
static lv_obj_t *info_label = NULL;
static lv_obj_t *prev_btn = NULL;
static lv_obj_t *next_btn = NULL;
static lv_obj_t *multi_select_btn = NULL;
// Context menu support - implementation pending
// static lv_obj_t *context_menu = NULL;

// Static file list cache
static file_entry_enhanced_t *file_cache = NULL;
static int file_cache_size = 0;

// Settings integration variables
static int current_view_mode = 0;  // 0=list, 1=grid, 2=detailed
static int current_sort_mode = 0;  // 0=name, 1=size, 2=date, 3=type
static bool sort_ascending = true;
static bool show_hidden = false;

// Forward declarations
static void create_file_list_items(void);
static void update_pagination_buttons(void);
static void scan_directory_enhanced(const char *path);
static int compare_files_by_name(const void *a, const void *b);
static int compare_files_by_size(const void *a, const void *b);
static int compare_files_by_date(const void *a, const void *b);
static int compare_files_by_type(const void *a, const void *b);

lv_obj_t* gui_file_browser_v2_create(void) {
    // Create screen
    file_browser_screen = lv_obj_create(NULL);
    lv_obj_add_style(file_browser_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Get configuration
    launcher_config_t *config = config_manager_get_current();
    browser_state.items_per_page = config->file_browser.items_per_page;
    
    // Create top bar container
    lv_obj_t *top_bar = lv_obj_create(file_browser_screen);
    lv_obj_set_size(top_bar, lv_pct(100), 80);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(top_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(top_bar, 10, 0);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(top_bar);
    lv_obj_set_size(back_btn, 80, 50);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);
    
    // Up directory button
    lv_obj_t *up_btn = lv_button_create(top_bar);
    lv_obj_set_size(up_btn, 80, 50);
    lv_obj_align_to(up_btn, back_btn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    apply_button_style(up_btn);
    lv_obj_add_event_cb(up_btn, file_browser_v2_up_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *up_label = lv_label_create(up_btn);
    lv_label_set_text(up_label, LV_SYMBOL_UP);
    lv_obj_center(up_label);
    
    // Path label
    path_label = lv_label_create(top_bar);
    lv_label_set_text(path_label, browser_state.current_path);
    lv_obj_set_style_text_color(path_label, THEME_SUCCESS_COLOR, 0);
    lv_obj_set_style_text_font(path_label, THEME_FONT_NORMAL, 0);
    lv_obj_align_to(path_label, up_btn, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_label_set_long_mode(path_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(path_label, 400);
    
    // Multi-select toggle button
    multi_select_btn = lv_button_create(top_bar);
    lv_obj_set_size(multi_select_btn, 100, 50);
    lv_obj_align(multi_select_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    apply_button_style(multi_select_btn);
    lv_obj_add_flag(multi_select_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(multi_select_btn, file_browser_v2_multi_select_handler, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_t *multi_label = lv_label_create(multi_select_btn);
    lv_label_set_text(multi_label, LV_SYMBOL_LIST " Multi");
    lv_obj_center(multi_label);
    
    // File container (scrollable)
    file_container = lv_obj_create(file_browser_screen);
    lv_obj_set_size(file_container, lv_pct(95), lv_pct(65));
    lv_obj_align(file_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_color(file_container, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_color(file_container, THEME_PRIMARY_COLOR, 0);
    lv_obj_set_style_border_width(file_container, 2, 0);
    lv_obj_set_style_radius(file_container, 10, 0);
    lv_obj_set_style_pad_all(file_container, 10, 0);
    lv_obj_set_flex_flow(file_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(file_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    
    // Bottom bar for pagination and info
    lv_obj_t *bottom_bar = lv_obj_create(file_browser_screen);
    lv_obj_set_size(bottom_bar, lv_pct(100), 80);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(bottom_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(bottom_bar, 10, 0);
    
    // Previous page button
    prev_btn = lv_button_create(bottom_bar);
    lv_obj_set_size(prev_btn, 80, 50);
    lv_obj_align(prev_btn, LV_ALIGN_LEFT_MID, 10, 0);
    apply_button_style(prev_btn);
    lv_obj_add_event_cb(prev_btn, file_browser_v2_prev_page_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *prev_label = lv_label_create(prev_btn);
    lv_label_set_text(prev_label, LV_SYMBOL_LEFT);
    lv_obj_center(prev_label);
    
    // Page info label
    page_label = lv_label_create(bottom_bar);
    lv_label_set_text(page_label, "Page 1 / 1");
    lv_obj_set_style_text_color(page_label, THEME_TEXT_COLOR, 0);
    lv_obj_align(page_label, LV_ALIGN_CENTER, 0, -10);
    
    // File info label
    info_label = lv_label_create(bottom_bar);
    lv_label_set_text(info_label, "0 files, 0 folders");
    lv_obj_set_style_text_color(info_label, THEME_SECONDARY_COLOR, 0);
    lv_obj_set_style_text_font(info_label, THEME_FONT_SMALL, 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, 10);
    
    // Next page button
    next_btn = lv_button_create(bottom_bar);
    lv_obj_set_size(next_btn, 80, 50);
    lv_obj_align(next_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    apply_button_style(next_btn);
    lv_obj_add_event_cb(next_btn, file_browser_v2_next_page_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *next_label = lv_label_create(next_btn);
    lv_label_set_text(next_label, LV_SYMBOL_RIGHT);
    lv_obj_center(next_label);
    
    // Initial directory scan
    scan_directory_enhanced(browser_state.current_path);
    create_file_list_items();
    update_pagination_buttons();
    
    return file_browser_screen;
}

static void scan_directory_enhanced(const char *path) {
    // Free previous cache
    if (file_cache) {
        free(file_cache);
        file_cache = NULL;
        file_cache_size = 0;
    }
    
    // Free selection array
    if (browser_state.selected_items) {
        free(browser_state.selected_items);
        browser_state.selected_items = NULL;
    }
    
    if (!sd_manager_is_mounted()) {
        ESP_LOGW(TAG, "SD card not mounted");
        browser_state.total_files = 0;
        browser_state.total_pages = 1;
        return;
    }
    
    // Build full path
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    // Count files first
    DIR *dir = opendir(full_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", full_path);
        browser_state.total_files = 0;
        browser_state.total_pages = 1;
        return;
    }
    
    int count = 0;
    struct dirent *entry;
    launcher_config_t *config = config_manager_get_current();
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip hidden files if configured (check both config and local override)
        bool should_show_hidden = config->file_browser.show_hidden_files || show_hidden;
        if (!should_show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        count++;
    }
    closedir(dir);
    
    if (count == 0) {
        browser_state.total_files = 0;
        browser_state.total_pages = 1;
        return;
    }
    
    // Allocate cache
    file_cache = calloc(count, sizeof(file_entry_enhanced_t));
    if (!file_cache) {
        ESP_LOGE(TAG, "Failed to allocate file cache");
        browser_state.total_files = 0;
        browser_state.total_pages = 1;
        return;
    }
    
    // Allocate selection array
    browser_state.selected_items = calloc(count, sizeof(bool));
    
    // Scan again to fill cache
    dir = opendir(full_path);
    if (!dir) {
        free(file_cache);
        file_cache = NULL;
        return;
    }
    
    int index = 0;
    while ((entry = readdir(dir)) != NULL && index < count) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        bool should_show_hidden = config->file_browser.show_hidden_files || show_hidden;
        if (!should_show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        // Get file info
        char item_path[768];
        snprintf(item_path, sizeof(item_path), "%s/%s", full_path, entry->d_name);
        
        struct stat file_stat;
        if (stat(item_path, &file_stat) == 0) {
            // Fill enhanced entry - use snprintf to avoid truncation warnings
            snprintf(file_cache[index].name, sizeof(file_cache[index].name), "%s", entry->d_name);
            
            // Create display name (truncate if needed)
            if (strlen(entry->d_name) > MAX_FILENAME_DISPLAY) {
                snprintf(file_cache[index].display_name, sizeof(file_cache[index].display_name), "%.*s...", MAX_FILENAME_DISPLAY - 3, entry->d_name);
            } else {
                snprintf(file_cache[index].display_name, sizeof(file_cache[index].display_name), "%s", entry->d_name);
            }
            
            file_cache[index].is_directory = S_ISDIR(file_stat.st_mode);
            file_cache[index].size = file_stat.st_size;
            file_cache[index].modified_time = file_stat.st_mtime;
            
            // Get icon and color
            file_cache[index].icon = gui_file_browser_v2_get_icon(entry->d_name, file_cache[index].is_directory);
            file_cache[index].color = gui_file_browser_v2_get_color(entry->d_name, file_cache[index].is_directory);
            
            // Format size and date
            gui_file_browser_v2_format_size(file_cache[index].size, file_cache[index].size_str, sizeof(file_cache[index].size_str));
            gui_file_browser_v2_format_date(file_cache[index].modified_time, file_cache[index].date_str, sizeof(file_cache[index].date_str));
            
            index++;
        }
    }
    closedir(dir);
    
    file_cache_size = index;
    browser_state.total_files = index;
    
    // Sort files based on configuration or local override
    void *compare_func = compare_files_by_name; // Default
    int sort_mode_to_use = current_sort_mode > 0 ? current_sort_mode : config->file_browser.sort_by;
    
    switch (sort_mode_to_use) {
        case 1: // SORT_BY_SIZE
            compare_func = compare_files_by_size;
            break;
        case 2: // SORT_BY_DATE
            compare_func = compare_files_by_date;
            break;
        case 3: // SORT_BY_TYPE
            compare_func = compare_files_by_type;
            break;
        case 0: // SORT_BY_NAME
        default:
            compare_func = compare_files_by_name;
            break;
    }
    
    qsort(file_cache, file_cache_size, sizeof(file_entry_enhanced_t), compare_func);
    
    // Calculate pages
    browser_state.total_pages = (browser_state.total_files + browser_state.items_per_page - 1) / browser_state.items_per_page;
    if (browser_state.total_pages == 0) {
        browser_state.total_pages = 1;
    }
    
    // Reset to first page
    browser_state.current_page = 0;
}

static void create_file_list_items(void) {
    // Clear existing items
    lv_obj_clean(file_container);
    
    if (file_cache_size == 0) {
        lv_obj_t *empty_label = lv_label_create(file_container);
        lv_label_set_text(empty_label, "No files found");
        lv_obj_set_style_text_color(empty_label, THEME_WARNING_COLOR, 0);
        lv_obj_center(empty_label);
        return;
    }
    
    launcher_config_t *config = config_manager_get_current();
    
    // Calculate range for current page
    int start_index = browser_state.current_page * browser_state.items_per_page;
    int end_index = start_index + browser_state.items_per_page;
    if (end_index > file_cache_size) {
        end_index = file_cache_size;
    }
    
    // Create items for current page
    for (int i = start_index; i < end_index; i++) {
        lv_obj_t *item = lv_obj_create(file_container);
        lv_obj_set_size(item, lv_pct(100), 60);
        lv_obj_set_style_bg_color(item, lv_color_hex(0x2a2a2a), 0);
        lv_obj_set_style_border_width(item, 1, 0);
        lv_obj_set_style_border_color(item, lv_color_hex(0x404040), 0);
        lv_obj_set_style_radius(item, 5, 0);
        lv_obj_set_style_pad_all(item, 8, 0);
        lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
        
        // Add checkbox if in multi-select mode
        if (browser_state.multi_select_mode) {
            lv_obj_t *checkbox = lv_checkbox_create(item);
            lv_obj_align(checkbox, LV_ALIGN_LEFT_MID, 0, 0);
            if (browser_state.selected_items && browser_state.selected_items[i]) {
                lv_obj_add_state(checkbox, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(checkbox, LV_STATE_CHECKED);
            }
        }
        
        // Icon
        lv_obj_t *icon = lv_label_create(item);
        lv_label_set_text(icon, file_cache[i].icon);
        lv_obj_set_style_text_color(icon, file_cache[i].color, 0);
        lv_obj_set_style_text_font(icon, THEME_FONT_LARGE, 0);
        if (browser_state.multi_select_mode) {
            lv_obj_align(icon, LV_ALIGN_LEFT_MID, 40, 0);
        } else {
            lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);
        }
        
        // File name
        lv_obj_t *name = lv_label_create(item);
        lv_label_set_text(name, file_cache[i].display_name);
        lv_obj_set_style_text_color(name, THEME_TEXT_COLOR, 0);
        lv_obj_set_style_text_font(name, THEME_FONT_NORMAL, 0);
        if (browser_state.multi_select_mode) {
            lv_obj_align_to(name, icon, LV_ALIGN_OUT_RIGHT_MID, 15, -10);
        } else {
            lv_obj_align_to(name, icon, LV_ALIGN_OUT_RIGHT_MID, 15, -10);
        }
        
        // Size/Info
        if (config->file_browser.show_file_sizes || config->file_browser.view_mode == VIEW_MODE_DETAILED) {
            lv_obj_t *size = lv_label_create(item);
            if (file_cache[i].is_directory) {
                lv_label_set_text(size, "Folder");
            } else {
                lv_label_set_text(size, file_cache[i].size_str);
            }
            lv_obj_set_style_text_color(size, THEME_SECONDARY_COLOR, 0);
            lv_obj_set_style_text_font(size, THEME_FONT_SMALL, 0);
            lv_obj_align_to(size, name, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
        }
        
        // Date (if detailed view)
        if (config->file_browser.view_mode == VIEW_MODE_DETAILED) {
            lv_obj_t *date = lv_label_create(item);
            lv_label_set_text(date, file_cache[i].date_str);
            lv_obj_set_style_text_color(date, THEME_SECONDARY_COLOR, 0);
            lv_obj_set_style_text_font(date, THEME_FONT_SMALL, 0);
            lv_obj_align(date, LV_ALIGN_RIGHT_MID, -10, 0);
        }
        
        // Add click event
        lv_obj_add_event_cb(item, NULL, LV_EVENT_CLICKED, (void*)(intptr_t)i);
    }
    
    // Update info label
    int dir_count = 0, file_count = 0;
    for (int i = 0; i < file_cache_size; i++) {
        if (file_cache[i].is_directory) {
            dir_count++;
        } else {
            file_count++;
        }
    }
    
    char info_text[64];
    snprintf(info_text, sizeof(info_text), "%d files, %d folders", file_count, dir_count);
    lv_label_set_text(info_label, info_text);
    
    // Update page label
    char page_text[32];
    snprintf(page_text, sizeof(page_text), "Page %d / %d", 
             browser_state.current_page + 1, browser_state.total_pages);
    lv_label_set_text(page_label, page_text);
}

static void update_pagination_buttons(void) {
    // Enable/disable pagination buttons
    if (browser_state.current_page == 0) {
        lv_obj_add_state(prev_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(prev_btn, LV_STATE_DISABLED);
    }
    
    if (browser_state.current_page >= browser_state.total_pages - 1) {
        lv_obj_add_state(next_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_clear_state(next_btn, LV_STATE_DISABLED);
    }
}

// Comparison functions for sorting
static int compare_files_by_name(const void *a, const void *b) {
    file_entry_enhanced_t *fa = (file_entry_enhanced_t *)a;
    file_entry_enhanced_t *fb = (file_entry_enhanced_t *)b;
    
    // Directories first
    if (fa->is_directory && !fb->is_directory) return -1;
    if (!fa->is_directory && fb->is_directory) return 1;
    
    launcher_config_t *config = config_manager_get_current();
    int result = strcasecmp(fa->name, fb->name);
    return config->file_browser.sort_ascending ? result : -result;
}

static int compare_files_by_size(const void *a, const void *b) {
    file_entry_enhanced_t *fa = (file_entry_enhanced_t *)a;
    file_entry_enhanced_t *fb = (file_entry_enhanced_t *)b;
    
    // Directories first
    if (fa->is_directory && !fb->is_directory) return -1;
    if (!fa->is_directory && fb->is_directory) return 1;
    
    launcher_config_t *config = config_manager_get_current();
    int result = (fa->size > fb->size) - (fa->size < fb->size);
    return config->file_browser.sort_ascending ? result : -result;
}

static int compare_files_by_date(const void *a, const void *b) {
    file_entry_enhanced_t *fa = (file_entry_enhanced_t *)a;
    file_entry_enhanced_t *fb = (file_entry_enhanced_t *)b;
    
    // Directories first
    if (fa->is_directory && !fb->is_directory) return -1;
    if (!fa->is_directory && fb->is_directory) return 1;
    
    launcher_config_t *config = config_manager_get_current();
    int result = (fa->modified_time > fb->modified_time) - (fa->modified_time < fb->modified_time);
    return config->file_browser.sort_ascending ? result : -result;
}

static int compare_files_by_type(const void *a, const void *b) {
    file_entry_enhanced_t *fa = (file_entry_enhanced_t *)a;
    file_entry_enhanced_t *fb = (file_entry_enhanced_t *)b;
    
    // Directories first
    if (fa->is_directory && !fb->is_directory) return -1;
    if (!fa->is_directory && fb->is_directory) return 1;
    
    // Get extensions
    const char *ext_a = strrchr(fa->name, '.');
    const char *ext_b = strrchr(fb->name, '.');
    
    if (!ext_a) ext_a = "";
    if (!ext_b) ext_b = "";
    
    launcher_config_t *config = config_manager_get_current();
    int result = strcasecmp(ext_a, ext_b);
    if (result == 0) {
        result = strcasecmp(fa->name, fb->name);
    }
    return config->file_browser.sort_ascending ? result : -result;
}

const char* gui_file_browser_v2_get_icon(const char *filename, bool is_directory) {
    if (is_directory) {
        return LV_SYMBOL_DIRECTORY;
    }
    
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return LV_SYMBOL_FILE;
    }
    ext++; // Skip the dot
    
    // Check file type
    if (strcasecmp(ext, "bin") == 0) {
        return LV_SYMBOL_DOWNLOAD;  // Binary/firmware
    } else if (strcasecmp(ext, "txt") == 0 || strcasecmp(ext, "log") == 0) {
        return LV_SYMBOL_FILE;  // Text
    } else if (strcasecmp(ext, "py") == 0) {
        return LV_SYMBOL_EDIT;  // Python
    } else if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "css") == 0 || 
               strcasecmp(ext, "js") == 0) {
        return LV_SYMBOL_WIFI;  // Web files
    } else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 || 
               strcasecmp(ext, "png") == 0 || strcasecmp(ext, "bmp") == 0) {
        return LV_SYMBOL_IMAGE;  // Images
    } else if (strcasecmp(ext, "json") == 0 || strcasecmp(ext, "cfg") == 0 || 
               strcasecmp(ext, "conf") == 0) {
        return LV_SYMBOL_SETTINGS;  // Config files
    } else if (strcasecmp(ext, "zip") == 0 || strcasecmp(ext, "tar") == 0 || 
               strcasecmp(ext, "gz") == 0) {
        return LV_SYMBOL_COPY;  // Archives
    }
    
    return LV_SYMBOL_FILE;
}

lv_color_t gui_file_browser_v2_get_color(const char *filename, bool is_directory) {
    if (is_directory) {
        return lv_color_hex(0x00ffff);  // Cyan for directories
    }
    
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return THEME_TEXT_COLOR;
    }
    ext++;
    
    // Color code by type
    if (strcasecmp(ext, "bin") == 0) {
        return lv_color_hex(0xff6b6b);  // Red for binaries
    } else if (strcasecmp(ext, "py") == 0) {
        return lv_color_hex(0x4ecdc4);  // Teal for Python
    } else if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "css") == 0 || 
               strcasecmp(ext, "js") == 0) {
        return lv_color_hex(0xffd93d);  // Yellow for web
    } else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 || 
               strcasecmp(ext, "png") == 0 || strcasecmp(ext, "bmp") == 0) {
        return lv_color_hex(0x6bcf7f);  // Green for images
    } else if (strcasecmp(ext, "json") == 0 || strcasecmp(ext, "cfg") == 0 || 
               strcasecmp(ext, "conf") == 0) {
        return lv_color_hex(0xa8dadc);  // Light blue for config
    }
    
    return THEME_TEXT_COLOR;
}

void gui_file_browser_v2_format_size(size_t size, char *buffer, size_t buffer_size) {
    if (size < 1024) {
        snprintf(buffer, buffer_size, "%zu B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buffer, buffer_size, "%.1f MB", size / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, buffer_size, "%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
}

void gui_file_browser_v2_format_date(uint32_t timestamp, char *buffer, size_t buffer_size) {
    struct tm *timeinfo = localtime((time_t*)&timestamp);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M", timeinfo);
}

void gui_file_browser_v2_update(lv_obj_t *screen) {
    if (screen != file_browser_screen) {
        return;
    }
    
    scan_directory_enhanced(browser_state.current_path);
    create_file_list_items();
    update_pagination_buttons();
}

void gui_file_browser_v2_navigate(const char *path) {
    // Only update if path is different to avoid restrict warnings
    if (path != browser_state.current_path) {
        snprintf(browser_state.current_path, sizeof(browser_state.current_path), "%s", path);
    }
    lv_label_set_text(path_label, browser_state.current_path);
    gui_file_browser_v2_update(file_browser_screen);
}

void gui_file_browser_v2_go_up(void) {
    char *last_slash = strrchr(browser_state.current_path, '/');
    if (last_slash && last_slash != browser_state.current_path) {
        *last_slash = '\0';
        // Direct update to avoid restrict warnings
        lv_label_set_text(path_label, browser_state.current_path);
        gui_file_browser_v2_update(file_browser_screen);
    }
}

void gui_file_browser_v2_next_page(void) {
    if (browser_state.current_page < browser_state.total_pages - 1) {
        browser_state.current_page++;
        create_file_list_items();
        update_pagination_buttons();
    }
}

void gui_file_browser_v2_prev_page(void) {
    if (browser_state.current_page > 0) {
        browser_state.current_page--;
        create_file_list_items();
        update_pagination_buttons();
    }
}

void gui_file_browser_v2_set_multi_select(bool enable) {
    browser_state.multi_select_mode = enable;
    if (enable && !browser_state.selected_items && file_cache_size > 0) {
        browser_state.selected_items = calloc(file_cache_size, sizeof(bool));
    }
    create_file_list_items();
}

file_browser_state_t* gui_file_browser_v2_get_state(void) {
    return &browser_state;
}

// Settings integration functions

void gui_file_browser_v2_set_view_mode(int mode) {
    current_view_mode = mode;
    // For now, we only support list view, but store the setting
    // Future implementation could switch between different layouts
    if (file_browser_screen) {
        create_file_list_items();
    }
}

void gui_file_browser_v2_set_sort_mode(int mode, bool ascending) {
    current_sort_mode = mode;
    sort_ascending = ascending;
    // Re-scan and sort the current directory
    if (file_browser_screen) {
        scan_directory_enhanced(browser_state.current_path);
        create_file_list_items();
    }
}

void gui_file_browser_v2_show_hidden_files(bool show) {
    show_hidden = show;
    // Re-scan directory with new hidden file setting
    if (file_browser_screen) {
        scan_directory_enhanced(browser_state.current_path);
        create_file_list_items();
    }
}