#include "gui_screen_settings.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "gui_file_browser_v2.h"
#include "config_manager.h"
#include "bsp/m5stack_tab5.h"
#include "esp_log.h"
#include <stdio.h>
#include <inttypes.h>

static const char *TAG = "GUI_SETTINGS";

// Screen objects (declared extern in gui_screens.h)
lv_obj_t *settings_screen = NULL;
static lv_obj_t *settings_container = NULL;
static lv_obj_t *tabview = NULL;

// System settings controls
static lv_obj_t *brightness_slider = NULL;
static lv_obj_t *timeout_dropdown = NULL;
static lv_obj_t *animations_switch = NULL;
static lv_obj_t *auto_mount_switch = NULL;

// File browser settings controls  
static lv_obj_t *view_mode_dropdown = NULL;
static lv_obj_t *sort_by_dropdown = NULL;
static lv_obj_t *sort_order_switch = NULL;
static lv_obj_t *show_hidden_switch = NULL;
static lv_obj_t *show_extensions_switch = NULL;
static lv_obj_t *items_per_page_slider = NULL;

// Theme settings controls
static lv_obj_t *theme_dropdown = NULL;
// Note: Color picker will be implemented in future update
// static lv_obj_t *primary_color_picker = NULL;

// Control IDs for event handling
typedef enum {
    SETTINGS_BRIGHTNESS,
    SETTINGS_TIMEOUT,
    SETTINGS_ANIMATIONS,
    SETTINGS_AUTO_MOUNT,
    SETTINGS_VIEW_MODE,
    SETTINGS_SORT_BY,
    SETTINGS_SORT_ORDER,
    SETTINGS_SHOW_HIDDEN,
    SETTINGS_SHOW_EXTENSIONS,
    SETTINGS_ITEMS_PER_PAGE,
    SETTINGS_THEME,
    SETTINGS_PRIMARY_COLOR,
    SETTINGS_RESET_DEFAULTS,
    SETTINGS_BACKUP_CONFIG,
    SETTINGS_RESTORE_CONFIG
} settings_control_id_t;

// Forward declarations
static void create_system_tab(lv_obj_t *parent);
static void create_file_browser_tab(lv_obj_t *parent);
static void create_theme_tab(lv_obj_t *parent);
static void create_backup_tab(lv_obj_t *parent);
static void settings_event_handler(lv_event_t *e);
static void apply_current_config_to_ui(void);
static void save_settings(void);

