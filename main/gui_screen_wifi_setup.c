#include "gui_screen_wifi_setup.h"
#include "gui_screens.h"
#include "gui_styles.h"
#include "gui_events.h"
#include "gui_status_bar.h"
#include "wifi_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "GUI_WIFI_SETUP";

lv_obj_t *wifi_setup_screen = NULL;
lv_obj_t *wifi_network_list = NULL;
lv_obj_t *wifi_password_textarea = NULL;
lv_obj_t *wifi_connect_btn = NULL;
lv_obj_t *wifi_status_label = NULL;
lv_obj_t *wifi_scan_btn = NULL;

static gui_status_bar_t *status_bar = NULL;
static char selected_ssid[MAX_SSID_LEN] = {0};
static wifi_auth_mode_t selected_auth_mode = WIFI_AUTH_OPEN;

// Event handlers
static void wifi_scan_btn_event_handler(lv_event_t *e);
static void wifi_network_list_event_handler(lv_event_t *e);
static void wifi_connect_btn_event_handler(lv_event_t *e);
static void wifi_back_btn_event_handler(lv_event_t *e);

// WiFi manager callbacks
static void wifi_status_callback(wifi_status_t status, uint32_t ip_addr);
static void wifi_scan_callback(wifi_scan_result_t *results, uint8_t count);

