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
    ESP_LOGI(TAG, "Creating enhanced WiFi setup screen");
    
    // Clean up any existing screen
    if (wifi_setup_screen) {
        lv_obj_del(wifi_setup_screen);
        wifi_setup_screen = NULL;
    }
    
    wifi_setup_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_setup_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create persistent status bar - safely check if function exists
    #ifndef CONFIG_IDF_TARGET_ESP32P4
    status_bar = gui_status_bar_create(wifi_setup_screen);
    #else
    status_bar = NULL; // Skip status bar on ESP32-P4 for stability
    #endif
    
    // Create main container (adjust position based on status bar)
    lv_obj_t *main_container = lv_obj_create(wifi_setup_screen);
    lv_obj_set_size(main_container, lv_pct(95), lv_pct(90));
    lv_obj_align(main_container, LV_ALIGN_CENTER, 0, status_bar ? 20 : 0);
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
    lv_label_set_text(wifi_status_label, "ESP32-P4 WiFi: Bridge Required");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(wifi_status_label, THEME_FONT_SMALL, 0);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 40);
    
    // Info panel for ESP32-P4 WiFi architecture
    lv_obj_t *info_panel = lv_obj_create(main_container);
    lv_obj_set_size(info_panel, lv_pct(90), 120);
    lv_obj_align(info_panel, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_set_style_bg_color(info_panel, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(info_panel, lv_color_hex(0x444444), 0);
    lv_obj_set_style_border_width(info_panel, 2, 0);
    lv_obj_set_style_radius(info_panel, 8, 0);
    lv_obj_set_style_pad_all(info_panel, 15, 0);
    
    lv_obj_t *info_title = lv_label_create(info_panel);
    lv_label_set_text(info_title, "ESP32-P4 WiFi Architecture");
    lv_obj_set_style_text_color(info_title, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_text_font(info_title, THEME_FONT_NORMAL, 0);
    lv_obj_align(info_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *info_text = lv_label_create(info_panel);
    lv_label_set_text(info_text, "• ESP32-P4 uses ESP-Hosted WiFi via ESP32-C6\n• Requires external bridge module\n• Interface ready for future integration");
    lv_obj_set_style_text_color(info_text, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(info_text, THEME_FONT_SMALL, 0);
    lv_obj_align(info_text, LV_ALIGN_TOP_LEFT, 0, 25);
    
    // Scan button (functional but shows stub behavior)
    wifi_scan_btn = lv_button_create(main_container);
    lv_obj_set_size(wifi_scan_btn, lv_pct(40), 50);
    lv_obj_align(wifi_scan_btn, LV_ALIGN_TOP_MID, 0, 200);
    apply_button_style(wifi_scan_btn);
    lv_obj_add_event_cb(wifi_scan_btn, wifi_scan_btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *scan_label = lv_label_create(wifi_scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH " Scan (Demo)");
    lv_obj_center(scan_label);
    
    // Network list with enhanced styling
    wifi_network_list = lv_list_create(main_container);
    lv_obj_set_size(wifi_network_list, lv_pct(90), 180);
    lv_obj_align(wifi_network_list, LV_ALIGN_TOP_MID, 0, 260);
    lv_obj_set_style_bg_color(wifi_network_list, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_color(wifi_network_list, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_width(wifi_network_list, 1, 0);
    lv_obj_set_style_radius(wifi_network_list, 8, 0);
    lv_obj_add_event_cb(wifi_network_list, wifi_network_list_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Password input container (initially hidden) with enhanced styling
    lv_obj_t *password_container = lv_obj_create(main_container);
    lv_obj_set_size(password_container, lv_pct(90), 80);
    lv_obj_align(password_container, LV_ALIGN_TOP_MID, 0, 450);
    lv_obj_set_style_bg_color(password_container, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(password_container, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_border_width(password_container, 1, 0);
    lv_obj_set_style_radius(password_container, 8, 0);
    lv_obj_set_style_pad_all(password_container, 10, 0);
    lv_obj_add_flag(password_container, LV_OBJ_FLAG_HIDDEN); // Initially hidden
    
    lv_obj_t *password_label = lv_label_create(password_container);
    lv_label_set_text(password_label, "Network Password:");
    lv_obj_set_style_text_color(password_label, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_text_font(password_label, THEME_FONT_SMALL, 0);
    lv_obj_align(password_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    wifi_password_textarea = lv_textarea_create(password_container);
    lv_obj_set_size(wifi_password_textarea, lv_pct(100), 40);
    lv_obj_align(wifi_password_textarea, LV_ALIGN_TOP_LEFT, 0, 25);
    lv_obj_set_style_bg_color(wifi_password_textarea, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_color(wifi_password_textarea, lv_color_hex(0x666666), 0);
    lv_obj_set_style_text_color(wifi_password_textarea, lv_color_hex(0xFFFFFF), 0);
    lv_textarea_set_placeholder_text(wifi_password_textarea, "Enter network password");
    lv_textarea_set_password_mode(wifi_password_textarea, true);
    
    // Button container for better layout
    lv_obj_t *button_container = lv_obj_create(main_container);
    lv_obj_set_size(button_container, lv_pct(100), 60);
    lv_obj_align(button_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(button_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(button_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(button_container, 0, 0);
    
    // Connect button (demo mode)
    wifi_connect_btn = lv_button_create(button_container);
    lv_obj_set_size(wifi_connect_btn, lv_pct(40), 50);
    lv_obj_align(wifi_connect_btn, LV_ALIGN_LEFT_MID, 0, 0);
    apply_button_style(wifi_connect_btn);
    lv_obj_set_style_bg_color(wifi_connect_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_add_event_cb(wifi_connect_btn, wifi_connect_btn_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(wifi_connect_btn, LV_STATE_DISABLED); // Initially disabled
    
    lv_obj_t *connect_label = lv_label_create(wifi_connect_btn);
    lv_label_set_text(connect_label, LV_SYMBOL_WIFI " Connect (Demo)");
    lv_obj_center(connect_label);
    
    // Settings button (for future WiFi configuration)
    lv_obj_t *settings_btn = lv_button_create(button_container);
    lv_obj_set_size(settings_btn, lv_pct(25), 50);
    lv_obj_align(settings_btn, LV_ALIGN_CENTER, 0, 0);
    apply_button_style(settings_btn);
    lv_obj_set_style_bg_color(settings_btn, lv_color_hex(0xFF9800), 0);
    
    lv_obj_t *settings_label = lv_label_create(settings_btn);
    lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_label);
    
    // Back button
    lv_obj_t *back_btn = lv_button_create(button_container);
    lv_obj_set_size(back_btn, lv_pct(25), 50);
    lv_obj_align(back_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    apply_button_style(back_btn);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x607D8B), 0);
    lv_obj_add_event_cb(back_btn, wifi_back_btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    // Initialize WiFi manager with status callback (safe for ESP32-P4)
    esp_err_t wifi_init_result = wifi_manager_init(wifi_status_callback);
    if (wifi_init_result != ESP_OK) {
        ESP_LOGW(TAG, "WiFi manager init returned: %s (expected for ESP32-P4)", esp_err_to_name(wifi_init_result));
    }
    
    // Populate with demo networks to show interface functionality
    wifi_populate_demo_networks();
    
    ESP_LOGI(TAG, "Enhanced WiFi setup screen created successfully");
}

static void wifi_scan_btn_event_handler(lv_event_t *e) {
    ESP_LOGI(TAG, "WiFi scan button clicked (demo mode)");
    
    // Clear existing list
    lv_obj_clean(wifi_network_list);
    
    // Update status to show scanning
    lv_label_set_text(wifi_status_label, "Scanning (Demo Mode)...");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFF00), 0);
    
    // Disable scan button temporarily
    lv_obj_add_state(wifi_scan_btn, LV_STATE_DISABLED);
    
    // Start WiFi scan (will return ESP_ERR_NOT_SUPPORTED on ESP32-P4)
    esp_err_t ret = wifi_manager_scan_start(wifi_scan_callback);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "WiFi scan not supported (ESP32-P4), showing demo results");
        
        // Simulate scan delay
        vTaskDelay(pdMS_TO_TICKS(1500));
        
        // Show demo networks with refreshed data
        wifi_populate_demo_networks();
        
        // Re-enable scan button
        lv_obj_clear_state(wifi_scan_btn, LV_STATE_DISABLED);
        
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi scan: %s", esp_err_to_name(ret));
        lv_label_set_text(wifi_status_label, "Scan failed - ESP32-P4 bridge required");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);
        lv_obj_clear_state(wifi_scan_btn, LV_STATE_DISABLED);
    }
    // If ret == ESP_OK, the callback will handle re-enabling the button
}

static void wifi_network_list_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    if (!btn) return;
    
    // Get selected network info from button text
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    if (!label) return;
    
    const char *text = lv_label_get_text(label);
    ESP_LOGI(TAG, "Network selected (demo): %s", text);
    
    // Reset all button styles first
    uint32_t child_count = lv_obj_get_child_count(wifi_network_list);
    for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *child_btn = lv_obj_get_child(wifi_network_list, i);
        lv_obj_set_style_bg_color(child_btn, lv_color_hex(0x2C2C2C), 0);
        lv_obj_set_style_border_width(child_btn, 0, 0);
    }
    
    // Highlight selected button
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x81C784), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    
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
            
            // Update status with enhanced messaging
            char status_msg[128];
            snprintf(status_msg, sizeof(status_msg), "Selected: %s (Demo Mode)", selected_ssid);
            lv_label_set_text(wifi_status_label, status_msg);
            lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x4CAF50), 0);
        }
    }
}

static void wifi_connect_btn_event_handler(lv_event_t *e) {
    ESP_LOGI(TAG, "WiFi connect button clicked (demo mode) for SSID: %s", selected_ssid);
    
    const char *password = "";
    if (selected_auth_mode != WIFI_AUTH_OPEN && wifi_password_textarea) {
        password = lv_textarea_get_text(wifi_password_textarea);
    }
    
    // Update status for demo
    lv_label_set_text(wifi_status_label, "Demo: Connection Simulation...");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFFFF00), 0);
    
    // Disable connect button
    lv_obj_add_state(wifi_connect_btn, LV_STATE_DISABLED);
    
    // Attempt connection (will return ESP_ERR_NOT_SUPPORTED on ESP32-P4)
    esp_err_t ret = wifi_manager_connect_new(selected_ssid, password, true);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "WiFi connection not supported (ESP32-P4), showing demo behavior");
        
        // Simulate connection attempt
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Show demo success message
        lv_label_set_text(wifi_status_label, "Demo: Would connect to ESP32-C6 bridge");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x2196F3), 0);
        
        // Re-enable connect button after demo
        lv_obj_clear_state(wifi_connect_btn, LV_STATE_DISABLED);
        
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initiate WiFi connection: %s", esp_err_to_name(ret));
        lv_label_set_text(wifi_status_label, "Connection failed - ESP32-P4 bridge required");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0xFF0000), 0);
        lv_obj_clear_state(wifi_connect_btn, LV_STATE_DISABLED);
    }
    // If ret == ESP_OK, the callback will handle re-enabling the button
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

