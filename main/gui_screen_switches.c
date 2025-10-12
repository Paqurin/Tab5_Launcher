#include "gui_screen_switches.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "hardware_control.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GUI_SWITCHES";

lv_obj_t *switches_screen = NULL;

// Switch UI objects
static lv_obj_t *charge_enable_switch = NULL;
static lv_obj_t *charge_qc_enable_switch = NULL;
static lv_obj_t *usb_5v_enable_switch = NULL;
static lv_obj_t *ext_5v_enable_switch = NULL;
static lv_obj_t *ext_antenna_enable_switch = NULL;

// Detection indicator objects
static lv_obj_t *usb_c_indicator = NULL;
static lv_obj_t *usb_a_indicator = NULL;
static lv_obj_t *headphone_indicator = NULL;

// Update timer and state tracking
static lv_timer_t *update_timer = NULL;
static hardware_detection_t last_detection = {0};

// Status message area
static lv_obj_t *switches_status_label = NULL;

// Forward declarations
static void update_switch_states(void);
static void update_detection_indicators(void);
static void update_timer_callback(lv_timer_t *timer);
static void show_status_message(const char *message, lv_color_t color);

// Event handlers for switches
static void charge_enable_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool state = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        hardware_set_charge_enable(state);
        show_status_message(state ? "Battery charging enabled" : "Battery charging disabled",
                          state ? lv_color_hex(0x27ae60) : lv_color_hex(0x666666));
        ESP_LOGI(TAG, "Charge enable: %s", state ? "ON" : "OFF");
    }
}

static void charge_qc_enable_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool state = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        hardware_set_charge_qc_enable(state);
        show_status_message(state ? "QC charging enabled" : "QC charging disabled",
                          state ? lv_color_hex(0xf39c12) : lv_color_hex(0x3498db));
        ESP_LOGI(TAG, "Charge QC enable: %s", state ? "ON" : "OFF");
    }
}

static void usb_5v_enable_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool state = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        hardware_set_usb_5v_enable(state);
        show_status_message(state ? "USB-A 5V enabled" : "USB-A 5V disabled",
                          state ? lv_color_hex(0xe74c3c) : lv_color_hex(0x2ecc71));
        ESP_LOGI(TAG, "USB 5V enable: %s", state ? "ON" : "OFF");
    }
}

static void ext_5v_enable_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool state = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        hardware_set_ext_5v_enable(state);
        show_status_message(state ? "EXT 5V enabled" : "EXT 5V disabled",
                          state ? lv_color_hex(0xe74c3c) : lv_color_hex(0x2ecc71));
        ESP_LOGI(TAG, "External 5V enable: %s", state ? "ON" : "OFF");
    }
}

static void ext_antenna_enable_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        bool state = lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED);
        hardware_set_ext_antenna_enable(state);
        show_status_message(state ? "Switched to external antenna" : "Switched to internal antenna",
                          lv_color_hex(0x3498db));
        ESP_LOGI(TAG, "External antenna enable: %s", state ? "ON" : "OFF");
    }
}

static void switches_back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        switches_screen_back();
    }
}