void create_wifi_setup_screen(void) {
    ESP_LOGI(TAG, "Creating WiFi setup screen");
    
    wifi_setup_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_setup_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create persistent status bar
    status_bar = gui_status_bar_create(wifi_setup_screen);
    
    // Create main container (adjust position to account for status bar)
    lv_obj_t *main_container = lv_obj_create(wifi_setup_screen);
    lv_obj_set_size(main_container, lv_pct(95), lv_pct(90));
    lv_obj_align(main_container, LV_ALIGN_CENTER, 0, 20); // Offset for status bar
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(main_container, 10, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(main_container);
    lv_label_set_text(title, "WiFi Setup");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);
    
    // WiFi status label
    wifi_status_label = lv_label_create(main_container);
    lv_label_set_text(wifi_status_label, "Ready to scan for networks");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(wifi_status_label, THEME_FONT_SMALL, 0);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 40);
    
    // Scan button
    wifi_scan_btn = lv_button_create(main_container);
    lv_obj_set_size(wifi_scan_btn, lv_pct(40), 50);
    lv_obj_align(wifi_scan_btn, LV_ALIGN_TOP_MID, 0, 70);
    apply_button_style(wifi_scan_btn);
    lv_obj_add_event_cb(wifi_scan_btn, wifi_scan_btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *scan_label = lv_label_create(wifi_scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(scan_label);
    
    // Network list
    wifi_network_list = lv_list_create(main_container);
    lv_obj_set_size(wifi_network_list, lv_pct(90), 200);
    lv_obj_align(wifi_network_list, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_add_event_cb(wifi_network_list, wifi_network_list_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Password input container (initially hidden)
    lv_obj_t *password_container = lv_obj_create(main_container);
    lv_obj_set_size(password_container, lv_pct(90), 60);
    lv_obj_align(password_container, LV_ALIGN_TOP_MID, 0, 340);
    lv_obj_set_style_bg_opa(password_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(password_container, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(password_container, LV_OBJ_FLAG_HIDDEN); // Initially hidden
    
    lv_obj_t *password_label = lv_label_create(password_container);
    lv_label_set_text(password_label, "Password:");
    lv_obj_set_style_text_color(password_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(password_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    wifi_password_textarea = lv_textarea_create(password_container);
    lv_obj_set_size(wifi_password_textarea, lv_pct(100), 35);
    lv_obj_align(wifi_password_textarea, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_textarea_set_placeholder_text(wifi_password_textarea, "Enter network password");
    lv_textarea_set_password_mode(wifi_password_textarea, true);
    
    // Connect button
    wifi_connect_btn = lv_button_create(main_container);
    lv_obj_set_size(wifi_connect_btn, lv_pct(40), 50);
    lv_obj_align(wifi_connect_btn, LV_ALIGN_BOTTOM_MID, -60, -60);
    apply_button_style(wifi_connect_btn);
    lv_obj_add_event_cb(wifi_connect_btn, wifi_connect_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(wifi_connect_btn, LV_STATE_DISABLED); // Initially disabled
    
    lv_obj_t *connect_label = lv_label_create(wifi_connect_btn);
    lv_label_set_text(connect_label, LV_SYMBOL_WIFI " Connect");
    lv_obj_center(connect_label);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(main_container);
    lv_obj_set_size(back_btn, lv_pct(40), 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 60, -60);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, wifi_back_btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Initialize WiFi manager with status callback
    wifi_manager_init(wifi_status_callback);
    
    ESP_LOGI(TAG, "WiFi setup screen created successfully");
}

static void wifi_scan_btn_event_handler(lv_event_t *e) {
    ESP_LOGI(TAG, "WiFi scan button clicked");
    
    // Clear existing list
    lv_obj_clean(wifi_network_list);
    
    // Update status
    lv_label_set_text(wifi_status_label, "Scanning for networks...");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFF00), 0);
    
    // Disable scan button temporarily
    lv_obj_add_state(wifi_scan_btn, LV_STATE_DISABLED);
    
    // Start WiFi scan
    esp_err_t ret = wifi_manager_scan_start(wifi_scan_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(ret));
        lv_label_set_text(wifi_status_label, "Scan failed - please try again");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);
        lv_obj_clear_state(wifi_scan_btn, LV_STATE_DISABLED);
    }
}

static void wifi_network_list_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    if (!btn) return;
    
    // Get selected network info from button text
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if (!label) return;
    
    const char *text = lv_label_get_text(label);
    ESP_LOGI(TAG, "Network selected: %s", text);
    
    // Parse SSID from button text (format: "SSID (Security) RSSI")
    char *ssid_end = strchr(text, '(');
    if (ssid_end) {
        size_t ssid_len = ssid_end - text - 1; // -1 for space before '('
        if (ssid_len < MAX_SSID_LEN) {
            strncpy(selected_ssid, text, ssid_len);
            selected_ssid[ssid_len] = '\0';
            
            // Parse security type from button text
            if (strstr(text, "(Open)")) {
                selected_auth_mode = WIFI_AUTH_OPEN;
                wifi_setup_show_password_input(false);
            } else {
                selected_auth_mode = WIFI_AUTH_WPA2_PSK; // Default to WPA2
                wifi_setup_show_password_input(true);
            }
            
            // Enable connect button
            lv_obj_clear_state(wifi_connect_btn, LV_STATE_DISABLED);
            
            // Update status
            lv_label_set_text(wifi_status_label, selected_ssid);
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00FF00), 0);
        }
    }
}

static void wifi_connect_btn_event_handler(lv_event_t *e) {
    ESP_LOGI(TAG, "WiFi connect button clicked for SSID: %s", selected_ssid);
    
    const char *password = "";
    if (selected_auth_mode != WIFI_AUTH_OPEN && wifi_password_textarea) {
        password = lv_textarea_get_text(wifi_password_textarea);
    }
    
    // Update status
    lv_label_set_text(wifi_status_label, "Connecting...");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFF00), 0);
    
    // Disable connect button
    lv_obj_add_state(wifi_connect_btn, LV_STATE_DISABLED);
    
    // Attempt connection
    esp_err_t ret = wifi_manager_connect_new(selected_ssid, password, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initiate WiFi connection: %s", esp_err_to_name(ret));
        lv_label_set_text(wifi_status_label, "Connection failed - please try again");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);
        lv_obj_clear_state(wifi_connect_btn, LV_STATE_DISABLED);
    }
}

static void wifi_back_btn_event_handler(lv_event_t *e) {
    ESP_LOGI(TAG, "Back button clicked");
    
    // Return to main screen
    lv_screen_load(main_screen);
}

