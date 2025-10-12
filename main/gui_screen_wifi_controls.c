#include "gui_screen_wifi_controls.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "GUI_WIFI_CONTROLS";

lv_obj_t *wifi_controls_screen = NULL;

// WiFi UI objects
static lv_obj_t *wifi_scan_btn = NULL;
static lv_obj_t *wifi_disconnect_btn = NULL;
static lv_obj_t *wifi_network_list = NULL;
static lv_obj_t *wifi_status_label = NULL;
static lv_obj_t *wifi_password_textarea = NULL;
static lv_obj_t *wifi_connect_btn = NULL;
static lv_obj_t *wifi_info_panel = NULL;

// Update timer and state tracking
static lv_timer_t *update_timer = NULL;
static char selected_ssid[64] = {0};
static bool password_input_visible = false;
static wifi_scan_result_t scan_results[MAX_SCAN_RESULTS];
static uint8_t scan_result_count = 0;

// Forward declarations
static void update_wifi_status(void);
static void update_timer_callback(lv_timer_t *timer);
static void show_wifi_status_message(const char *message, lv_color_t color);
static void show_password_input(bool show);
static void populate_network_list(void);
static void wifi_scan_complete_callback(wifi_scan_result_t *results, uint8_t count);

// Event handlers
static void wifi_scan_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        show_wifi_status_message("Scanning for networks...", lv_color_hex(0x3498db));
        esp_err_t ret = wifi_manager_scan_start(wifi_scan_complete_callback);
        if (ret != ESP_OK) {
            show_wifi_status_message("Scan failed", lv_color_hex(0xe74c3c));
        }
        ESP_LOGI(TAG, "WiFi scan initiated");
    }
}

static void wifi_disconnect_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        show_wifi_status_message("Disconnecting...", lv_color_hex(0xf39c12));
        esp_err_t ret = wifi_manager_disconnect();
        if (ret == ESP_OK) {
            show_wifi_status_message("Disconnected from WiFi", lv_color_hex(0x95a5a6));
        } else {
            show_wifi_status_message("Disconnect failed", lv_color_hex(0xe74c3c));
        }
        ESP_LOGI(TAG, "WiFi disconnect initiated");
    }
}

static void wifi_connect_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (strlen(selected_ssid) == 0) {
            show_wifi_status_message("Please select a network", lv_color_hex(0xe74c3c));
            return;
        }

        const char *password = lv_textarea_get_text(wifi_password_textarea);
        show_wifi_status_message("Connecting...", lv_color_hex(0x3498db));

        esp_err_t ret = wifi_manager_connect_new(selected_ssid, password, true);
        if (ret != ESP_OK) {
            show_wifi_status_message("Connection failed", lv_color_hex(0xe74c3c));
        }
        ESP_LOGI(TAG, "WiFi connect initiated for SSID: %s", selected_ssid);
    }
}

static void network_list_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *clicked_obj = lv_event_get_target(e);
        const char *ssid = lv_label_get_text(clicked_obj);

        // Extract SSID from the label text (remove WiFi symbol and RSSI)
        if (ssid && strlen(ssid) > 2) {
            const char *ssid_start = ssid + 2; // Skip WiFi symbol
            char *rssi_pos = strstr(ssid_start, " (");
            if (rssi_pos) {
                size_t ssid_len = rssi_pos - ssid_start;
                strncpy(selected_ssid, ssid_start, sizeof(selected_ssid) - 1);
                selected_ssid[ssid_len] = '\0';
            } else {
                strncpy(selected_ssid, ssid_start, sizeof(selected_ssid) - 1);
                selected_ssid[sizeof(selected_ssid) - 1] = '\0';
            }

            show_wifi_status_message(selected_ssid, lv_color_hex(0x3498db));
            show_password_input(true);
            ESP_LOGI(TAG, "Network selected: %s", selected_ssid);
        }
    }
}

static void wifi_controls_back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        wifi_controls_screen_back();
    }
}

