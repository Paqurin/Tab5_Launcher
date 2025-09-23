#include "gui_screen_settings.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "gui_status_bar.h"
#include "gui_file_browser_v2.h"
#include "gui_animation_manager.h"
#include "config_manager.h"
#include "bsp/m5stack_tab5.h"
#include "esp_log.h"
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "M5Unified.h"

static const char *TAG = "GUI_SETTINGS";

// Screen objects (declared extern in gui_screens.h)
lv_obj_t *settings_screen = NULL;
// static lv_obj_t *settings_container = NULL;
static lv_obj_t *tabview = NULL;
static gui_status_bar_t *status_bar = NULL;

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

// Date/Time settings controls
static lv_obj_t *year_spinbox = NULL;
static lv_obj_t *month_spinbox = NULL;
static lv_obj_t *day_spinbox = NULL;
static lv_obj_t *hour_spinbox = NULL;
static lv_obj_t *minute_spinbox = NULL;

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
    SETTINGS_SET_TIME,
    SETTINGS_YEAR_SPINBOX,
    SETTINGS_MONTH_SPINBOX,
    SETTINGS_DAY_SPINBOX,
    SETTINGS_HOUR_SPINBOX,
    SETTINGS_MINUTE_SPINBOX,
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

// Spinbox button event handlers
/* Unused event handlers - commenting out to fix compilation warnings
static void year_inc_event_handler(lv_event_t *e) {
    lv_spinbox_increment(year_spinbox);
    lv_event_send(year_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void year_dec_event_handler(lv_event_t *e) {
    lv_spinbox_decrement(year_spinbox);
    lv_event_send(year_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void month_inc_event_handler(lv_event_t *e) {
    lv_spinbox_increment(month_spinbox);
    lv_event_send(month_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void month_dec_event_handler(lv_event_t *e) {
    lv_spinbox_decrement(month_spinbox);
    lv_event_send(month_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void day_inc_event_handler(lv_event_t *e) {
    lv_spinbox_increment(day_spinbox);
    lv_event_send(day_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void day_dec_event_handler(lv_event_t *e) {
    lv_spinbox_decrement(day_spinbox);
    lv_event_send(day_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void hour_inc_event_handler(lv_event_t *e) {
    lv_spinbox_increment(hour_spinbox);
    lv_event_send(hour_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void hour_dec_event_handler(lv_event_t *e) {
    lv_spinbox_decrement(hour_spinbox);
    lv_event_send(hour_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void minute_inc_event_handler(lv_event_t *e) {
    lv_spinbox_increment(minute_spinbox);
    lv_event_send(minute_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}

static void minute_dec_event_handler(lv_event_t *e) {
    lv_spinbox_decrement(minute_spinbox);
    lv_event_send(minute_spinbox, LV_EVENT_VALUE_CHANGED, NULL);
}
*/

