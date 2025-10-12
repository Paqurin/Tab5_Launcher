#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "esp_event.h"

// Missing ESP error constant for ESP32-P4
#ifndef ESP_ERR_INVALID_DATA
#define ESP_ERR_INVALID_DATA 0x109 // ESP_ERR_INVALID_DATA from esp_err.h
#endif

// Include standard ESP-IDF WiFi headers
// On ESP32-P4, these are provided by esp_wifi_remote which wraps ESP-Hosted
#include "esp_wifi.h"
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
 * @brief Ensure WiFi hardware is initialized (lazy initialization)
 * This function can be called multiple times safely - it will only initialize once.
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_ensure_hardware_initialized(void);

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