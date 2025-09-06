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

#ifdef CONFIG_IDF_TARGET_ESP32P4
#include "esp_wifi_remote.h"
#else
#include "esp_wifi.h"
#endif

static const char *TAG = "WIFI_MANAGER";

#ifdef CONFIG_IDF_TARGET_ESP32P4
// Stub implementations for WiFi functions on ESP32-P4

// Event base definition
const char* WIFI_EVENT = "WIFI_EVENT";

esp_err_t esp_wifi_init(const wifi_init_config_t *config) {
    ESP_LOGI(TAG, "esp_wifi_init: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_init(config);
}

esp_err_t esp_wifi_deinit(void) {
    ESP_LOGI(TAG, "esp_wifi_deinit: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_deinit();
}

esp_err_t esp_wifi_set_mode(wifi_mode_t mode) {
    ESP_LOGI(TAG, "esp_wifi_set_mode: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_set_mode(mode);
}

esp_err_t esp_wifi_start(void) {
    ESP_LOGI(TAG, "esp_wifi_start: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_start();
}

esp_err_t esp_wifi_stop(void) {
    ESP_LOGI(TAG, "esp_wifi_stop: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_stop();
}

esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf) {
    ESP_LOGI(TAG, "esp_wifi_set_config: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_set_config(interface, conf);
}

esp_err_t esp_wifi_connect(void) {
    ESP_LOGI(TAG, "esp_wifi_connect: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_connect();
}

esp_err_t esp_wifi_disconnect(void) {
    ESP_LOGI(TAG, "esp_wifi_disconnect: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_disconnect();
}

esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block) {
    ESP_LOGI(TAG, "esp_wifi_scan_start: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_scan_start(config, block);
}

esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records) {
    ESP_LOGI(TAG, "esp_wifi_scan_get_ap_records: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_scan_get_ap_records(number, ap_records);
}

esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info) {
    ESP_LOGI(TAG, "esp_wifi_sta_get_ap_info: Using esp_wifi_remote for ESP32-P4");
    return esp_wifi_remote_sta_get_ap_info(ap_info);
}

// Note: esp_netif_create_default_wifi_sta should work normally on ESP32-P4
// as it's a network interface function, not a WiFi-specific function

#endif // CONFIG_IDF_TARGET_ESP32P4

// Event group for WiFi events
static EventGroupHandle_t s_wifi_event_group;
static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;

// WiFi manager state
static wifi_status_t s_wifi_status = WIFI_STATUS_DISCONNECTED;
static wifi_status_callback_t s_status_callback = NULL;
static wifi_scan_callback_t s_scan_callback = NULL;
static esp_netif_t *s_sta_netif = NULL;
static int s_retry_num = 0;
static const int WIFI_MAX_RETRY = 5;
static bool s_auto_reconnect = true;
static uint32_t s_ip_address = 0;
static int8_t s_rssi = -127;

// Forward declarations
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static esp_err_t wifi_manager_save_credentials(const wifi_credentials_t *credentials);
static esp_err_t wifi_manager_load_credentials(wifi_credentials_t *credentials);
static void wifi_manager_set_status(wifi_status_t status);

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
    
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (!s_sta_netif) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
        return ESP_FAIL;
    }
    
    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi initialization failed: %s", esp_err_to_name(ret));
        if (ret == ESP_ERR_NOT_SUPPORTED) {
            ESP_LOGI(TAG, "WiFi not supported on this platform (ESP32-P4 requires ESP-Hosted)");
        }
    }
    
    // Create event group
    s_wifi_event_group = xEventGroupCreate();
    if (!s_wifi_event_group) {
        ESP_LOGE(TAG, "Failed to create WiFi event group");
        return ESP_ERR_NO_MEM;
    }
    
    // Register event handlers
#ifdef CONFIG_IDF_TARGET_ESP32P4
    ESP_LOGI(TAG, "Registering WIFI_REMOTE_EVENT handlers for ESP32-P4");
    ret = esp_event_handler_register(WIFI_REMOTE_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
#else
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
#endif
    if (ret == ESP_OK) {
        ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register event handlers: %s", esp_err_to_name(ret));
    }
    
    // Set WiFi mode to station
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret == ESP_OK) {
        ret = esp_wifi_start();
    }
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "WiFi Manager initialized successfully");
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void) {
    ESP_LOGI(TAG, "Deinitializing WiFi Manager");
    
    // Stop WiFi
    esp_wifi_stop();
    esp_wifi_deinit();
    
    // Unregister event handlers
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler);
    
    // Clean up event group
    if (s_wifi_event_group) {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    
    // Clean up network interface
    if (s_sta_netif) {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
    }
    
    s_status_callback = NULL;
    s_scan_callback = NULL;
    
    return ESP_OK;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi station started");
                break;
                
            case WIFI_EVENT_STA_CONNECTED: {
                wifi_event_sta_connected_t* event = (wifi_event_sta_connected_t*) event_data;
                ESP_LOGI(TAG, "Connected to AP SSID:%s", event->ssid);
                wifi_manager_set_status(WIFI_STATUS_CONNECTING);
                s_retry_num = 0;
                break;
            }
            
            case WIFI_EVENT_STA_DISCONNECTED: {
                wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
                ESP_LOGI(TAG, "Disconnected from AP SSID:%s, reason:%d", event->ssid, event->reason);
                
                s_ip_address = 0;
                s_rssi = -127;
                
                if (s_auto_reconnect && s_retry_num < WIFI_MAX_RETRY) {
                    esp_wifi_connect();
                    s_retry_num++;
                    wifi_manager_set_status(WIFI_STATUS_RECONNECTING);
                    ESP_LOGI(TAG, "Retry to connect to the AP, attempt %d/%d", s_retry_num, WIFI_MAX_RETRY);
                } else {
                    if (event->reason == WIFI_REASON_NO_AP_FOUND) {
                        wifi_manager_set_status(WIFI_STATUS_AP_NOT_FOUND);
                    } else if (event->reason == WIFI_REASON_AUTH_FAIL || 
                              event->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
                              event->reason == WIFI_REASON_HANDSHAKE_TIMEOUT) {
                        wifi_manager_set_status(WIFI_STATUS_AUTH_FAILED);
                    } else {
                        wifi_manager_set_status(WIFI_STATUS_CONNECTION_FAILED);
                    }
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                break;
            }
            
            case WIFI_EVENT_SCAN_DONE: {
                wifi_event_sta_scan_done_t* event = (wifi_event_sta_scan_done_t*) event_data;
                ESP_LOGI(TAG, "WiFi scan completed, found %" PRIu32 " networks", event->number);
                
                if (s_scan_callback) {
                    uint16_t scan_count = MIN(event->number, MAX_SCAN_RESULTS);
                    wifi_ap_record_t ap_records[MAX_SCAN_RESULTS];
                    wifi_scan_result_t results[MAX_SCAN_RESULTS];
                    
                    esp_err_t scan_ret = esp_wifi_scan_get_ap_records(&scan_count, ap_records);
                    if (scan_ret == ESP_OK) {
                        for (int i = 0; i < scan_count; i++) {
                            strncpy(results[i].ssid, (char*)ap_records[i].ssid, MAX_SSID_LEN - 1);
                            results[i].ssid[MAX_SSID_LEN - 1] = '\0';
                            results[i].rssi = ap_records[i].rssi;
                            results[i].auth_mode = ap_records[i].authmode;
                            results[i].channel = ap_records[i].primary;
                        }
                        s_scan_callback(results, scan_count);
                    } else {
                        // Return empty scan results on error
                        wifi_scan_result_t empty_results[1];
                        s_scan_callback(empty_results, 0);
                    }
                }
                break;
            }
            
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        
        s_ip_address = event->ip_info.ip.addr;
        
        // Get WiFi info including RSSI
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            s_rssi = ap_info.rssi;
        } else {
            // Set default RSSI if WiFi info unavailable
            s_rssi = -50; // Default reasonable signal strength
        }
        
        wifi_manager_set_status(WIFI_STATUS_CONNECTED);
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_manager_set_status(wifi_status_t status) {
    if (s_wifi_status != status) {
        s_wifi_status = status;
        ESP_LOGI(TAG, "WiFi status changed to: %s", wifi_manager_status_to_string(status));
        
        if (s_status_callback) {
            s_status_callback(status, s_ip_address);
        }
    }
}

esp_err_t wifi_manager_connect(void) {
    ESP_LOGI(TAG, "Attempting to connect using stored credentials");
    
    wifi_credentials_t credentials;
    esp_err_t ret = wifi_manager_load_credentials(&credentials);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No stored credentials found");
        return ESP_ERR_NOT_FOUND;
    }
    
    return wifi_manager_connect_new(credentials.ssid, credentials.password, false);
}

esp_err_t wifi_manager_connect_new(const char *ssid, const char *password, bool save_credentials) {
    ESP_LOGI(TAG, "Connecting to WiFi network: %s", ssid);
    
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "Invalid SSID");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Save credentials if requested
    if (save_credentials) {
        wifi_credentials_t creds;
        strncpy(creds.ssid, ssid, MAX_SSID_LEN - 1);
        creds.ssid[MAX_SSID_LEN - 1] = '\0';
        strncpy(creds.password, password ? password : "", MAX_PASSWORD_LEN - 1);
        creds.password[MAX_PASSWORD_LEN - 1] = '\0';
        creds.auto_connect = true;
        
        esp_err_t save_ret = wifi_manager_save_credentials(&creds);
        if (save_ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save credentials: %s", esp_err_to_name(save_ret));
        }
    }
    
    // Configure WiFi
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
        wifi_manager_set_status(WIFI_STATUS_DISCONNECTED);
        return ret;
    }
    
    // Clear event group bits
    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    
    s_retry_num = 0;
    wifi_manager_set_status(WIFI_STATUS_CONNECTING);
    
    return esp_wifi_connect();
}

