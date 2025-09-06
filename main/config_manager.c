#include "config_manager.h"
#include "sd_manager.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "cJSON.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <errno.h>

static const char *TAG = "CONFIG_MGR";
static const char *CONFIG_FILE = "/spiffs/launcher_config.json";
static const char *CONFIG_BACKUP = "/spiffs/launcher_config.bak";
static const uint32_t CONFIG_MAGIC = 0x4C414E43; // "LANC" for LAuNCher

static launcher_config_t current_config;
static bool spiffs_mounted = false;

// Initialize default configuration
static void set_default_config(launcher_config_t *config) {
    memset(config, 0, sizeof(launcher_config_t));
    
    config->version = CONFIG_VERSION;
    config->magic = CONFIG_MAGIC;
    
    // System defaults
    config->system.brightness = DEFAULT_BRIGHTNESS;
    config->system.volume = DEFAULT_VOLUME;
    config->system.timeout_minutes = DEFAULT_TIMEOUT_MINUTES;
    config->system.auto_mount_sd = true;
    config->system.enable_animations = true;
    config->system.enable_haptic = false;
    strcpy(config->system.language, DEFAULT_LANGUAGE);
    
    // File browser defaults
    config->file_browser.view_mode = VIEW_MODE_LIST;
    config->file_browser.sort_by = SORT_BY_NAME;
    config->file_browser.sort_ascending = true;
    config->file_browser.show_hidden_files = false;
    config->file_browser.show_file_extensions = true;
    config->file_browser.show_file_sizes = true;
    config->file_browser.confirm_delete = true;
    config->file_browser.items_per_page = 10;
    
    // Network defaults (WiFi disabled for ESP32-P4)
    config->network.wifi_enabled = false;
    config->network.ap_mode = false;
    config->network.web_interface_enabled = false;
    config->network.web_port = 80;
    config->network.ap_channel = 1;
    strcpy(config->network.ap_ssid, "Tab5_Launcher");
    strcpy(config->network.ap_password, "12345678");
    
    // Python launcher defaults
    config->python.auto_reload = true;
    config->python.show_line_numbers = true;
    config->python.syntax_highlighting = true;
    config->python.tab_size = 4;
    config->python.use_spaces = true;
    strcpy(config->python.default_path, "/");
    
    // Theme defaults
    strcpy(config->theme.name, DEFAULT_THEME);
    config->theme.primary_color = 0xFF2196F3;     // Material Blue
    config->theme.secondary_color = 0xFF1976D2;   // Darker Blue
    config->theme.background_color = 0xFF121212;  // Dark Background
    config->theme.text_color = 0xFFFFFFFF;        // White Text
    config->theme.accent_color = 0xFF4CAF50;      // Green Accent
}

esp_err_t config_manager_init(void) {
    ESP_LOGI(TAG, "Initializing configuration manager with SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "spiffs",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    // Mount SPIFFS
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }
    
    // Check SPIFFS partition information
    size_t total = 0, used = 0;
    ret = esp_spiffs_info("spiffs", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS partition: total: %d KB, used: %d KB", total / 1024, used / 1024);
    }
    
    spiffs_mounted = true;
    
    // Load configuration or create default
    ret = config_manager_load(&current_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No valid configuration found, creating defaults");
        set_default_config(&current_config);
        config_manager_save(&current_config);
    }
    
    ESP_LOGI(TAG, "Configuration manager initialized successfully");
    return ESP_OK;
}

