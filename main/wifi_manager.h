#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"

// Missing ESP error constant for ESP32-P4
#ifndef ESP_ERR_INVALID_DATA
#define ESP_ERR_INVALID_DATA 0x109 // ESP_ERR_INVALID_DATA from esp_err.h
#endif
#ifdef CONFIG_IDF_TARGET_ESP32P4
// ESP32-P4 uses ESP-Hosted WiFi, not direct WiFi APIs
// Define minimal WiFi types for compatibility
typedef enum {
    WIFI_AUTH_OPEN = 0,         /**< authenticate mode : open */
    WIFI_AUTH_WEP,              /**< authenticate mode : WEP */
    WIFI_AUTH_WPA_PSK,          /**< authenticate mode : WPA_PSK */
    WIFI_AUTH_WPA2_PSK,         /**< authenticate mode : WPA2_PSK */
    WIFI_AUTH_WPA_WPA2_PSK,     /**< authenticate mode : WPA_WPA2_PSK */
    WIFI_AUTH_ENTERPRISE,       /**< authenticate mode : WiFi EAP security */
    WIFI_AUTH_WPA3_PSK,         /**< authenticate mode : WPA3_PSK */
    WIFI_AUTH_WPA2_WPA3_PSK,    /**< authenticate mode : WPA2_WPA3_PSK */
    WIFI_AUTH_MAX
} wifi_auth_mode_t;

// Additional WiFi types for ESP32-P4 compatibility
typedef enum {
    WIFI_IF_STA = 0,     /**< ESP32 station interface */
    WIFI_IF_AP,          /**< ESP32 soft-AP interface */
    WIFI_IF_MAX
} wifi_interface_t;

// WiFi mode definitions
typedef enum {
    WIFI_MODE_NULL = 0,
    WIFI_MODE_STA,
    WIFI_MODE_AP,
    WIFI_MODE_APSTA,
    WIFI_MODE_MAX
} wifi_mode_t;

// WiFi scan type
typedef enum {
    WIFI_SCAN_TYPE_ACTIVE = 0,
    WIFI_SCAN_TYPE_PASSIVE
} wifi_scan_type_t;

// WiFi events
typedef enum {
    WIFI_EVENT_WIFI_READY = 0,
    WIFI_EVENT_SCAN_DONE,
    WIFI_EVENT_STA_START,
    WIFI_EVENT_STA_STOP,
    WIFI_EVENT_STA_CONNECTED,
    WIFI_EVENT_STA_DISCONNECTED,
    WIFI_EVENT_MAX,
} wifi_event_t;

// WiFi disconnect reason
typedef enum {
    WIFI_REASON_NO_AP_FOUND = 201,
    WIFI_REASON_AUTH_FAIL = 202,
    WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT = 15,
    WIFI_REASON_HANDSHAKE_TIMEOUT = 204,
} wifi_err_reason_t;

// PMF configuration
typedef struct {
    bool capable;
    bool required;
} wifi_pmf_config_t;

// Threshold configuration
typedef struct {
    wifi_auth_mode_t authmode;
} wifi_scan_threshold_t;

// WiFi configuration structures
typedef struct {
    struct {
        uint8_t ssid[32];
        uint8_t password[64];
        wifi_scan_threshold_t threshold;
        wifi_pmf_config_t pmf_cfg;
    } sta;
} wifi_config_t;

// WiFi init configuration
typedef struct {
    int nvs_enable;
    int nano_enable;
} wifi_init_config_t;

// WiFi scan configuration
typedef struct {
    uint8_t *ssid;
    uint8_t *bssid;
    uint8_t channel;
    bool show_hidden;
    wifi_scan_type_t scan_type;
    struct {
        struct {
            uint32_t min;
            uint32_t max;
        } active;
        struct {
            uint32_t max;
        } passive;
    } scan_time;
} wifi_scan_config_t;

// WiFi AP record
typedef struct {
    uint8_t bssid[6];
    uint8_t ssid[33];
    uint8_t primary;
    uint8_t second;
    int8_t rssi;
    wifi_auth_mode_t authmode;
} wifi_ap_record_t;

// WiFi events structures
typedef struct {
    uint32_t number;
} wifi_event_sta_scan_done_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t ssid_len;
    uint8_t bssid[6];
    uint8_t channel;
} wifi_event_sta_connected_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t ssid_len;
    uint8_t bssid[6];
    uint8_t channel;
    wifi_err_reason_t reason;
    int8_t rssi;
} wifi_event_sta_disconnected_t;

// Event base definition - define as pointer for compatibility
extern const char* WIFI_EVENT;

