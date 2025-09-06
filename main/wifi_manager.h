#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"

#ifdef CONFIG_IDF_TARGET_ESP32P4
#include "esp_wifi_remote.h"
#else
#include "esp_wifi.h"
#endif

#include <stdint.h>
#include <stdbool.h>

#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64
#define MAX_SCAN_RESULTS 20

// WiFi status enumeration
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_DISCONNECTING,
    WIFI_STATUS_ERROR,
    WIFI_STATUS_CONNECTION_FAILED,
    WIFI_STATUS_AP_NOT_FOUND,
    WIFI_STATUS_AUTH_FAILED,
    WIFI_STATUS_RECONNECTING
} wifi_status_t;

// WiFi credentials structure
typedef struct {
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASSWORD_LEN];
    wifi_auth_mode_t auth_mode;
} wifi_credentials_t;

// WiFi scan result structure
typedef struct {
    char ssid[MAX_SSID_LEN];
    int8_t rssi;
    wifi_auth_mode_t auth_mode;
    bool is_open;
} wifi_scan_result_t;

// WiFi status callback
typedef void (*wifi_status_callback_t)(wifi_status_t status, const char* message);

// WiFi scan complete callback
typedef void (*wifi_scan_callback_t)(wifi_scan_result_t* results, int count);

/**
 * @brief Initialize WiFi manager
 * @param status_callback Callback for status updates (can be NULL)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_init(wifi_status_callback_t status_callback);

/**
 * @brief Deinitialize WiFi manager
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief Start WiFi scan
 * @param scan_callback Callback when scan completes (can be NULL)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_scan_start(wifi_scan_callback_t scan_callback);

/**
 * @brief Connect to new WiFi network
 * @param ssid Network SSID
 * @param password Network password (can be NULL for open networks)
 * @param save_credentials Whether to save credentials to NVS
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_connect_new(const char* ssid, const char* password, bool save_credentials);

/**
 * @brief Connect using saved credentials
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_connect_saved(void);

/**
 * @brief Disconnect from current network
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get current WiFi status
 * @return Current WiFi status
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * @brief Get current IP address
 * @return IP address as uint32_t (0 if not connected)
 */
uint32_t wifi_manager_get_ip_address(void);

/**
 * @brief Get current RSSI
 * @return RSSI value in dBm (-127 if not connected)
 */
int8_t wifi_manager_get_rssi(void);

/**
 * @brief Check if auto-reconnect is enabled
 * @return true if enabled, false otherwise
 */
bool wifi_manager_get_auto_reconnect(void);

/**
 * @brief Set auto-reconnect behavior
 * @param enable Enable/disable auto-reconnect
 */
void wifi_manager_set_auto_reconnect(bool enable);

/**
 * @brief Get saved credentials from NVS
 * @param credentials Pointer to credentials structure
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no credentials saved
 */
esp_err_t wifi_manager_get_saved_credentials(wifi_credentials_t* credentials);

/**
 * @brief Clear saved credentials from NVS
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_manager_clear_saved_credentials(void);

/**
 * @brief Convert authentication mode to string
 * @param auth_mode Authentication mode
 * @return String representation of auth mode
 */
const char* wifi_manager_auth_mode_to_string(wifi_auth_mode_t auth_mode);

/**
 * @brief Convert WiFi status to string
 * @param status WiFi status
 * @return String representation of status
 */
const char* wifi_manager_status_to_string(wifi_status_t status);

/**
 * @brief Get WiFi manager event group handle
 * @return Event group handle for WiFi events
 */
void* wifi_manager_get_event_group(void);

#endif // WIFI_MANAGER_H