static void wifi_status_callback(wifi_status_t status, uint32_t ip_addr) {
    ESP_LOGI(TAG, "WiFi status callback: %s", wifi_manager_status_to_string(status));
    
    // Update status display
    const char *status_text = wifi_manager_status_to_string(status);
    
    switch (status) {
        case WIFI_STATUS_CONNECTED:
            lv_label_set_text(wifi_status_label, "Connected successfully!");
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00FF00), 0);
            
            // Update status bar WiFi indicator
            if (status_bar) {
                gui_status_bar_update_wifi(status_bar, true, wifi_manager_get_rssi());
            }
            
            // Auto-return to main screen after successful connection
            vTaskDelay(pdMS_TO_TICKS(2000)); // Show success message for 2 seconds
            lv_screen_load(main_screen);
            break;
            
        case WIFI_STATUS_CONNECTION_FAILED:
        case WIFI_STATUS_AP_NOT_FOUND:
        case WIFI_STATUS_AUTH_FAILED:
            lv_label_set_text(wifi_status_label, status_text);
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);
            lv_obj_clear_state(wifi_connect_btn, LV_STATE_DISABLED);
            
            // Update status bar WiFi indicator
            if (status_bar) {
                gui_status_bar_update_wifi(status_bar, false, -127);
            }
            break;
            
        case WIFI_STATUS_CONNECTING:
        case WIFI_STATUS_RECONNECTING:
            lv_label_set_text(wifi_status_label, status_text);
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFF00), 0);
            break;
            
        default:
            if (status_bar) {
                gui_status_bar_update_wifi(status_bar, false, -127);
            }
            break;
    }
}

static void wifi_scan_callback(wifi_scan_result_t *results, uint8_t count) {
    ESP_LOGI(TAG, "WiFi scan completed with %d networks found", count);
    
    // Clear existing list
    lv_obj_clean(wifi_network_list);
    
    if (count == 0) {
        lv_label_set_text(wifi_status_label, "No networks found");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFF00), 0);
    } else {
        lv_label_set_text(wifi_status_label, "Select a network to connect");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFFFF), 0);
        
        // Add networks to list
        for (int i = 0; i < count; i++) {
            char network_text[128];
            const char *auth_str = wifi_manager_auth_mode_to_string(results[i].auth_mode);
            
            snprintf(network_text, sizeof(network_text), "%s (%s) %ddBm", 
                     results[i].ssid, auth_str, results[i].rssi);
            
            lv_obj_t *btn = lv_list_add_btn(wifi_network_list, LV_SYMBOL_WIFI, network_text);
            
            // Color code by signal strength
            if (results[i].rssi > -50) {
                lv_obj_set_style_text_color(btn, lv_color_hex(0x00FF00), 0); // Strong - green
            } else if (results[i].rssi > -70) {
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFF00), 0); // Medium - yellow
            } else {
                lv_obj_set_style_text_color(btn, lv_color_hex(0xFF8800), 0); // Weak - orange
            }
        }
    }
    
    // Re-enable scan button
    lv_obj_clear_state(wifi_scan_btn, LV_STATE_DISABLED);
}

void update_wifi_network_list(void) {
    if (wifi_network_list) {
        wifi_scan_btn_event_handler(NULL); // Trigger a new scan
    }
}

void update_wifi_status_display(const char *status, const char *message) {
    if (wifi_status_label && message) {
        lv_label_set_text(wifi_status_label, message);
        
        // Set color based on status
        if (strcmp(status, "connected") == 0) {
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x00FF00), 0);
        } else if (strcmp(status, "error") == 0) {
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);
        } else {
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFFFF), 0);
        }
    }
}

void wifi_setup_show_password_input(bool show_password) {
    if (!wifi_password_textarea) return;
    
    lv_obj_t *password_container = lv_obj_get_parent(wifi_password_textarea);
    if (!password_container) return;
    
    if (show_password) {
        lv_obj_clear_flag(password_container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(password_container, LV_OBJ_FLAG_HIDDEN);
    }
}

void wifi_setup_clear_password(void) {
    if (wifi_password_textarea) {
        lv_textarea_set_text(wifi_password_textarea, "");
    }
}