void create_settings_screen(void) {
    if (settings_screen) {
        return; // Already created
    }
    
    ESP_LOGI(TAG, "Creating settings screen");
    
    // Create main screen
    settings_screen = lv_obj_create(NULL);
    lv_obj_add_style(settings_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(settings_screen);
    lv_obj_set_size(title_bar, lv_pct(100), 60);
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_opa(title_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    
    // Title label
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Settings");
    lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(title_label, THEME_FONT_LARGE, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 20, 0);
    
    // Back button
    lv_obj_t *back_btn = lv_btn_create(title_bar);
    lv_obj_set_size(back_btn, 80, 40);
    lv_obj_align(back_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
    lv_obj_center(back_label);
    
    // Create main container for tabs - position below title bar
    settings_container = lv_obj_create(settings_screen);
    lv_obj_set_size(settings_container, lv_pct(100), lv_pct(100) - 60);
    lv_obj_align(settings_container, LV_ALIGN_TOP_LEFT, 0, 60);
    lv_obj_set_style_bg_opa(settings_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(settings_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(settings_container, 10, 0);
    
    // Create tabview
    tabview = lv_tabview_create(settings_container);
    lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tabview, 60);
    
    // Create tabs
    lv_obj_t *system_tab = lv_tabview_add_tab(tabview, "System");
    lv_obj_t *browser_tab = lv_tabview_add_tab(tabview, "Browser");
    lv_obj_t *theme_tab = lv_tabview_add_tab(tabview, "Theme");
    lv_obj_t *backup_tab = lv_tabview_add_tab(tabview, "Backup");
    
    // Create tab content
    create_system_tab(system_tab);
    create_file_browser_tab(browser_tab);
    create_theme_tab(theme_tab);
    create_backup_tab(backup_tab);
    
    // Apply current configuration to UI
    apply_current_config_to_ui();
    
    ESP_LOGI(TAG, "Settings screen created successfully");
}

static void create_system_tab(lv_obj_t *parent) {
    // Create scrollable container
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_set_style_pad_all(cont, 20, 0);
    
    int y_offset = 0;
    
    // Brightness setting
    lv_obj_t *brightness_label = lv_label_create(cont);
    lv_label_set_text(brightness_label, "Screen Brightness");
    lv_obj_set_style_text_font(brightness_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    brightness_slider = lv_slider_create(cont);
    lv_obj_set_size(brightness_slider, lv_pct(80), 20);
    lv_obj_align(brightness_slider, LV_ALIGN_TOP_LEFT, 0, y_offset + 30);
    lv_slider_set_range(brightness_slider, 10, 100);
    lv_obj_add_event_cb(brightness_slider, settings_event_handler, LV_EVENT_VALUE_CHANGED, 
                        (void*)SETTINGS_BRIGHTNESS);
    y_offset += 70;
    
    // Screen timeout setting
    lv_obj_t *timeout_label = lv_label_create(cont);
    lv_label_set_text(timeout_label, "Screen Timeout");
    lv_obj_set_style_text_font(timeout_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(timeout_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    timeout_dropdown = lv_dropdown_create(cont);
    lv_dropdown_set_options(timeout_dropdown, "Never\n1 min\n2 min\n5 min\n10 min\n15 min");
    lv_obj_set_size(timeout_dropdown, 200, 40);
    lv_obj_align(timeout_dropdown, LV_ALIGN_TOP_LEFT, 0, y_offset + 30);
    lv_obj_add_event_cb(timeout_dropdown, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_TIMEOUT);
    y_offset += 80;
    
    // Animations setting
    lv_obj_t *anim_label = lv_label_create(cont);
    lv_label_set_text(anim_label, "Enable Animations");
    lv_obj_set_style_text_font(anim_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(anim_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    animations_switch = lv_switch_create(cont);
    lv_obj_align(animations_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(animations_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_ANIMATIONS);
    y_offset += 50;
    
    // Auto-mount SD setting
    lv_obj_t *mount_label = lv_label_create(cont);
    lv_label_set_text(mount_label, "Auto-mount SD Card");
    lv_obj_set_style_text_font(mount_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(mount_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    auto_mount_switch = lv_switch_create(cont);
    lv_obj_align(auto_mount_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(auto_mount_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_AUTO_MOUNT);
}

static void create_file_browser_tab(lv_obj_t *parent) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_scroll_dir(cont, LV_DIR_VER);
    lv_obj_set_style_pad_all(cont, 20, 0);
    
    int y_offset = 0;
    
    // View mode setting
    lv_obj_t *view_label = lv_label_create(cont);
    lv_label_set_text(view_label, "View Mode");
    lv_obj_set_style_text_font(view_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(view_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    view_mode_dropdown = lv_dropdown_create(cont);
    lv_dropdown_set_options(view_mode_dropdown, "List\nGrid\nDetailed");
    lv_obj_set_size(view_mode_dropdown, 200, 40);
    lv_obj_align(view_mode_dropdown, LV_ALIGN_TOP_LEFT, 0, y_offset + 30);
    lv_obj_add_event_cb(view_mode_dropdown, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_VIEW_MODE);
    y_offset += 80;
    
    // Sort by setting
    lv_obj_t *sort_label = lv_label_create(cont);
    lv_label_set_text(sort_label, "Sort By");
    lv_obj_set_style_text_font(sort_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(sort_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    sort_by_dropdown = lv_dropdown_create(cont);
    lv_dropdown_set_options(sort_by_dropdown, "Name\nSize\nDate\nType");
    lv_obj_set_size(sort_by_dropdown, 200, 40);
    lv_obj_align(sort_by_dropdown, LV_ALIGN_TOP_LEFT, 0, y_offset + 30);
    lv_obj_add_event_cb(sort_by_dropdown, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_SORT_BY);
    y_offset += 80;
    
    // Sort order setting
    lv_obj_t *order_label = lv_label_create(cont);
    lv_label_set_text(order_label, "Ascending Sort");
    lv_obj_set_style_text_font(order_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(order_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    sort_order_switch = lv_switch_create(cont);
    lv_obj_align(sort_order_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(sort_order_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_SORT_ORDER);
    y_offset += 50;
    
    // Show hidden files setting
    lv_obj_t *hidden_label = lv_label_create(cont);
    lv_label_set_text(hidden_label, "Show Hidden Files");
    lv_obj_set_style_text_font(hidden_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(hidden_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    show_hidden_switch = lv_switch_create(cont);
    lv_obj_align(show_hidden_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(show_hidden_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_SHOW_HIDDEN);
    y_offset += 50;
    
    // Show extensions setting
    lv_obj_t *ext_label = lv_label_create(cont);
    lv_label_set_text(ext_label, "Show File Extensions");
    lv_obj_set_style_text_font(ext_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(ext_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    show_extensions_switch = lv_switch_create(cont);
    lv_obj_align(show_extensions_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(show_extensions_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_SHOW_EXTENSIONS);
    y_offset += 50;
    
    // Items per page setting
    lv_obj_t *items_label = lv_label_create(cont);
    lv_label_set_text(items_label, "Items Per Page");
    lv_obj_set_style_text_font(items_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(items_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    items_per_page_slider = lv_slider_create(cont);
    lv_obj_set_size(items_per_page_slider, lv_pct(60), 20);
    lv_obj_align(items_per_page_slider, LV_ALIGN_TOP_LEFT, 0, y_offset + 30);
    lv_slider_set_range(items_per_page_slider, 5, 20);
    lv_obj_add_event_cb(items_per_page_slider, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_ITEMS_PER_PAGE);
}

static void create_theme_tab(lv_obj_t *parent) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(cont, 20, 0);
    
    // Theme selection
    lv_obj_t *theme_label = lv_label_create(cont);
    lv_label_set_text(theme_label, "Color Theme");
    lv_obj_set_style_text_font(theme_label, THEME_FONT_MEDIUM, 0);
    lv_obj_align(theme_label, LV_ALIGN_TOP_LEFT, 0, 20);
    
    theme_dropdown = lv_dropdown_create(cont);
    lv_dropdown_set_options(theme_dropdown, "Dark\nLight\nBlue\nGreen\nCustom");
    lv_obj_set_size(theme_dropdown, 200, 40);
    lv_obj_align(theme_dropdown, LV_ALIGN_TOP_LEFT, 0, 50);
    lv_obj_add_event_cb(theme_dropdown, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_THEME);
    
    // Note: Color picker would be implemented here for custom themes
    lv_obj_t *note_label = lv_label_create(cont);
    lv_label_set_text(note_label, "Custom color options will be available in a future update.");
    lv_obj_set_style_text_font(note_label, THEME_FONT_SMALL, 0);
    lv_obj_align(note_label, LV_ALIGN_TOP_LEFT, 0, 120);
}

static void create_backup_tab(lv_obj_t *parent) {
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(cont, 20, 0);
    
    // Reset to defaults button
    lv_obj_t *reset_btn = lv_btn_create(cont);
    lv_obj_set_size(reset_btn, lv_pct(80), 50);
    lv_obj_align(reset_btn, LV_ALIGN_TOP_MID, 0, 20);
    apply_button_style(reset_btn);
    lv_obj_add_event_cb(reset_btn, settings_event_handler, LV_EVENT_CLICKED,
                        (void*)SETTINGS_RESET_DEFAULTS);
    
    lv_obj_t *reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "Reset to Defaults");
    lv_obj_set_style_text_color(reset_label, lv_color_white(), 0);
    lv_obj_center(reset_label);
    
    // Backup configuration button
    lv_obj_t *backup_btn = lv_btn_create(cont);
    lv_obj_set_size(backup_btn, lv_pct(80), 50);
    lv_obj_align(backup_btn, LV_ALIGN_TOP_MID, 0, 90);
    apply_button_style(backup_btn);
    lv_obj_add_event_cb(backup_btn, settings_event_handler, LV_EVENT_CLICKED,
                        (void*)SETTINGS_BACKUP_CONFIG);
    
    lv_obj_t *backup_label = lv_label_create(backup_btn);
    lv_label_set_text(backup_label, "Backup to SD Card");
    lv_obj_set_style_text_color(backup_label, lv_color_white(), 0);
    lv_obj_center(backup_label);
    
    // Restore configuration button
    lv_obj_t *restore_btn = lv_btn_create(cont);
    lv_obj_set_size(restore_btn, lv_pct(80), 50);
    lv_obj_align(restore_btn, LV_ALIGN_TOP_MID, 0, 160);
    apply_button_style(restore_btn);
    lv_obj_add_event_cb(restore_btn, settings_event_handler, LV_EVENT_CLICKED,
                        (void*)SETTINGS_RESTORE_CONFIG);
    
    lv_obj_t *restore_label = lv_label_create(restore_btn);
    lv_label_set_text(restore_label, "Restore from SD Card");
    lv_obj_set_style_text_color(restore_label, lv_color_white(), 0);
    lv_obj_center(restore_label);
}

static void settings_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    settings_control_id_t control_id = (settings_control_id_t)(uintptr_t)lv_event_get_user_data(e);
    launcher_config_t *config = config_manager_get_current();
    
    if (code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_CLICKED) {
        bool save_needed = true;
        
        switch (control_id) {
            case SETTINGS_BRIGHTNESS:
                config->system.brightness = (uint8_t)lv_slider_get_value(brightness_slider);
                ESP_LOGI(TAG, "Brightness set to %d", config->system.brightness);
                // Apply brightness to hardware
                esp_err_t ret = bsp_display_brightness_set(config->system.brightness);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to set hardware brightness: %s", esp_err_to_name(ret));
                }
                break;
                
            case SETTINGS_TIMEOUT: {
                uint16_t sel = lv_dropdown_get_selected(timeout_dropdown);
                uint8_t timeout_values[] = {0, 1, 2, 5, 10, 15};
                if (sel < sizeof(timeout_values)) {
                    config->system.timeout_minutes = timeout_values[sel];
                    ESP_LOGI(TAG, "Timeout set to %d minutes", config->system.timeout_minutes);
                }
                break;
            }
            
            case SETTINGS_ANIMATIONS:
                config->system.enable_animations = lv_obj_has_state(animations_switch, LV_STATE_CHECKED);
                ESP_LOGI(TAG, "Animations %s", config->system.enable_animations ? "enabled" : "disabled");
                break;
                
            case SETTINGS_AUTO_MOUNT:
                config->system.auto_mount_sd = lv_obj_has_state(auto_mount_switch, LV_STATE_CHECKED);
                ESP_LOGI(TAG, "Auto-mount SD %s", config->system.auto_mount_sd ? "enabled" : "disabled");
                break;
                
            case SETTINGS_VIEW_MODE:
                config->file_browser.view_mode = (view_mode_t)lv_dropdown_get_selected(view_mode_dropdown);
                ESP_LOGI(TAG, "View mode set to %d", config->file_browser.view_mode);
                // Apply to file browser
                gui_file_browser_v2_set_view_mode(config->file_browser.view_mode);
                break;
                
            case SETTINGS_SORT_BY:
                config->file_browser.sort_by = (sort_by_t)lv_dropdown_get_selected(sort_by_dropdown);
                ESP_LOGI(TAG, "Sort by set to %d", config->file_browser.sort_by);
                // Apply to file browser
                gui_file_browser_v2_set_sort_mode(config->file_browser.sort_by, config->file_browser.sort_ascending);
                break;
                
            case SETTINGS_SORT_ORDER:
                config->file_browser.sort_ascending = lv_obj_has_state(sort_order_switch, LV_STATE_CHECKED);
                ESP_LOGI(TAG, "Sort order: %s", config->file_browser.sort_ascending ? "ascending" : "descending");
                // Apply to file browser
                gui_file_browser_v2_set_sort_mode(config->file_browser.sort_by, config->file_browser.sort_ascending);
                break;
                
            case SETTINGS_SHOW_HIDDEN:
                config->file_browser.show_hidden_files = lv_obj_has_state(show_hidden_switch, LV_STATE_CHECKED);
                ESP_LOGI(TAG, "Show hidden files %s", config->file_browser.show_hidden_files ? "enabled" : "disabled");
                // Apply to file browser
                gui_file_browser_v2_show_hidden_files(config->file_browser.show_hidden_files);
                break;
                
            case SETTINGS_SHOW_EXTENSIONS:
                config->file_browser.show_file_extensions = lv_obj_has_state(show_extensions_switch, LV_STATE_CHECKED);
                ESP_LOGI(TAG, "Show extensions %s", config->file_browser.show_file_extensions ? "enabled" : "disabled");
                // Note: Extension display is handled automatically in file browser based on config
                break;
                
            case SETTINGS_ITEMS_PER_PAGE:
                config->file_browser.items_per_page = (uint8_t)lv_slider_get_value(items_per_page_slider);
                ESP_LOGI(TAG, "Items per page set to %d", config->file_browser.items_per_page);
                break;
                
            case SETTINGS_THEME:
                // Theme switching logic would go here
                ESP_LOGI(TAG, "Theme changed to %" PRIu32, lv_dropdown_get_selected(theme_dropdown));
                break;
                
            case SETTINGS_RESET_DEFAULTS:
                ESP_LOGI(TAG, "Resetting configuration to defaults");
                config_manager_reset_defaults(config);
                apply_current_config_to_ui();
                break;
                
            case SETTINGS_BACKUP_CONFIG:
                ESP_LOGI(TAG, "Backing up configuration to SD card");
                if (config_manager_backup_to_sd("/launcher_backup.json") == ESP_OK) {
                    ESP_LOGI(TAG, "Backup successful");
                } else {
                    ESP_LOGE(TAG, "Backup failed");
                }
                save_needed = false; // Don't save twice
                break;
                
            case SETTINGS_RESTORE_CONFIG:
                ESP_LOGI(TAG, "Restoring configuration from SD card");
                if (config_manager_restore_from_sd("/launcher_backup.json") == ESP_OK) {
                    ESP_LOGI(TAG, "Restore successful");
                    apply_current_config_to_ui();
                } else {
                    ESP_LOGE(TAG, "Restore failed");
                }
                save_needed = false; // Don't save since we just restored
                break;
                
            default:
                save_needed = false;
                break;
        }
        
        if (save_needed) {
            save_settings();
        }
    }
}

static void apply_current_config_to_ui(void) {
    launcher_config_t *config = config_manager_get_current();
    
    if (!config) {
        ESP_LOGE(TAG, "Failed to get current configuration");
        return;
    }
    
    // Apply system settings
    if (brightness_slider) {
        lv_slider_set_value(brightness_slider, config->system.brightness, LV_ANIM_OFF);
    }
    
    if (timeout_dropdown) {
        // Map timeout value to dropdown index
        uint8_t timeout_values[] = {0, 1, 2, 5, 10, 15};
        for (int i = 0; i < sizeof(timeout_values); i++) {
            if (timeout_values[i] == config->system.timeout_minutes) {
                lv_dropdown_set_selected(timeout_dropdown, i);
                break;
            }
        }
    }
    
    if (animations_switch) {
        if (config->system.enable_animations) {
            lv_obj_add_state(animations_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(animations_switch, LV_STATE_CHECKED);
        }
    }
    
    if (auto_mount_switch) {
        if (config->system.auto_mount_sd) {
            lv_obj_add_state(auto_mount_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(auto_mount_switch, LV_STATE_CHECKED);
        }
    }
    
    // Apply file browser settings
    if (view_mode_dropdown) {
        lv_dropdown_set_selected(view_mode_dropdown, config->file_browser.view_mode);
    }
    
    if (sort_by_dropdown) {
        lv_dropdown_set_selected(sort_by_dropdown, config->file_browser.sort_by);
    }
    
    if (sort_order_switch) {
        if (config->file_browser.sort_ascending) {
            lv_obj_add_state(sort_order_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(sort_order_switch, LV_STATE_CHECKED);
        }
    }
    
    if (show_hidden_switch) {
        if (config->file_browser.show_hidden_files) {
            lv_obj_add_state(show_hidden_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(show_hidden_switch, LV_STATE_CHECKED);
        }
    }
    
    if (show_extensions_switch) {
        if (config->file_browser.show_file_extensions) {
            lv_obj_add_state(show_extensions_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(show_extensions_switch, LV_STATE_CHECKED);
        }
    }
    
    if (items_per_page_slider) {
        lv_slider_set_value(items_per_page_slider, config->file_browser.items_per_page, LV_ANIM_OFF);
    }
    
    ESP_LOGI(TAG, "Applied current configuration to UI");
}

static void save_settings(void) {
    launcher_config_t *config = config_manager_get_current();
    esp_err_t ret = config_manager_save(config);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Settings saved successfully");
    } else {
        ESP_LOGE(TAG, "Failed to save settings: %s", esp_err_to_name(ret));
    }
}

lv_obj_t* get_settings_screen(void) {
    return settings_screen;
}

void destroy_settings_screen(void) {
    if (settings_screen) {
        lv_obj_del(settings_screen);
        settings_screen = NULL;
        ESP_LOGI(TAG, "Settings screen destroyed");
    }
}