#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_err.h"
#include "esp_event.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// WiFi Manager Configuration
#define WIFI_MANAGER_NVS_NAMESPACE "wifi_mgr"
#define WIFI_MANAGER_NVS_KEY "wifi_cred"
#define MAX_SSID_LEN 32
#define MAX_PASSWORD_LEN 64
#define MAX_SCAN_RESULTS 20

// WiFi status enum
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_CONNECTION_FAILED,
    WIFI_STATUS_AP_NOT_FOUND,
    WIFI_STATUS_AUTH_FAILED,
    WIFI_STATUS_RECONNECTING
} wifi_status_t;

// WiFi credentials structure
typedef struct {
    char ssid[MAX_SSID_LEN];
    char password[MAX_PASSWORD_LEN];
    bool auto_connect;
} wifi_credentials_t;

// WiFi scan result structure
typedef struct {
    char ssid[MAX_SSID_LEN];
    int8_t rssi;
    wifi_auth_mode_t auth_mode;
    uint8_t channel;
} wifi_scan_result_t;

// Callback function types
typedef void (*wifi_status_callback_t)(wifi_status_t status, uint32_t ip_address);
typedef void (*wifi_scan_callback_t)(wifi_scan_result_t *results, uint16_t count);

/**
 * @brief Initialize WiFi Manager
 * @param status_callback Callback for status updates (can be NULL)
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_init(wifi_status_callback_t status_callback);

/**
 * @brief Deinitialize WiFi Manager
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief Connect using stored credentials
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no credentials stored
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Connect to a new WiFi network
 * @param ssid Network SSID
 * @param password Network password (can be NULL for open networks)
 * @param save_credentials Whether to save credentials for auto-connect
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_connect_new(const char *ssid, const char *password, bool save_credentials);

/**
 * @brief Disconnect from current WiFi network
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Start WiFi scan
 * @param scan_callback Callback to receive scan results
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
 * @return IP address as uint32_t (0 if not connected)
 */
uint32_t wifi_manager_get_ip_address(void);

/**
 * @brief Get current RSSI
 * @return RSSI in dBm
 */
int8_t wifi_manager_get_rssi(void);

/**
 * @brief Check if credentials are stored
 * @return true if credentials are stored
 */
bool wifi_manager_has_credentials(void);

/**
 * @brief Get stored credentials
 * @param credentials Pointer to credentials structure to fill
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no credentials stored
 */
esp_err_t wifi_manager_get_credentials(wifi_credentials_t *credentials);

/**
 * @brief Clear stored credentials
 * @return ESP_OK on success
 */
esp_err_t wifi_manager_clear_credentials(void);

/**
 * @brief Enable/disable auto-reconnect
 * @param enable True to enable auto-reconnect
 */
void wifi_manager_set_auto_reconnect(bool enable);

/**
 * @brief Get WiFi status as string
 * @param status Status enum value
 * @return Human-readable status string
 */
const char* wifi_manager_status_to_string(wifi_status_t status);

/**
 * @brief Get WiFi authentication mode as string
 * @param auth_mode Authentication mode
 * @return Human-readable auth mode string
 */
const char* wifi_manager_auth_mode_to_string(wifi_auth_mode_t auth_mode);

#ifdef __cplusplus
}
#endif

#endif // WIFI_MANAGER_H