// Forward declaration of esp_netif_t for ESP32-P4
struct esp_netif_obj;
typedef struct esp_netif_obj esp_netif_t;

// Macro for WiFi init config default
#define WIFI_INIT_CONFIG_DEFAULT() { \
    .nvs_enable = 1, \
    .nano_enable = 0 \
}

// Stub function declarations (will return ESP_ERR_NOT_SUPPORTED)
esp_err_t esp_wifi_init(const wifi_init_config_t *config);
esp_err_t esp_wifi_deinit(void);
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_set_config(wifi_interface_t interface, wifi_config_t *conf);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_disconnect(void);
esp_err_t esp_wifi_scan_start(const wifi_scan_config_t *config, bool block);
esp_err_t esp_wifi_scan_get_ap_records(uint16_t *number, wifi_ap_record_t *ap_records);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *ap_info);

// Stub for missing netif function
esp_netif_t* esp_netif_create_default_wifi_sta(void);

#else
#include "esp_wifi.h"
#endif
#include <stdint.h>
#include <stdbool.h>

#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64
#define MAX_SCAN_RESULTS 20
#define WIFI_MANAGER_NVS_NAMESPACE "wifi_config"
#define WIFI_MANAGER_NVS_KEY "credentials"

/**
 * @brief WiFi connection status
 */
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_CONNECTION_FAILED,
    WIFI_STATUS_AP_NOT_FOUND,
    WIFI_STATUS_AUTH_FAILED,
    WIFI_STATUS_RECONNECTING
} wifi_status_t;

/**
 * @brief WiFi credential structure
 */
typedef struct {
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASSWORD_LEN];
    bool auto_connect;
    int8_t rssi;
    wifi_auth_mode_t auth_mode;
} wifi_credentials_t;

/**
 * @brief WiFi scan result structure
 */
typedef struct {
    char ssid[MAX_SSID_LEN];
    int8_t rssi;
    wifi_auth_mode_t auth_mode;
    uint8_t channel;
} wifi_scan_result_t;

/**
 * @brief WiFi status callback function type
 * @param status Current WiFi connection status
 * @param ip_addr IP address when connected (0 when disconnected)
 */
typedef void (*wifi_status_callback_t)(wifi_status_t status, uint32_t ip_addr);

/**
 * @brief WiFi scan completion callback function type
 * @param results Array of scan results
 * @param count Number of results found
 */
typedef void (*wifi_scan_callback_t)(wifi_scan_result_t *results, uint8_t count);

/**
 * @brief Initialize WiFi manager
 * @param status_callback Callback for WiFi status changes
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(wifi_status_callback_t status_callback);

/**
 * @brief Deinitialize WiFi manager
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief Connect to WiFi network using stored credentials
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no credentials stored
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Connect to WiFi network with new credentials
 * @param ssid Network SSID
 * @param password Network password
 * @param save_credentials Whether to save credentials for auto-connect
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect_new(const char *ssid, const char *password, bool save_credentials);

/**
 * @brief Disconnect from WiFi network
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Start WiFi network scan
 * @param scan_callback Callback for scan results
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_scan_start(wifi_scan_callback_t scan_callback);

/**
 * @brief Get current WiFi status
 * @return Current WiFi status
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * @brief Get current IP address
 * @return IP address (0 if not connected)
 */
uint32_t wifi_manager_get_ip_address(void);

/**
 * @brief Get current RSSI
 * @return RSSI value (-127 if not connected)
 */
int8_t wifi_manager_get_rssi(void);

/**
 * @brief Check if WiFi credentials are stored
 * @return true if credentials exist, false otherwise
 */
bool wifi_manager_has_credentials(void);

/**
 * @brief Get stored WiFi credentials
 * @param credentials Pointer to structure to fill with credentials
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no credentials stored
 */
esp_err_t wifi_manager_get_credentials(wifi_credentials_t *credentials);

/**
 * @brief Clear stored WiFi credentials
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_clear_credentials(void);

/**
 * @brief Enable/disable auto-reconnection
 * @param enable true to enable auto-reconnection, false to disable
 */
void wifi_manager_set_auto_reconnect(bool enable);

/**
 * @brief Get WiFi status as string for display
 * @param status WiFi status enum
 * @return Human-readable status string
 */
const char* wifi_manager_status_to_string(wifi_status_t status);

/**
 * @brief Get WiFi authentication mode as string
 * @param auth_mode Authentication mode
 * @return Human-readable auth mode string
 */
const char* wifi_manager_auth_mode_to_string(wifi_auth_mode_t auth_mode);

#endif // WIFI_MANAGER_H