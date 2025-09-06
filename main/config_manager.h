#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// Configuration version for migration/compatibility
#define CONFIG_VERSION 1

// Default configuration values
#define DEFAULT_BRIGHTNESS 80
#define DEFAULT_VOLUME 50
#define DEFAULT_TIMEOUT_MINUTES 5
#define DEFAULT_THEME "dark"
#define DEFAULT_LANGUAGE "en"

// File browser preferences
typedef enum {
    VIEW_MODE_LIST,
    VIEW_MODE_GRID,
    VIEW_MODE_DETAILED
} view_mode_t;

typedef enum {
    SORT_BY_NAME,
    SORT_BY_SIZE,
    SORT_BY_DATE,
    SORT_BY_TYPE
} sort_by_t;

// Theme configuration
typedef struct {
    char name[32];
    uint32_t primary_color;
    uint32_t secondary_color;
    uint32_t background_color;
    uint32_t text_color;
    uint32_t accent_color;
} theme_config_t;

// File browser configuration
typedef struct {
    view_mode_t view_mode;
    sort_by_t sort_by;
    bool sort_ascending;
    bool show_hidden_files;
    bool show_file_extensions;
    bool show_file_sizes;
    bool confirm_delete;
    uint8_t items_per_page;
} file_browser_config_t;

// System configuration
typedef struct {
    uint8_t brightness;          // 0-100
    uint8_t volume;              // 0-100
    uint8_t timeout_minutes;     // Screen timeout in minutes, 0 = never
    bool auto_mount_sd;
    bool enable_animations;
    bool enable_haptic;
    char language[8];            // Language code (e.g., "en", "es", "zh")
} system_config_t;

// Network configuration (for future web interface)
typedef struct {
    bool wifi_enabled;
    char ssid[64];
    char password[64];
    bool ap_mode;
    char ap_ssid[64];
    char ap_password[64];
    uint8_t ap_channel;
    bool web_interface_enabled;
    uint16_t web_port;
} network_config_t;

// Python launcher configuration
typedef struct {
    bool auto_reload;
    bool show_line_numbers;
    bool syntax_highlighting;
    uint8_t tab_size;
    bool use_spaces;
    char default_path[256];
} python_config_t;

// Main configuration structure
typedef struct {
    uint32_t version;
    uint32_t magic;              // Magic number for validation
    system_config_t system;
    file_browser_config_t file_browser;
    network_config_t network;
    python_config_t python;
    theme_config_t theme;
} launcher_config_t;

/**
 * @brief Initialize configuration manager and SPIFFS
 * @return ESP_OK on success
 */
esp_err_t config_manager_init(void);

/**
 * @brief Deinitialize configuration manager and unmount SPIFFS
 * @return ESP_OK on success
 */
esp_err_t config_manager_deinit(void);

/**
 * @brief Load configuration from SPIFFS
 * @param config Pointer to configuration structure to fill
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if no config exists
 */
esp_err_t config_manager_load(launcher_config_t *config);

/**
 * @brief Save configuration to SPIFFS
 * @param config Pointer to configuration structure to save
 * @return ESP_OK on success
 */
esp_err_t config_manager_save(const launcher_config_t *config);

/**
 * @brief Reset configuration to defaults
 * @param config Pointer to configuration structure to reset
 * @return ESP_OK on success
 */
esp_err_t config_manager_reset_defaults(launcher_config_t *config);

/**
 * @brief Backup configuration to SD card
 * @param filename Backup filename on SD card
 * @return ESP_OK on success
 */
esp_err_t config_manager_backup_to_sd(const char *filename);

/**
 * @brief Restore configuration from SD card
 * @param filename Backup filename on SD card
 * @return ESP_OK on success
 */
esp_err_t config_manager_restore_from_sd(const char *filename);

/**
 * @brief Validate configuration structure
 * @param config Pointer to configuration to validate
 * @return true if valid, false otherwise
 */
bool config_manager_validate(const launcher_config_t *config);

/**
 * @brief Export configuration to JSON string
 * @param config Pointer to configuration to export
 * @param json_buffer Buffer to store JSON string
 * @param buffer_size Size of the buffer
 * @return ESP_OK on success
 */
esp_err_t config_manager_export_json(const launcher_config_t *config, char *json_buffer, size_t buffer_size);

/**
 * @brief Import configuration from JSON string
 * @param json_string JSON string to parse
 * @param config Pointer to configuration structure to fill
 * @return ESP_OK on success
 */
esp_err_t config_manager_import_json(const char *json_string, launcher_config_t *config);

/**
 * @brief Get the current configuration (singleton)
 * @return Pointer to current configuration
 */
launcher_config_t* config_manager_get_current(void);

/**
 * @brief Check if SPIFFS is mounted and accessible
 * @return true if SPIFFS is ready, false otherwise
 */
bool config_manager_is_ready(void);

#endif // CONFIG_MANAGER_H