esp_err_t config_manager_deinit(void) {
    if (!spiffs_mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = esp_vfs_spiffs_unregister("spiffs");
    if (ret == ESP_OK) {
        spiffs_mounted = false;
        ESP_LOGI(TAG, "SPIFFS unmounted successfully");
    }
    return ret;
}

esp_err_t config_manager_load(launcher_config_t *config) {
    if (!spiffs_mounted || !config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    FILE *f = fopen(CONFIG_FILE, "r");
    if (!f) {
        ESP_LOGW(TAG, "Configuration file not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > 16384) { // Max 16KB for config
        fclose(f);
        ESP_LOGE(TAG, "Invalid configuration file size: %ld", file_size);
        return ESP_FAIL;
    }
    
    // Read JSON content
    char *json_buffer = malloc(file_size + 1);
    if (!json_buffer) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    
    size_t bytes_read = fread(json_buffer, 1, file_size, f);
    fclose(f);
    json_buffer[bytes_read] = '\0';
    
    // Parse JSON and fill config structure
    esp_err_t ret = config_manager_import_json(json_buffer, config);
    free(json_buffer);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Configuration loaded successfully");
    }
    
    return ret;
}

esp_err_t config_manager_save(const launcher_config_t *config) {
    if (!spiffs_mounted || !config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // First save to backup
    if (rename(CONFIG_FILE, CONFIG_BACKUP) != 0 && errno != ENOENT) {
        ESP_LOGW(TAG, "Failed to create backup: %d", errno);
    }
    
    // Export to JSON
    char *json_buffer = malloc(8192);
    if (!json_buffer) {
        return ESP_ERR_NO_MEM;
    }
    
    esp_err_t ret = config_manager_export_json(config, json_buffer, 8192);
    if (ret != ESP_OK) {
        free(json_buffer);
        return ret;
    }
    
    // Write to file
    FILE *f = fopen(CONFIG_FILE, "w");
    if (!f) {
        free(json_buffer);
        ESP_LOGE(TAG, "Failed to open config file for writing");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(json_buffer, 1, strlen(json_buffer), f);
    fclose(f);
    free(json_buffer);
    
    if (written != strlen(json_buffer)) {
        ESP_LOGE(TAG, "Failed to write complete configuration");
        // Try to restore backup
        rename(CONFIG_BACKUP, CONFIG_FILE);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    return ESP_OK;
}

esp_err_t config_manager_reset_defaults(launcher_config_t *config) {
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    set_default_config(config);
    ESP_LOGI(TAG, "Configuration reset to defaults");
    return ESP_OK;
}

esp_err_t config_manager_backup_to_sd(const char *filename) {
    if (!spiffs_mounted || !filename) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Export current config to JSON
    char *json_buffer = malloc(8192);
    if (!json_buffer) {
        return ESP_ERR_NO_MEM;
    }
    
    esp_err_t ret = config_manager_export_json(&current_config, json_buffer, 8192);
    if (ret != ESP_OK) {
        free(json_buffer);
        return ret;
    }
    
    // Write to SD card
    FILE *f = sd_manager_open_file(filename, "w");
    if (!f) {
        free(json_buffer);
        ESP_LOGE(TAG, "Failed to create backup file on SD card");
        return ESP_FAIL;
    }
    
    size_t written = fwrite(json_buffer, 1, strlen(json_buffer), f);
    fclose(f);
    free(json_buffer);
    
    if (written != strlen(json_buffer)) {
        ESP_LOGE(TAG, "Failed to write complete backup");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Configuration backed up to SD: %s", filename);
    return ESP_OK;
}

esp_err_t config_manager_restore_from_sd(const char *filename) {
    if (!spiffs_mounted || !filename) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Get file size
    size_t file_size = sd_manager_get_file_size(filename);
    if (file_size == 0 || file_size > 16384) {
        ESP_LOGE(TAG, "Invalid backup file size: %zu", file_size);
        return ESP_FAIL;
    }
    
    // Read from SD card
    FILE *f = sd_manager_open_file(filename, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open backup file from SD card");
        return ESP_FAIL;
    }
    
    char *json_buffer = malloc(file_size + 1);
    if (!json_buffer) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }
    
    size_t bytes_read = fread(json_buffer, 1, file_size, f);
    fclose(f);
    json_buffer[bytes_read] = '\0';
    
    // Parse and validate
    launcher_config_t temp_config;
    esp_err_t ret = config_manager_import_json(json_buffer, &temp_config);
    free(json_buffer);
    
    if (ret == ESP_OK && config_manager_validate(&temp_config)) {
        // Save the restored configuration
        memcpy(&current_config, &temp_config, sizeof(launcher_config_t));
        ret = config_manager_save(&current_config);
        ESP_LOGI(TAG, "Configuration restored from SD: %s", filename);
    } else {
        ESP_LOGE(TAG, "Invalid configuration in backup file");
        ret = ESP_FAIL;
    }
    
    return ret;
}

bool config_manager_validate(const launcher_config_t *config) {
    if (!config) {
        return false;
    }
    
    // Check magic number
    if (config->magic != CONFIG_MAGIC) {
        ESP_LOGE(TAG, "Invalid magic number: 0x%08" PRIX32, config->magic);
        return false;
    }
    
    // Check version compatibility
    if (config->version > CONFIG_VERSION) {
        ESP_LOGE(TAG, "Configuration version %" PRIu32 " is newer than supported %d", 
                 config->version, CONFIG_VERSION);
        return false;
    }
    
    // Validate ranges
    if (config->system.brightness > 100 || 
        config->system.volume > 100 ||
        config->file_browser.items_per_page == 0 ||
        config->file_browser.items_per_page > 50) {
        ESP_LOGE(TAG, "Configuration values out of range");
        return false;
    }
    
    return true;
}

esp_err_t config_manager_export_json(const launcher_config_t *config, char *json_buffer, size_t buffer_size) {
    if (!config || !json_buffer || buffer_size < 1024) {
        return ESP_ERR_INVALID_ARG;
    }
    
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }
    
    // Add version and magic
    cJSON_AddNumberToObject(root, "version", config->version);
    cJSON_AddNumberToObject(root, "magic", config->magic);
    
    // System configuration
    cJSON *system = cJSON_CreateObject();
    cJSON_AddNumberToObject(system, "brightness", config->system.brightness);
    cJSON_AddNumberToObject(system, "volume", config->system.volume);
    cJSON_AddNumberToObject(system, "timeout_minutes", config->system.timeout_minutes);
    cJSON_AddBoolToObject(system, "auto_mount_sd", config->system.auto_mount_sd);
    cJSON_AddBoolToObject(system, "enable_animations", config->system.enable_animations);
    cJSON_AddBoolToObject(system, "enable_haptic", config->system.enable_haptic);
    cJSON_AddStringToObject(system, "language", config->system.language);
    cJSON_AddItemToObject(root, "system", system);
    
    // File browser configuration
    cJSON *browser = cJSON_CreateObject();
    cJSON_AddNumberToObject(browser, "view_mode", config->file_browser.view_mode);
    cJSON_AddNumberToObject(browser, "sort_by", config->file_browser.sort_by);
    cJSON_AddBoolToObject(browser, "sort_ascending", config->file_browser.sort_ascending);
    cJSON_AddBoolToObject(browser, "show_hidden_files", config->file_browser.show_hidden_files);
    cJSON_AddBoolToObject(browser, "show_file_extensions", config->file_browser.show_file_extensions);
    cJSON_AddBoolToObject(browser, "show_file_sizes", config->file_browser.show_file_sizes);
    cJSON_AddBoolToObject(browser, "confirm_delete", config->file_browser.confirm_delete);
    cJSON_AddNumberToObject(browser, "items_per_page", config->file_browser.items_per_page);
    cJSON_AddItemToObject(root, "file_browser", browser);
    
    // Network configuration
    cJSON *network = cJSON_CreateObject();
    cJSON_AddBoolToObject(network, "wifi_enabled", config->network.wifi_enabled);
    cJSON_AddStringToObject(network, "ssid", config->network.ssid);
    cJSON_AddStringToObject(network, "password", config->network.password);
    cJSON_AddBoolToObject(network, "ap_mode", config->network.ap_mode);
    cJSON_AddStringToObject(network, "ap_ssid", config->network.ap_ssid);
    cJSON_AddStringToObject(network, "ap_password", config->network.ap_password);
    cJSON_AddNumberToObject(network, "ap_channel", config->network.ap_channel);
    cJSON_AddBoolToObject(network, "web_interface_enabled", config->network.web_interface_enabled);
    cJSON_AddNumberToObject(network, "web_port", config->network.web_port);
    cJSON_AddItemToObject(root, "network", network);
    
    // Python configuration
    cJSON *python = cJSON_CreateObject();
    cJSON_AddBoolToObject(python, "auto_reload", config->python.auto_reload);
    cJSON_AddBoolToObject(python, "show_line_numbers", config->python.show_line_numbers);
    cJSON_AddBoolToObject(python, "syntax_highlighting", config->python.syntax_highlighting);
    cJSON_AddNumberToObject(python, "tab_size", config->python.tab_size);
    cJSON_AddBoolToObject(python, "use_spaces", config->python.use_spaces);
    cJSON_AddStringToObject(python, "default_path", config->python.default_path);
    cJSON_AddItemToObject(root, "python", python);
    
    // Theme configuration
    cJSON *theme = cJSON_CreateObject();
    cJSON_AddStringToObject(theme, "name", config->theme.name);
    cJSON_AddNumberToObject(theme, "primary_color", config->theme.primary_color);
    cJSON_AddNumberToObject(theme, "secondary_color", config->theme.secondary_color);
    cJSON_AddNumberToObject(theme, "background_color", config->theme.background_color);
    cJSON_AddNumberToObject(theme, "text_color", config->theme.text_color);
    cJSON_AddNumberToObject(theme, "accent_color", config->theme.accent_color);
    cJSON_AddItemToObject(root, "theme", theme);
    
    // Print to buffer
    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }
    
    if (strlen(json_str) >= buffer_size) {
        free(json_str);
        cJSON_Delete(root);
        return ESP_ERR_INVALID_SIZE;
    }
    
    strcpy(json_buffer, json_str);
    free(json_str);
    cJSON_Delete(root);
    
    return ESP_OK;
}

esp_err_t config_manager_import_json(const char *json_string, launcher_config_t *config) {
    if (!json_string || !config) {
        return ESP_ERR_INVALID_ARG;
    }
    
    cJSON *root = cJSON_Parse(json_string);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }
    
    // Start with defaults
    set_default_config(config);
    
    // Parse version and magic
    cJSON *item = cJSON_GetObjectItem(root, "version");
    if (item && cJSON_IsNumber(item)) {
        config->version = item->valueint;
    }
    
    item = cJSON_GetObjectItem(root, "magic");
    if (item && cJSON_IsNumber(item)) {
        config->magic = item->valueint;
    }
    
    // Parse system configuration
    cJSON *system = cJSON_GetObjectItem(root, "system");
    if (system) {
        item = cJSON_GetObjectItem(system, "brightness");
        if (item && cJSON_IsNumber(item)) config->system.brightness = item->valueint;
        
        item = cJSON_GetObjectItem(system, "volume");
        if (item && cJSON_IsNumber(item)) config->system.volume = item->valueint;
        
        item = cJSON_GetObjectItem(system, "timeout_minutes");
        if (item && cJSON_IsNumber(item)) config->system.timeout_minutes = item->valueint;
        
        item = cJSON_GetObjectItem(system, "auto_mount_sd");
        if (item && cJSON_IsBool(item)) config->system.auto_mount_sd = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(system, "enable_animations");
        if (item && cJSON_IsBool(item)) config->system.enable_animations = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(system, "enable_haptic");
        if (item && cJSON_IsBool(item)) config->system.enable_haptic = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(system, "language");
        if (item && cJSON_IsString(item)) {
            strncpy(config->system.language, item->valuestring, sizeof(config->system.language) - 1);
        }
    }
    
    // Parse file browser configuration
    cJSON *browser = cJSON_GetObjectItem(root, "file_browser");
    if (browser) {
        item = cJSON_GetObjectItem(browser, "view_mode");
        if (item && cJSON_IsNumber(item)) config->file_browser.view_mode = item->valueint;
        
        item = cJSON_GetObjectItem(browser, "sort_by");
        if (item && cJSON_IsNumber(item)) config->file_browser.sort_by = item->valueint;
        
        item = cJSON_GetObjectItem(browser, "sort_ascending");
        if (item && cJSON_IsBool(item)) config->file_browser.sort_ascending = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(browser, "show_hidden_files");
        if (item && cJSON_IsBool(item)) config->file_browser.show_hidden_files = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(browser, "show_file_extensions");
        if (item && cJSON_IsBool(item)) config->file_browser.show_file_extensions = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(browser, "show_file_sizes");
        if (item && cJSON_IsBool(item)) config->file_browser.show_file_sizes = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(browser, "confirm_delete");
        if (item && cJSON_IsBool(item)) config->file_browser.confirm_delete = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(browser, "items_per_page");
        if (item && cJSON_IsNumber(item)) config->file_browser.items_per_page = item->valueint;
    }
    
    // Parse network configuration
    cJSON *network = cJSON_GetObjectItem(root, "network");
    if (network) {
        item = cJSON_GetObjectItem(network, "wifi_enabled");
        if (item && cJSON_IsBool(item)) config->network.wifi_enabled = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(network, "ssid");
        if (item && cJSON_IsString(item)) {
            strncpy(config->network.ssid, item->valuestring, sizeof(config->network.ssid) - 1);
        }
        
        item = cJSON_GetObjectItem(network, "password");
        if (item && cJSON_IsString(item)) {
            strncpy(config->network.password, item->valuestring, sizeof(config->network.password) - 1);
        }
        
        item = cJSON_GetObjectItem(network, "ap_mode");
        if (item && cJSON_IsBool(item)) config->network.ap_mode = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(network, "ap_ssid");
        if (item && cJSON_IsString(item)) {
            strncpy(config->network.ap_ssid, item->valuestring, sizeof(config->network.ap_ssid) - 1);
        }
        
        item = cJSON_GetObjectItem(network, "ap_password");
        if (item && cJSON_IsString(item)) {
            strncpy(config->network.ap_password, item->valuestring, sizeof(config->network.ap_password) - 1);
        }
        
        item = cJSON_GetObjectItem(network, "ap_channel");
        if (item && cJSON_IsNumber(item)) config->network.ap_channel = item->valueint;
        
        item = cJSON_GetObjectItem(network, "web_interface_enabled");
        if (item && cJSON_IsBool(item)) config->network.web_interface_enabled = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(network, "web_port");
        if (item && cJSON_IsNumber(item)) config->network.web_port = item->valueint;
    }
    
    // Parse Python configuration
    cJSON *python = cJSON_GetObjectItem(root, "python");
    if (python) {
        item = cJSON_GetObjectItem(python, "auto_reload");
        if (item && cJSON_IsBool(item)) config->python.auto_reload = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(python, "show_line_numbers");
        if (item && cJSON_IsBool(item)) config->python.show_line_numbers = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(python, "syntax_highlighting");
        if (item && cJSON_IsBool(item)) config->python.syntax_highlighting = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(python, "tab_size");
        if (item && cJSON_IsNumber(item)) config->python.tab_size = item->valueint;
        
        item = cJSON_GetObjectItem(python, "use_spaces");
        if (item && cJSON_IsBool(item)) config->python.use_spaces = cJSON_IsTrue(item);
        
        item = cJSON_GetObjectItem(python, "default_path");
        if (item && cJSON_IsString(item)) {
            strncpy(config->python.default_path, item->valuestring, sizeof(config->python.default_path) - 1);
        }
    }
    
    // Parse theme configuration
    cJSON *theme = cJSON_GetObjectItem(root, "theme");
    if (theme) {
        item = cJSON_GetObjectItem(theme, "name");
        if (item && cJSON_IsString(item)) {
            strncpy(config->theme.name, item->valuestring, sizeof(config->theme.name) - 1);
        }
        
        item = cJSON_GetObjectItem(theme, "primary_color");
        if (item && cJSON_IsNumber(item)) config->theme.primary_color = item->valueint;
        
        item = cJSON_GetObjectItem(theme, "secondary_color");
        if (item && cJSON_IsNumber(item)) config->theme.secondary_color = item->valueint;
        
        item = cJSON_GetObjectItem(theme, "background_color");
        if (item && cJSON_IsNumber(item)) config->theme.background_color = item->valueint;
        
        item = cJSON_GetObjectItem(theme, "text_color");
        if (item && cJSON_IsNumber(item)) config->theme.text_color = item->valueint;
        
        item = cJSON_GetObjectItem(theme, "accent_color");
        if (item && cJSON_IsNumber(item)) config->theme.accent_color = item->valueint;
    }
    
    cJSON_Delete(root);
    return ESP_OK;
}

launcher_config_t* config_manager_get_current(void) {
    return &current_config;
}

bool config_manager_is_ready(void) {
    return spiffs_mounted;
}