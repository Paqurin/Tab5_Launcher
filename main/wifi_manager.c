#include "wifi_manager.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include <string.h>

// Include WiFi headers for type definitions
#include "esp_wifi.h"
#include "esp_wifi_types.h"

static const char *TAG = "WIFI_MANAGER";

// WiFi manager state
static wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;
static wifi_status_callback_t s_status_callback = NULL;
static wifi_scan_callback_t s_scan_callback = NULL;
static uint32_t s_ip_address = 0;
static int8_t s_rssi = -127;

esp_err_t wifi_manager_init(wifi_status_callback_t status_callback) {
    ESP_LOGI(TAG, "Initializing WiFi Manager");
    
    s_status_callback = status_callback;
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
#ifdef CONFIG_IDF_TARGET_ESP32P4
    // ESP32-P4 WiFi functionality temporarily disabled
    ESP_LOGW(TAG, "WiFi functionality temporarily disabled on ESP32-P4 - component compatibility issue");
    ret = ESP_ERR_NOT_SUPPORTED;
#else
    // Normal WiFi initialization for other platforms
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret == ESP_OK) {
        ret = esp_wifi_set_mode(WIFI_MODE_STA);
        if (ret == ESP_OK) {
            ret = esp_wifi_start();
        }
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi initialization failed: %s", esp_err_to_name(ret));
    }
#endif
    
    ESP_LOGI(TAG, "WiFi Manager initialized successfully");
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing WiFi Manager");
    
#ifndef CONFIG_IDF_TARGET_ESP32P4
    esp_wifi_stop();
    esp_wifi_deinit();
#endif
    
    s_status_callback = NULL;
    s_scan_callback = NULL;
    
    return ESP_OK;
}

esp_err_t wifi_manager_connect(void) {
    ESP_LOGI(TAG, "Attempting to connect using stored credentials");
    
#ifdef CONFIG_IDF_TARGET_ESP32P4
    ESP_LOGW(TAG, "WiFi connect not supported on ESP32-P4 - component compatibility issue");
    return ESP_ERR_NOT_SUPPORTED;
#else
    return ESP_ERR_NOT_FOUND; // No stored credentials functionality yet
#endif
}

esp_err_t wifi_manager_connect_new(const char *ssid, const char *password, bool save_credentials) {
    ESP_LOGI(TAG, "Connecting to WiFi network: %s", ssid);
    
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Invalid SSID");
        return ESP_ERR_INVALID_ARG;
    }
    
#ifdef CONFIG_IDF_TARGET_ESP32P4
    ESP_LOGW(TAG, "WiFi connect not supported on ESP32-P4 - component compatibility issue");
    return ESP_ERR_NOT_SUPPORTED;
#else
    wifi_config_t wifi_config = {0};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    if (password && strlen(password) > 0) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    }
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to set WiFi config: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return esp_wifi_connect();
#endif
}

esp_err_t wifi_manager_disconnect(void) {
    ESP_LOGI(TAG, "Disconnecting from WiFi");
    
#ifdef CONFIG_IDF_TARGET_ESP32P4
    ESP_LOGW(TAG, "WiFi disconnect not supported on ESP32-P4 - component compatibility issue");
    return ESP_ERR_NOT_SUPPORTED;
#else
    return esp_wifi_disconnect();
#endif
}

esp_err_t wifi_manager_scan_start(wifi_scan_callback_t scan_callback) {
    ESP_LOGI(TAG, "Starting WiFi scan");
    
    s_scan_callback = scan_callback;
    
#ifdef CONFIG_IDF_TARGET_ESP32P4
    ESP_LOGW(TAG, "WiFi scan not supported on ESP32-P4 - component compatibility issue");
    return ESP_ERR_NOT_SUPPORTED;
#else
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 300
            }
        }
    };
    
    return esp_wifi_scan_start(&scan_config, false);
#endif
}

wifi_status_t wifi_manager_get_status(void) {
    return s_wifi_status;
}

uint32_t wifi_manager_get_ip_address(void) {
    return s_ip_address;
}

int8_t wifi_manager_get_rssi(void) {
    return s_rssi;
}

bool wifi_manager_has_credentials(void) {
    return false; // Simplified - no credential storage yet
}

esp_err_t wifi_manager_get_credentials(wifi_credentials_t *credentials) {
    return ESP_ERR_NOT_FOUND; // Simplified - no credential storage yet
}

esp_err_t wifi_manager_clear_credentials(void) {
    return ESP_OK; // Simplified - no credential storage yet
}

void wifi_manager_set_auto_reconnect(bool enable) {
    ESP_LOGI(TAG, "Auto-reconnect %s", enable ? "enabled" : "disabled");
}

const char* wifi_manager_status_to_string(wifi_status_t status) {
    switch (status) {
        case WIFI_STATUS_DISCONNECTED: return "Disconnected";
        case WIFI_STATUS_CONNECTING: return "Connecting";
        case WIFI_STATUS_CONNECTED: return "Connected";
        case WIFI_STATUS_CONNECTION_FAILED: return "Connection Failed";
        case WIFI_STATUS_AP_NOT_FOUND: return "Network Not Found";
        case WIFI_STATUS_AUTH_FAILED: return "Authentication Failed";
        case WIFI_STATUS_RECONNECTING: return "Reconnecting";
        default: return "Unknown";
    }
}

const char* wifi_manager_auth_mode_to_string(wifi_auth_mode_t auth_mode) {
    switch (auth_mode) {
        case WIFI_AUTH_OPEN: return "Open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
        case WIFI_AUTH_WPA3_PSK: return "WPA3";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
        default: return "Unknown";
    }
}