void create_wifi_controls_screen(void) {
    if (wifi_controls_screen) {
        return; // Already created
    }

    wifi_controls_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_controls_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(wifi_controls_screen);
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
    lv_obj_add_event_cb(back_btn, wifi_controls_back_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    // Title
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "WiFi Controls");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_24, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // WiFi info panel (connection status)
    wifi_info_panel = lv_obj_create(wifi_controls_screen);
    lv_obj_set_size(wifi_info_panel, lv_pct(90), 60);
    lv_obj_align(wifi_info_panel, LV_ALIGN_TOP_MID, 0, 110);
    lv_obj_set_style_bg_color(wifi_info_panel, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_radius(wifi_info_panel, 10, 0);
    lv_obj_set_style_pad_all(wifi_info_panel, 15, 0);

    wifi_status_label = lv_label_create(wifi_info_panel);
    lv_label_set_text(wifi_status_label, "WiFi Status: Initializing...");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_16, 0);
    lv_obj_align(wifi_status_label, LV_ALIGN_LEFT_MID, 0, 0);

    // Control buttons container
    lv_obj_t *controls_container = lv_obj_create(wifi_controls_screen);
    lv_obj_set_size(controls_container, lv_pct(90), 80);
    lv_obj_align(controls_container, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_style_bg_opa(controls_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(controls_container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(controls_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(controls_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Scan button
    wifi_scan_btn = lv_button_create(controls_container);
    lv_obj_set_size(wifi_scan_btn, 120, 60);
    apply_button_style(wifi_scan_btn);
    lv_obj_set_style_bg_color(wifi_scan_btn, lv_color_hex(0x3498db), 0);
    lv_obj_add_event_cb(wifi_scan_btn, wifi_scan_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *scan_label = lv_label_create(wifi_scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(scan_label);

    // Disconnect button
    wifi_disconnect_btn = lv_button_create(controls_container);
    lv_obj_set_size(wifi_disconnect_btn, 140, 60);
    apply_button_style(wifi_disconnect_btn);
    lv_obj_set_style_bg_color(wifi_disconnect_btn, lv_color_hex(0xe74c3c), 0);
    lv_obj_add_event_cb(wifi_disconnect_btn, wifi_disconnect_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *disconnect_label = lv_label_create(wifi_disconnect_btn);
    lv_label_set_text(disconnect_label, LV_SYMBOL_CLOSE " Disconnect");
    lv_obj_center(disconnect_label);

    // Network list container
    lv_obj_t *list_container = lv_obj_create(wifi_controls_screen);
    lv_obj_set_size(list_container, lv_pct(90), 200);
    lv_obj_align(list_container, LV_ALIGN_TOP_MID, 0, 270);
    lv_obj_set_style_bg_color(list_container, lv_color_hex(0x2c3e50), 0);
    lv_obj_set_style_radius(list_container, 10, 0);
    lv_obj_set_style_pad_all(list_container, 10, 0);

    // Network list title
    lv_obj_t *list_title = lv_label_create(list_container);
    lv_label_set_text(list_title, "Available Networks:");
    lv_obj_set_style_text_color(list_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(list_title, &lv_font_montserrat_14, 0);
    lv_obj_align(list_title, LV_ALIGN_TOP_LEFT, 0, 0);

    // Scrollable network list
    wifi_network_list = lv_obj_create(list_container);
    lv_obj_set_size(wifi_network_list, lv_pct(100), 160);
    lv_obj_align(wifi_network_list, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_opa(wifi_network_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(wifi_network_list, LV_OPA_TRANSP, 0);
    lv_obj_set_scrollbar_mode(wifi_network_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_flex_flow(wifi_network_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wifi_network_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Password input section (initially hidden)
    lv_obj_t *password_container = lv_obj_create(wifi_controls_screen);
    lv_obj_set_size(password_container, lv_pct(90), 120);
    lv_obj_align(password_container, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(password_container, lv_color_hex(0x34495e), 0);
    lv_obj_set_style_radius(password_container, 10, 0);
    lv_obj_set_style_pad_all(password_container, 15, 0);
    lv_obj_add_flag(password_container, LV_OBJ_FLAG_HIDDEN); // Initially hidden

    lv_obj_t *password_title = lv_label_create(password_container);
    lv_label_set_text(password_title, "Password:");
    lv_obj_set_style_text_color(password_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(password_title, LV_ALIGN_TOP_LEFT, 0, 0);

    wifi_password_textarea = lv_textarea_create(password_container);
    lv_obj_set_size(wifi_password_textarea, lv_pct(100), 40);
    lv_obj_align(wifi_password_textarea, LV_ALIGN_TOP_MID, 0, 25);
    lv_textarea_set_placeholder_text(wifi_password_textarea, "Enter network password");
    lv_textarea_set_password_mode(wifi_password_textarea, true);

    wifi_connect_btn = lv_button_create(password_container);
    lv_obj_set_size(wifi_connect_btn, 120, 40);
    lv_obj_align(wifi_connect_btn, LV_ALIGN_TOP_RIGHT, 0, 70);
    apply_button_style(wifi_connect_btn);
    lv_obj_set_style_bg_color(wifi_connect_btn, lv_color_hex(0x27ae60), 0);
    lv_obj_add_event_cb(wifi_connect_btn, wifi_connect_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *connect_label = lv_label_create(wifi_connect_btn);
    lv_label_set_text(connect_label, LV_SYMBOL_WIFI " Connect");
    lv_obj_center(connect_label);

    // Initialize WiFi status
    update_wifi_status();
    populate_network_list();

    // Start update timer
    update_timer = lv_timer_create(update_timer_callback, 1000, NULL);
}

void show_wifi_controls_screen(void) {
    if (!wifi_controls_screen) {
        create_wifi_controls_screen();
    }
    lv_screen_load_anim(wifi_controls_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
}

void wifi_controls_screen_back(void) {
    ESP_LOGI(TAG, "Returning to main screen");
    // Load main screen before destroying to prevent active screen deletion
    lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    // Now safe to clean up wifi controls screen after switching away
    destroy_wifi_controls_screen();
}

void destroy_wifi_controls_screen(void) {
    if (wifi_controls_screen) {
        ESP_LOGI(TAG, "Destroying wifi controls screen...");

        // Stop update timer
        if (update_timer) {
            lv_timer_del(update_timer);
            update_timer = NULL;
        }

        // DEFENSIVE: Check screen validity before destruction
        if (!lv_obj_is_valid(wifi_controls_screen)) {
            ESP_LOGW(TAG, "WiFi controls screen is invalid, setting to NULL");
            wifi_controls_screen = NULL;
            return;
        }

        // DEFENSIVE: Check if screen is currently active
        if (lv_screen_active() == wifi_controls_screen) {
            ESP_LOGW(TAG, "Warning: Destroying active screen - this should not happen");
        }

        // Disable events on screen to prevent corruption during deletion
        lv_obj_remove_event_cb(wifi_controls_screen, NULL);

        // Reset all object pointers
        wifi_scan_btn = NULL;
        wifi_disconnect_btn = NULL;
        wifi_network_list = NULL;
        wifi_status_label = NULL;
        wifi_password_textarea = NULL;
        wifi_connect_btn = NULL;
        wifi_info_panel = NULL;

        // Reset state
        memset(selected_ssid, 0, sizeof(selected_ssid));
        password_input_visible = false;

        // Now safely delete the screen
        lv_obj_del(wifi_controls_screen);
        wifi_controls_screen = NULL;
        ESP_LOGI(TAG, "WiFi controls screen destroyed");
    }
}

// Scan completion callback
static void wifi_scan_complete_callback(wifi_scan_result_t *results, uint8_t count) {
    if (results && count > 0) {
        scan_result_count = (count > MAX_SCAN_RESULTS) ? MAX_SCAN_RESULTS : count;
        memcpy(scan_results, results, scan_result_count * sizeof(wifi_scan_result_t));
        populate_network_list();
        show_wifi_status_message("Scan complete", lv_color_hex(0x27ae60));
        ESP_LOGI(TAG, "WiFi scan completed with %d networks", scan_result_count);
    } else {
        scan_result_count = 0;
        populate_network_list();
        show_wifi_status_message("No networks found", lv_color_hex(0x95a5a6));
        ESP_LOGI(TAG, "WiFi scan completed with no networks");
    }
}

// Helper function implementations
static void update_wifi_status(void) {
    if (!wifi_controls_screen || !wifi_status_label) return;

    wifi_status_t status = wifi_manager_get_status();
    wifi_credentials_t creds;

    switch (status) {
        case WIFI_STATUS_DISCONNECTED:
            lv_label_set_text(wifi_status_label, "WiFi Status: Disconnected");
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x95a5a6), 0);
            break;

        case WIFI_STATUS_CONNECTING:
            lv_label_set_text(wifi_status_label, "WiFi Status: Connecting...");
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xf39c12), 0);
            break;

        case WIFI_STATUS_CONNECTED:
            if (wifi_manager_get_credentials(&creds) == ESP_OK) {
                char status_text[128];
                snprintf(status_text, sizeof(status_text), "WiFi Status: Connected to %s", creds.ssid);
                lv_label_set_text(wifi_status_label, status_text);
            } else {
                lv_label_set_text(wifi_status_label, "WiFi Status: Connected");
            }
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x27ae60), 0);
            break;

        case WIFI_STATUS_CONNECTION_FAILED:
            lv_label_set_text(wifi_status_label, "WiFi Status: Connection Failed");
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xe74c3c), 0);
            break;

        case WIFI_STATUS_AP_NOT_FOUND:
            lv_label_set_text(wifi_status_label, "WiFi Status: Network Not Found");
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xe74c3c), 0);
            break;

        case WIFI_STATUS_AUTH_FAILED:
            lv_label_set_text(wifi_status_label, "WiFi Status: Authentication Failed");
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xe74c3c), 0);
            break;

        case WIFI_STATUS_RECONNECTING:
            lv_label_set_text(wifi_status_label, "WiFi Status: Reconnecting...");
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xf39c12), 0);
            break;
    }
}

static void populate_network_list(void) {
    if (!wifi_network_list) return;

    // Clear existing network items
    lv_obj_clean(wifi_network_list);

    if (scan_result_count == 0) {
        lv_obj_t *no_networks_label = lv_label_create(wifi_network_list);
        lv_label_set_text(no_networks_label, "No networks found. Press Scan to refresh.");
        lv_obj_set_style_text_color(no_networks_label, lv_color_hex(0x95a5a6), 0);
        return;
    }

    // Create network list items
    for (int i = 0; i < scan_result_count && i < 10; i++) { // Limit to 10 networks for UI
        lv_obj_t *network_btn = lv_button_create(wifi_network_list);
        lv_obj_set_size(network_btn, lv_pct(100), 35);
        apply_button_style(network_btn);
        lv_obj_set_style_bg_color(network_btn, lv_color_hex(0x34495e), 0);
        lv_obj_add_event_cb(network_btn, network_list_event_handler, LV_EVENT_CLICKED, NULL);

        lv_obj_t *network_label = lv_label_create(network_btn);
        char network_text[256];  // Large buffer to prevent truncation
        int ret = snprintf(network_text, sizeof(network_text), LV_SYMBOL_WIFI " %.31s (%d dBm)",
                          scan_results[i].ssid, scan_results[i].rssi);
        if (ret >= sizeof(network_text)) {
            ESP_LOGW(TAG, "Network text truncated for SSID: %.31s", scan_results[i].ssid);
        }
        lv_label_set_text(network_label, network_text);
        lv_obj_align(network_label, LV_ALIGN_LEFT_MID, 10, 0);

        // Color code by signal strength
        if (scan_results[i].rssi > -50) {
            lv_obj_set_style_text_color(network_label, lv_color_hex(0x27ae60), 0); // Strong
        } else if (scan_results[i].rssi > -70) {
            lv_obj_set_style_text_color(network_label, lv_color_hex(0xf39c12), 0); // Medium
        } else {
            lv_obj_set_style_text_color(network_label, lv_color_hex(0xe74c3c), 0); // Weak
        }
    }
}

static void show_password_input(bool show) {
    if (!wifi_controls_screen) return;

    lv_obj_t *password_container = lv_obj_get_child(wifi_controls_screen, -1); // Last child

    if (show) {
        lv_obj_remove_flag(password_container, LV_OBJ_FLAG_HIDDEN);
        lv_textarea_set_text(wifi_password_textarea, "");
        password_input_visible = true;
    } else {
        lv_obj_add_flag(password_container, LV_OBJ_FLAG_HIDDEN);
        password_input_visible = false;
    }
}

static void update_timer_callback(lv_timer_t *timer) {
    (void)timer; // Unused parameter
    update_wifi_status();
}

static void show_wifi_status_message(const char *message, lv_color_t color) {
    if (wifi_status_label && message) {
        char status_text[128];
        snprintf(status_text, sizeof(status_text), "WiFi Status: %s", message);
        lv_label_set_text(wifi_status_label, status_text);
        lv_obj_set_style_text_color(wifi_status_label, color, 0);
        ESP_LOGI(TAG, "Status: %s", message);
    }
}