esp_err_t wifi_manager_disconnect(void) {
    ESP_LOGI(TAG, "Disconnecting from WiFi");
    
    s_auto_reconnect = false;
    esp_err_t ret = esp_wifi_disconnect();
    
    // Always update status regardless of WiFi call result
    wifi_manager_set_status(WIFI_STATUS_DISCONNECTED);
    s_ip_address = 0;
    s_rssi = -127;
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WiFi disconnect failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t wifi_manager_scan_start(wifi_scan_callback_t scan_callback) {
    ESP_LOGI(TAG, "Starting WiFi scan");
    
    s_scan_callback = scan_callback;
    
#ifdef CONFIG_IDF_TARGET_ESP32P4
    // ESP32-P4 WiFi scanning not implemented - requires ESP-Hosted
    ESP_LOGW(TAG, "WiFi scan not available on ESP32-P4 (needs ESP-Hosted implementation)");
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

static esp_err_t wifi_manager_save_credentials(const wifi_credentials_t *credentials) {
    ESP_LOGI(TAG, "Saving WiFi credentials to NVS");
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Create JSON object for credentials
    cJSON *json = cJSON_CreateObject();
    cJSON *ssid = cJSON_CreateString(credentials->ssid);
    cJSON *password = cJSON_CreateString(credentials->password);
    cJSON *auto_connect = cJSON_CreateBool(credentials->auto_connect);
    
    cJSON_AddItemToObject(json, "ssid", ssid);
    cJSON_AddItemToObject(json, "password", password);
    cJSON_AddItemToObject(json, "auto_connect", auto_connect);
    
    char *json_string = cJSON_Print(json);
    if (!json_string) {
        ESP_LOGE(TAG, "Failed to create JSON string");
        cJSON_Delete(json);
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }
    
    ret = nvs_set_str(nvs_handle, WIFI_MANAGER_NVS_KEY, json_string);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save credentials: %s", esp_err_to_name(ret));
    } else {
        ret = nvs_commit(nvs_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        }
    }
    
    free(json_string);
    cJSON_Delete(json);
    nvs_close(nvs_handle);
    
    return ret;
}

static esp_err_t wifi_manager_load_credentials(wifi_credentials_t *credentials) {
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    size_t required_size = 0;
    ret = nvs_get_str(nvs_handle, WIFI_MANAGER_NVS_KEY, NULL, &required_size);
    if (ret != ESP_OK) {
        nvs_close(nvs_handle);
        return ret;
    }
    
    char *json_string = malloc(required_size);
    if (!json_string) {
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }
    
    ret = nvs_get_str(nvs_handle, WIFI_MANAGER_NVS_KEY, json_string, &required_size);
    nvs_close(nvs_handle);
    
    if (ret != ESP_OK) {
        free(json_string);
        return ret;
    }
    
    // Parse JSON
    cJSON *json = cJSON_Parse(json_string);
    free(json_string);
    
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse stored credentials JSON");
        return ESP_ERR_INVALID_DATA;
    }
    
    cJSON *ssid = cJSON_GetObjectItem(json, "ssid");
    cJSON *password = cJSON_GetObjectItem(json, "password");
    cJSON *auto_connect = cJSON_GetObjectItem(json, "auto_connect");
    
    if (cJSON_IsString(ssid)) {
        strncpy(credentials->ssid, ssid->valuestring, MAX_SSID_LEN - 1);
        credentials->ssid[MAX_SSID_LEN - 1] = '\0';
    }
    
    if (cJSON_IsString(password)) {
        strncpy(credentials->password, password->valuestring, MAX_PASSWORD_LEN - 1);
        credentials->password[MAX_PASSWORD_LEN - 1] = '\0';
    }
    
    credentials->auto_connect = cJSON_IsTrue(auto_connect);
    
    cJSON_Delete(json);
    return ESP_OK;
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
    wifi_credentials_t credentials;
    return wifi_manager_load_credentials(&credentials) == ESP_OK;
}

esp_err_t wifi_manager_get_credentials(wifi_credentials_t *credentials) {
    return wifi_manager_load_credentials(credentials);
}

esp_err_t wifi_manager_clear_credentials(void) {
    ESP_LOGI(TAG, "Clearing stored WiFi credentials");
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(WIFI_MANAGER_NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        return ret;
    }
    
    ret = nvs_erase_key(nvs_handle, WIFI_MANAGER_NVS_KEY);
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs_handle);
    }
    
    nvs_close(nvs_handle);
    return ret;
}

void wifi_manager_set_auto_reconnect(bool enable) {
    s_auto_reconnect = enable;
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