void create_settings_screen(void) {
    if (settings_screen) {
        return; // Already created
    }
    
    ESP_LOGI(TAG, "Creating settings screen");
    
    // Create main screen
    settings_screen = lv_obj_create(NULL);
    lv_obj_add_style(settings_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Status bar is now global - no individual creation needed
    
    // Create a centered container for the screen (below status bar)
    lv_obj_t *center_container = lv_obj_create(settings_screen);
    lv_obj_set_size(center_container, lv_pct(80), lv_pct(85));
    lv_obj_align(center_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(center_container, 10, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(center_container);
    lv_label_set_text(title, "Settings");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(center_container);
    lv_obj_set_size(back_btn, 100, 50);
    lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 35);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Create tabview in the center container
    tabview = lv_tabview_create(center_container);
    lv_obj_set_size(tabview, lv_pct(98), lv_pct(85));
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tabview, 70);

    // Apply consistent styling to match project theme
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x2E2E2E), LV_PART_MAIN);
    lv_obj_set_style_border_color(tabview, lv_color_hex(0x4A90E2), LV_PART_MAIN);
    lv_obj_set_style_border_width(tabview, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(tabview, 12, LV_PART_MAIN);

    // Style tab buttons to match project buttons
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x2E2E2E), LV_PART_ITEMS);
    lv_obj_set_style_border_color(tabview, lv_color_hex(0x4A90E2), LV_PART_ITEMS);
    lv_obj_set_style_border_width(tabview, 2, LV_PART_ITEMS);
    lv_obj_set_style_text_color(tabview, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_text_font(tabview, &lv_font_montserrat_24, LV_PART_ITEMS);
    
    // Create tabs
    lv_obj_t *system_tab = lv_tabview_add_tab(tabview, "System");
    lv_obj_t *browser_tab = lv_tabview_add_tab(tabview, "File Browser");
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
    apply_text_style(brightness_label);
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
    apply_text_style(timeout_label);
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
    apply_text_style(anim_label);
    lv_obj_align(anim_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    animations_switch = lv_switch_create(cont);
    lv_obj_align(animations_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(animations_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_ANIMATIONS);
    y_offset += 50;
    
    // Auto-mount SD setting
    lv_obj_t *mount_label = lv_label_create(cont);
    lv_label_set_text(mount_label, "Auto-mount SD Card");
    apply_text_style(mount_label);
    lv_obj_align(mount_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    auto_mount_switch = lv_switch_create(cont);
    lv_obj_align(auto_mount_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(auto_mount_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_AUTO_MOUNT);
    y_offset += 60;

    // Date & Time setting section
    lv_obj_t *datetime_label = lv_label_create(cont);
    lv_label_set_text(datetime_label, "Date & Time");
    apply_text_style(datetime_label);
    lv_obj_align(datetime_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    y_offset += 40;

    // Get current time from RTC to populate fields
    m5::rtc_datetime_t current_datetime;
    bool rtc_valid = M5.Rtc.getDateTime(&current_datetime);

    // If RTC read fails, use system time as fallback
    struct tm timeinfo_storage;
    struct tm *timeinfo;
    if (rtc_valid) {
        // Use RTC time
        timeinfo_storage.tm_year = current_datetime.date.year - 1900;
        timeinfo_storage.tm_mon = current_datetime.date.month - 1;
        timeinfo_storage.tm_mday = current_datetime.date.date;
        timeinfo_storage.tm_hour = current_datetime.time.hours;
        timeinfo_storage.tm_min = current_datetime.time.minutes;
        timeinfo_storage.tm_sec = current_datetime.time.seconds;
        timeinfo = &timeinfo_storage;
        ESP_LOGI(TAG, "Using RTC time for settings initialization");
    } else {
        // Fallback to system time
        time_t now;
        time(&now);
        timeinfo = localtime(&now);
        ESP_LOGW(TAG, "RTC read failed, using system time for settings initialization");
    }

    // Date section with labels
    lv_obj_t *date_section_label = lv_label_create(cont);
    lv_label_set_text(date_section_label, "Date:");
    apply_text_style(date_section_label);
    lv_obj_align(date_section_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    y_offset += 35;

    // Create date labels row
    lv_obj_t *date_labels_cont = lv_obj_create(cont);
    lv_obj_set_size(date_labels_cont, lv_pct(100), 30);
    lv_obj_align(date_labels_cont, LV_ALIGN_TOP_LEFT, 0, y_offset);
    lv_obj_set_style_bg_opa(date_labels_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(date_labels_cont, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(date_labels_cont, 0, 0);
    lv_obj_set_flex_flow(date_labels_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(date_labels_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Year label
    lv_obj_t *year_label = lv_label_create(date_labels_cont);
    lv_label_set_text(year_label, "Year");
    lv_obj_set_style_text_font(year_label, THEME_FONT_SMALL, 0);
    lv_obj_set_style_text_color(year_label, THEME_TEXT_MUTED, 0);
    lv_obj_set_width(year_label, 120);

    // Month label
    lv_obj_t *month_label = lv_label_create(date_labels_cont);
    lv_label_set_text(month_label, "Month");
    lv_obj_set_style_text_font(month_label, THEME_FONT_SMALL, 0);
    lv_obj_set_style_text_color(month_label, THEME_TEXT_MUTED, 0);
    lv_obj_set_width(month_label, 100);

    // Day label
    lv_obj_t *day_label = lv_label_create(date_labels_cont);
    lv_label_set_text(day_label, "Day");
    lv_obj_set_style_text_font(day_label, THEME_FONT_SMALL, 0);
    lv_obj_set_style_text_color(day_label, THEME_TEXT_MUTED, 0);
    lv_obj_set_width(day_label, 100);

    y_offset += 35;

    // Create date spinboxes row
    lv_obj_t *date_cont = lv_obj_create(cont);
    lv_obj_set_size(date_cont, lv_pct(100), 80);
    lv_obj_align(date_cont, LV_ALIGN_TOP_LEFT, 0, y_offset);
    lv_obj_set_style_bg_opa(date_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(date_cont, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(date_cont, 0, 0);
    lv_obj_set_flex_flow(date_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(date_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Year spinbox
    year_spinbox = lv_spinbox_create(date_cont);
    lv_spinbox_set_range(year_spinbox, 2020, 2100);
    lv_spinbox_set_value(year_spinbox, timeinfo->tm_year + 1900);
    lv_obj_set_size(year_spinbox, 180, 70);
    lv_obj_set_style_text_font(year_spinbox, THEME_FONT_MEDIUM, 0);
    // Ensure spinbox arrows are visible
    lv_obj_set_style_bg_color(year_spinbox, lv_color_hex(0x333333), LV_PART_KNOB);
    lv_obj_set_style_text_color(year_spinbox, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(year_spinbox, settings_event_handler, LV_EVENT_VALUE_CHANGED, (void*)SETTINGS_YEAR_SPINBOX);

    // Month spinbox
    month_spinbox = lv_spinbox_create(date_cont);
    lv_spinbox_set_range(month_spinbox, 1, 12);
    lv_spinbox_set_value(month_spinbox, timeinfo->tm_mon + 1);
    lv_obj_set_size(month_spinbox, 160, 70);
    lv_obj_set_style_text_font(month_spinbox, THEME_FONT_MEDIUM, 0);
    // Ensure spinbox arrows are visible
    lv_obj_set_style_bg_color(month_spinbox, lv_color_hex(0x333333), LV_PART_KNOB);
    lv_obj_set_style_text_color(month_spinbox, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(month_spinbox, settings_event_handler, LV_EVENT_VALUE_CHANGED, (void*)SETTINGS_MONTH_SPINBOX);

    // Day spinbox
    day_spinbox = lv_spinbox_create(date_cont);
    lv_spinbox_set_range(day_spinbox, 1, 31);
    lv_spinbox_set_value(day_spinbox, timeinfo->tm_mday);
    lv_obj_set_size(day_spinbox, 160, 70);
    lv_obj_set_style_text_font(day_spinbox, THEME_FONT_MEDIUM, 0);
    // Ensure spinbox arrows are visible
    lv_obj_set_style_bg_color(day_spinbox, lv_color_hex(0x333333), LV_PART_KNOB);
    lv_obj_set_style_text_color(day_spinbox, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(day_spinbox, settings_event_handler, LV_EVENT_VALUE_CHANGED, (void*)SETTINGS_DAY_SPINBOX);

    y_offset += 90;

    // Time section with labels
    lv_obj_t *time_section_label = lv_label_create(cont);
    lv_label_set_text(time_section_label, "Time:");
    apply_text_style(time_section_label);
    lv_obj_align(time_section_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    y_offset += 35;

    // Create time labels row
    lv_obj_t *time_labels_cont = lv_obj_create(cont);
    lv_obj_set_size(time_labels_cont, lv_pct(100), 30);
    lv_obj_align(time_labels_cont, LV_ALIGN_TOP_LEFT, 0, y_offset);
    lv_obj_set_style_bg_opa(time_labels_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(time_labels_cont, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(time_labels_cont, 0, 0);
    lv_obj_set_flex_flow(time_labels_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_labels_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Hour label
    lv_obj_t *hour_label = lv_label_create(time_labels_cont);
    lv_label_set_text(hour_label, "Hour");
    lv_obj_set_style_text_font(hour_label, THEME_FONT_SMALL, 0);
    lv_obj_set_style_text_color(hour_label, THEME_TEXT_MUTED, 0);
    lv_obj_set_width(hour_label, 100);

    // Minute label
    lv_obj_t *minute_label = lv_label_create(time_labels_cont);
    lv_label_set_text(minute_label, "Minute");
    lv_obj_set_style_text_font(minute_label, THEME_FONT_SMALL, 0);
    lv_obj_set_style_text_color(minute_label, THEME_TEXT_MUTED, 0);
    lv_obj_set_width(minute_label, 100);

    y_offset += 35;

    // Create time spinboxes and button row
    lv_obj_t *time_cont = lv_obj_create(cont);
    lv_obj_set_size(time_cont, lv_pct(100), 80);
    lv_obj_align(time_cont, LV_ALIGN_TOP_LEFT, 0, y_offset);
    lv_obj_set_style_bg_opa(time_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_opa(time_cont, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(time_cont, 0, 0);
    lv_obj_set_flex_flow(time_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Hour spinbox
    hour_spinbox = lv_spinbox_create(time_cont);
    lv_spinbox_set_range(hour_spinbox, 0, 23);
    lv_spinbox_set_value(hour_spinbox, timeinfo->tm_hour);
    lv_obj_set_size(hour_spinbox, 160, 70);
    lv_obj_set_style_text_font(hour_spinbox, THEME_FONT_MEDIUM, 0);
    // Ensure spinbox arrows are visible
    lv_obj_set_style_bg_color(hour_spinbox, lv_color_hex(0x333333), LV_PART_KNOB);
    lv_obj_set_style_text_color(hour_spinbox, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(hour_spinbox, settings_event_handler, LV_EVENT_VALUE_CHANGED, (void*)SETTINGS_HOUR_SPINBOX);

    // Minute spinbox
    minute_spinbox = lv_spinbox_create(time_cont);
    lv_spinbox_set_range(minute_spinbox, 0, 59);
    lv_spinbox_set_value(minute_spinbox, timeinfo->tm_min);
    lv_obj_set_size(minute_spinbox, 160, 70);
    lv_obj_set_style_text_font(minute_spinbox, THEME_FONT_MEDIUM, 0);
    // Ensure spinbox arrows are visible
    lv_obj_set_style_bg_color(minute_spinbox, lv_color_hex(0x333333), LV_PART_KNOB);
    lv_obj_set_style_text_color(minute_spinbox, lv_color_white(), LV_PART_KNOB);
    lv_obj_add_event_cb(minute_spinbox, settings_event_handler, LV_EVENT_VALUE_CHANGED, (void*)SETTINGS_MINUTE_SPINBOX);

    // Set time button
    lv_obj_t *set_time_btn = lv_btn_create(time_cont);
    lv_obj_set_size(set_time_btn, 120, 70);
    apply_button_style(set_time_btn);
    lv_obj_add_event_cb(set_time_btn, settings_event_handler, LV_EVENT_CLICKED, (void*)SETTINGS_SET_TIME);

    lv_obj_t *set_time_label = lv_label_create(set_time_btn);
    lv_label_set_text(set_time_label, "Set Time");
    lv_obj_set_style_text_color(set_time_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(set_time_label, THEME_FONT_NORMAL, 0);
    lv_obj_center(set_time_label);

    y_offset += 90;
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
    apply_text_style(view_label);
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
    apply_text_style(sort_label);
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
    apply_text_style(order_label);
    lv_obj_align(order_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    sort_order_switch = lv_switch_create(cont);
    lv_obj_align(sort_order_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(sort_order_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_SORT_ORDER);
    y_offset += 50;
    
    // Show hidden files setting
    lv_obj_t *hidden_label = lv_label_create(cont);
    lv_label_set_text(hidden_label, "Show Hidden Files");
    apply_text_style(hidden_label);
    lv_obj_align(hidden_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    show_hidden_switch = lv_switch_create(cont);
    lv_obj_align(show_hidden_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(show_hidden_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_SHOW_HIDDEN);
    y_offset += 50;
    
    // Show extensions setting
    lv_obj_t *ext_label = lv_label_create(cont);
    lv_label_set_text(ext_label, "Show File Extensions");
    apply_text_style(ext_label);
    lv_obj_align(ext_label, LV_ALIGN_TOP_LEFT, 0, y_offset);
    
    show_extensions_switch = lv_switch_create(cont);
    lv_obj_align(show_extensions_switch, LV_ALIGN_TOP_RIGHT, -20, y_offset);
    lv_obj_add_event_cb(show_extensions_switch, settings_event_handler, LV_EVENT_VALUE_CHANGED,
                        (void*)SETTINGS_SHOW_EXTENSIONS);
    y_offset += 50;
    
    // Items per page setting
    lv_obj_t *items_label = lv_label_create(cont);
    lv_label_set_text(items_label, "Items Per Page");
    apply_text_style(items_label);
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
    apply_text_style(theme_label);
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
    apply_text_muted_style(note_label);
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
            case SETTINGS_BRIGHTNESS: {
                config->system.brightness = (uint8_t)lv_slider_get_value(brightness_slider);
                ESP_LOGI(TAG, "Brightness set to %d", config->system.brightness);
                // Apply brightness to hardware
                esp_err_t ret = bsp_display_brightness_set(config->system.brightness);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to set hardware brightness: %s", esp_err_to_name(ret));
                }
                break;
            }
                
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

                // Update global animation manager immediately
                gui_animation_manager_set_enabled(config->system.enable_animations);
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

            case SETTINGS_SET_TIME: {
                ESP_LOGI(TAG, "Set time button clicked");

                // Get values from spinboxes and create RTC datetime structure
                m5::rtc_datetime_t new_datetime;
                new_datetime.date.year = lv_spinbox_get_value(year_spinbox);
                new_datetime.date.month = lv_spinbox_get_value(month_spinbox);
                new_datetime.date.date = lv_spinbox_get_value(day_spinbox);
                new_datetime.time.hours = lv_spinbox_get_value(hour_spinbox);
                new_datetime.time.minutes = lv_spinbox_get_value(minute_spinbox);
                new_datetime.time.seconds = 0; // Set seconds to 0

                // Calculate weekday (0=Sunday, 6=Saturday)
                struct tm temp_time = {};
                temp_time.tm_year = new_datetime.date.year - 1900;
                temp_time.tm_mon = new_datetime.date.month - 1;
                temp_time.tm_mday = new_datetime.date.date;
                temp_time.tm_hour = new_datetime.time.hours;
                temp_time.tm_min = new_datetime.time.minutes;
                temp_time.tm_sec = new_datetime.time.seconds;
                temp_time.tm_isdst = -1;
                time_t timestamp = mktime(&temp_time);
                new_datetime.date.weekDay = temp_time.tm_wday;

                // Set RTC time using M5Unified
                M5.Rtc.setDateTime(new_datetime);
                ESP_LOGI(TAG, "RTC time set successfully");

                // Also update system time to match RTC
                if (timestamp != -1) {
                    struct timeval tv = { .tv_sec = timestamp, .tv_usec = 0 };
                    settimeofday(&tv, NULL);
                    ESP_LOGI(TAG, "System time synchronized with RTC");
                }
                save_needed = false; // Don't save config for time changes
                break;
            }

            case SETTINGS_YEAR_SPINBOX:
            case SETTINGS_MONTH_SPINBOX:
            case SETTINGS_DAY_SPINBOX:
            case SETTINGS_HOUR_SPINBOX:
            case SETTINGS_MINUTE_SPINBOX:
                // Spinbox value changed - no immediate action needed
                // Values will be read when "Set" button is clicked
                ESP_LOGD(TAG, "Date/time spinbox value changed");
                save_needed = false; // Don't save config for spinbox changes
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
        lv_slider_set_value(brightness_slider, config->system.brightness, gui_animation_manager_get_anim_flag());
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
        lv_slider_set_value(items_per_page_slider, config->file_browser.items_per_page, gui_animation_manager_get_anim_flag());
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

void update_settings_status_bar(float voltage, float current_ma, bool charging) {
    if (status_bar) {
        gui_status_bar_update_power(status_bar, voltage, current_ma, charging);
        gui_status_bar_update_sdcard(status_bar);
        // WiFi update would go here when WiFi manager is available
        // gui_status_bar_update_wifi(status_bar, wifi_connected, rssi);
    }
}

lv_obj_t* get_settings_screen(void) {
    return settings_screen;
}

void destroy_settings_screen(void) {
    if (settings_screen) {
        ESP_LOGI(TAG, "Destroying settings screen...");

        // Disable events on screen to prevent corruption during deletion
        lv_obj_remove_event_cb(settings_screen, NULL);

        // Remove all event callbacks from child objects to prevent dangling pointers
        uint32_t child_cnt = lv_obj_get_child_count(settings_screen);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(settings_screen, i);
            if (child) {
                lv_obj_remove_event_cb(child, NULL);
            }
        }

        // Now safely delete the screen
        lv_obj_del(settings_screen);
        settings_screen = NULL;
        ESP_LOGI(TAG, "Settings screen destroyed");
    }
}