void wifi_populate_demo_networks(void) {
    ESP_LOGI(TAG, "Populating demo WiFi networks for interface demonstration");
    
    // Clear existing list
    if (wifi_network_list) {
        lv_obj_clean(wifi_network_list);
    }
    
    // Create demo networks to show interface functionality
    struct {
        const char *name;
        const char *security;
        int rssi;
        uint32_t color;
    } demo_networks[] = {
        {"HomeNetwork_5G", "WPA2", -35, 0x00FF00},      // Strong - green
        {"OfficeWiFi", "WPA2-Enterprise", -45, 0x00FF00}, // Strong - green  
        {"GuestNetwork", "Open", -55, 0xFFFF00},         // Medium - yellow
        {"Neighbor_WiFi", "WPA3", -65, 0xFFFF00},        // Medium - yellow
        {"PublicHotspot", "Open", -75, 0xFF8800},        // Weak - orange
        {"ESP32-P4_Demo", "WPA2", -40, 0x2196F3},        // Demo - blue
    };
    
    int demo_count = sizeof(demo_networks) / sizeof(demo_networks[0]);
    
    for (int i = 0; i < demo_count; i++) {
        char network_text[128];
        snprintf(network_text, sizeof(network_text), "%s (%s) %ddBm", 
                demo_networks[i].name, demo_networks[i].security, demo_networks[i].rssi);
        
        lv_obj_t *btn = lv_list_add_btn(wifi_network_list, LV_SYMBOL_WIFI, network_text);
        
        // Style the button based on signal strength
        lv_obj_set_style_text_color(btn, lv_color_hex(demo_networks[i].color), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2C2C2C), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x404040), LV_STATE_PRESSED);
    }
    
    // Update status
    if (wifi_status_label) {
        lv_label_set_text(wifi_status_label, "Demo Networks (ESP32-P4 Bridge Required)");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x2196F3), 0);
    }
    
    ESP_LOGI(TAG, "Demo networks populated successfully");
}