void create_switches_screen(void) {
    if (switches_screen) {
        return; // Already created
    }

    // Initialize hardware control system
    hardware_control_init();

    switches_screen = lv_obj_create(NULL);
    lv_obj_add_style(switches_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(switches_screen);
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
    lv_obj_add_event_cb(back_btn, switches_back_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    // Title
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Hardware Switches");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // Create main container for switches
    lv_obj_t *switches_container = lv_obj_create(switches_screen);
    lv_obj_set_size(switches_container, lv_pct(90), lv_pct(75));
    lv_obj_align(switches_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(switches_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(switches_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(switches_container, 20, 0);

    // Create switch controls in 2x3 grid layout

    // Row 1: External Antenna and Quick Charge
    lv_obj_t *row1_container = lv_obj_create(switches_container);
    lv_obj_set_size(row1_container, lv_pct(100), 80);
    lv_obj_align(row1_container, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_opa(row1_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(row1_container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(row1_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row1_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // External Antenna Switch
    lv_obj_t *ext_antenna_panel = lv_obj_create(row1_container);
    lv_obj_set_size(ext_antenna_panel, 300, 70);
    lv_obj_set_style_bg_color(ext_antenna_panel, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_radius(ext_antenna_panel, 10, 0);

    lv_obj_t *ext_antenna_label = lv_label_create(ext_antenna_panel);
    lv_label_set_text(ext_antenna_label, LV_SYMBOL_WIFI " External Antenna");
    lv_obj_set_style_text_color(ext_antenna_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ext_antenna_label, LV_ALIGN_LEFT_MID, 15, 0);

    ext_antenna_enable_switch = lv_switch_create(ext_antenna_panel);
    lv_obj_align(ext_antenna_enable_switch, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_obj_add_event_cb(ext_antenna_enable_switch, ext_antenna_enable_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Quick Charge Switch
    lv_obj_t *charge_qc_panel = lv_obj_create(row1_container);
    lv_obj_set_size(charge_qc_panel, 300, 70);
    lv_obj_set_style_bg_color(charge_qc_panel, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_radius(charge_qc_panel, 10, 0);

    lv_obj_t *charge_qc_label = lv_label_create(charge_qc_panel);
    lv_label_set_text(charge_qc_label, LV_SYMBOL_CHARGE " Quick Charge");
    lv_obj_set_style_text_color(charge_qc_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(charge_qc_label, LV_ALIGN_LEFT_MID, 15, 0);

    charge_qc_enable_switch = lv_switch_create(charge_qc_panel);
    lv_obj_align(charge_qc_enable_switch, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_obj_add_event_cb(charge_qc_enable_switch, charge_qc_enable_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Row 2: USB-A 5V and Battery Charging
    lv_obj_t *row2_container = lv_obj_create(switches_container);
    lv_obj_set_size(row2_container, lv_pct(100), 80);
    lv_obj_align(row2_container, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_set_style_bg_opa(row2_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(row2_container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(row2_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row2_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // USB-A 5V Switch
    lv_obj_t *usb_5v_panel = lv_obj_create(row2_container);
    lv_obj_set_size(usb_5v_panel, 300, 70);
    lv_obj_set_style_bg_color(usb_5v_panel, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_radius(usb_5v_panel, 10, 0);

    lv_obj_t *usb_5v_label = lv_label_create(usb_5v_panel);
    lv_label_set_text(usb_5v_label, LV_SYMBOL_USB " USB-A 5V");
    lv_obj_set_style_text_color(usb_5v_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(usb_5v_label, LV_ALIGN_LEFT_MID, 15, 0);

    usb_5v_enable_switch = lv_switch_create(usb_5v_panel);
    lv_obj_align(usb_5v_enable_switch, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_obj_add_event_cb(usb_5v_enable_switch, usb_5v_enable_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Battery Charging Switch
    lv_obj_t *charge_panel = lv_obj_create(row2_container);
    lv_obj_set_size(charge_panel, 300, 70);
    lv_obj_set_style_bg_color(charge_panel, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_radius(charge_panel, 10, 0);

    lv_obj_t *charge_label = lv_label_create(charge_panel);
    lv_label_set_text(charge_label, LV_SYMBOL_BATTERY_3 " Battery Charging");
    lv_obj_set_style_text_color(charge_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(charge_label, LV_ALIGN_LEFT_MID, 15, 0);

    charge_enable_switch = lv_switch_create(charge_panel);
    lv_obj_align(charge_enable_switch, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_obj_add_event_cb(charge_enable_switch, charge_enable_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Row 3: External 5V (centered)
    lv_obj_t *ext_5v_panel = lv_obj_create(switches_container);
    lv_obj_set_size(ext_5v_panel, 300, 70);
    lv_obj_align(ext_5v_panel, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_bg_color(ext_5v_panel, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_radius(ext_5v_panel, 10, 0);

    lv_obj_t *ext_5v_label = lv_label_create(ext_5v_panel);
    lv_label_set_text(ext_5v_label, LV_SYMBOL_POWER " External 5V");
    lv_obj_set_style_text_color(ext_5v_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(ext_5v_label, LV_ALIGN_LEFT_MID, 15, 0);

    ext_5v_enable_switch = lv_switch_create(ext_5v_panel);
    lv_obj_align(ext_5v_enable_switch, LV_ALIGN_RIGHT_MID, -15, 0);
    lv_obj_add_event_cb(ext_5v_enable_switch, ext_5v_enable_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // Detection indicators section
    lv_obj_t *detection_container = lv_obj_create(switches_screen);
    lv_obj_set_size(detection_container, lv_pct(90), 60);
    lv_obj_align(detection_container, LV_ALIGN_BOTTOM_MID, 0, -70);
    lv_obj_set_style_bg_color(detection_container, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_radius(detection_container, 10, 0);
    lv_obj_set_style_pad_all(detection_container, 10, 0);
    lv_obj_set_flex_flow(detection_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(detection_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // USB-C Detection
    usb_c_indicator = lv_label_create(detection_container);
    lv_label_set_text(usb_c_indicator, LV_SYMBOL_USB " USB-C");
    lv_obj_set_style_text_color(usb_c_indicator, lv_color_hex(0x95a5a6), 0);

    // USB-A Detection
    usb_a_indicator = lv_label_create(detection_container);
    lv_label_set_text(usb_a_indicator, LV_SYMBOL_USB " USB-A");
    lv_obj_set_style_text_color(usb_a_indicator, lv_color_hex(0x95a5a6), 0);

    // Headphone Detection
    headphone_indicator = lv_label_create(detection_container);
    lv_label_set_text(headphone_indicator, LV_SYMBOL_AUDIO " Headphone");
    lv_obj_set_style_text_color(headphone_indicator, lv_color_hex(0x95a5a6), 0);

    // Status message area
    switches_status_label = lv_label_create(switches_screen);
    lv_label_set_text(switches_status_label, "Hardware Switches Ready");
    lv_obj_set_style_text_color(switches_status_label, lv_color_hex(0xBDC3C7), 0);
    lv_obj_set_style_text_font(switches_status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(switches_status_label, LV_ALIGN_BOTTOM_MID, 0, -10);

    // Initialize switch states from hardware
    update_switch_states();
    update_detection_indicators();

    // Start update timer for detection monitoring
    update_timer = lv_timer_create(update_timer_callback, 200, NULL);
}

void show_switches_screen(void) {
    if (!switches_screen) {
        create_switches_screen();
    }
    lv_screen_load_anim(switches_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void switches_screen_back(void) {
    ESP_LOGI(TAG, "Returning to main screen");
    // Load main screen before destroying to prevent active screen deletion
    lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    // Now safe to clean up switches screen after switching away
    destroy_switches_screen();
}

void destroy_switches_screen(void) {
    if (switches_screen) {
        ESP_LOGI(TAG, "Destroying switches screen...");

        // Stop update timer
        if (update_timer) {
            lv_timer_del(update_timer);
            update_timer = NULL;
        }

        // DEFENSIVE: Check screen validity before destruction
        if (!lv_obj_is_valid(switches_screen)) {
            ESP_LOGW(TAG, "Switches screen is invalid, setting to NULL");
            switches_screen = NULL;
            return;
        }

        // DEFENSIVE: Check if screen is currently active
        if (lv_screen_active() == switches_screen) {
            ESP_LOGW(TAG, "Warning: Destroying active screen - this should not happen");
        }

        // Disable events on screen to prevent corruption during deletion
        lv_obj_remove_event_cb(switches_screen, NULL);

        // Remove all event callbacks from child objects to prevent dangling pointers
        uint32_t child_cnt = lv_obj_get_child_count(switches_screen);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(switches_screen, i);
            if (child && lv_obj_is_valid(child)) {
                lv_obj_remove_event_cb(child, NULL);
            }
        }

        // Reset all object pointers
        charge_enable_switch = NULL;
        charge_qc_enable_switch = NULL;
        usb_5v_enable_switch = NULL;
        ext_5v_enable_switch = NULL;
        ext_antenna_enable_switch = NULL;
        usb_c_indicator = NULL;
        usb_a_indicator = NULL;
        headphone_indicator = NULL;
        switches_status_label = NULL;

        // Now safely delete the screen
        lv_obj_del(switches_screen);
        switches_screen = NULL;
        ESP_LOGI(TAG, "Switches screen destroyed");
    }
}

// Helper function implementations
static void update_switch_states(void) {
    if (!switches_screen) return;

    // Update switch UI states to match hardware states
    if (charge_enable_switch) {
        if (hardware_get_charge_enable()) {
            lv_obj_add_state(charge_enable_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(charge_enable_switch, LV_STATE_CHECKED);
        }
    }

    if (charge_qc_enable_switch) {
        if (hardware_get_charge_qc_enable()) {
            lv_obj_add_state(charge_qc_enable_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(charge_qc_enable_switch, LV_STATE_CHECKED);
        }
    }

    if (usb_5v_enable_switch) {
        if (hardware_get_usb_5v_enable()) {
            lv_obj_add_state(usb_5v_enable_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(usb_5v_enable_switch, LV_STATE_CHECKED);
        }
    }

    if (ext_5v_enable_switch) {
        if (hardware_get_ext_5v_enable()) {
            lv_obj_add_state(ext_5v_enable_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(ext_5v_enable_switch, LV_STATE_CHECKED);
        }
    }

    if (ext_antenna_enable_switch) {
        if (hardware_get_ext_antenna_enable()) {
            lv_obj_add_state(ext_antenna_enable_switch, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(ext_antenna_enable_switch, LV_STATE_CHECKED);
        }
    }
}

static void update_detection_indicators(void) {
    if (!switches_screen) return;

    hardware_detection_t current_detection;
    hardware_get_detection(&current_detection);

    // Update USB-C indicator
    if (usb_c_indicator) {
        lv_obj_set_style_text_color(usb_c_indicator,
            current_detection.usb_c_detected ? lv_color_hex(0xf39c12) : lv_color_hex(0x95a5a6), 0);
    }

    // Update USB-A indicator
    if (usb_a_indicator) {
        lv_obj_set_style_text_color(usb_a_indicator,
            current_detection.usb_a_detected ? lv_color_hex(0xf39c12) : lv_color_hex(0x95a5a6), 0);
    }

    // Update headphone indicator
    if (headphone_indicator) {
        lv_obj_set_style_text_color(headphone_indicator,
            current_detection.headphone_detected ? lv_color_hex(0xf39c12) : lv_color_hex(0x95a5a6), 0);
    }

    // Check for state changes and show messages
    if (current_detection.usb_a_detected != last_detection.usb_a_detected) {
        show_status_message(current_detection.usb_a_detected ? "USB-A connected" : "USB-A disconnected",
                          current_detection.usb_a_detected ? lv_color_hex(0xf39c12) : lv_color_hex(0x95a5a6));
    }

    if (current_detection.headphone_detected != last_detection.headphone_detected) {
        show_status_message(current_detection.headphone_detected ? "Headphone connected" : "Headphone disconnected",
                          current_detection.headphone_detected ? lv_color_hex(0xf39c12) : lv_color_hex(0x95a5a6));
    }

    last_detection = current_detection;
}

static void update_timer_callback(lv_timer_t *timer) {
    (void)timer; // Unused parameter
    update_detection_indicators();
}

static void show_status_message(const char *message, lv_color_t color) {
    if (switches_status_label && message) {
        lv_label_set_text(switches_status_label, message);
        lv_obj_set_style_text_color(switches_status_label, color, 0);
        ESP_LOGI(TAG, "Status: %s", message);
    }
}