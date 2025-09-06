#include "firmware_loader.h"
#include "sd_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_format.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static const char *TAG = "FIRMWARE_CORE";
#define BUFFER_SIZE 4096

static bool is_valid_firmware_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    return (strcmp(&filename[len-4], ".bin") == 0);
}

static esp_err_t validate_firmware_header(FILE *file) {
    esp_image_header_t header;
    
    fseek(file, 0, SEEK_SET);
    if (fread(&header, sizeof(header), 1, file) != 1) {
        ESP_LOGE(TAG, "Failed to read firmware header");
        return ESP_ERR_INVALID_SIZE;
    }
    
    if (header.magic != ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGE(TAG, "Invalid firmware magic: 0x%02x", header.magic);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Firmware header validated successfully");
    return ESP_OK;
}

esp_err_t firmware_loader_init(void) {
    ESP_LOGI(TAG, "Firmware loader initialized");
    return ESP_OK;
}

esp_err_t firmware_loader_flash_from_sd_with_progress(const char *firmware_path, firmware_progress_callback_t progress_callback) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!is_valid_firmware_file(firmware_path)) {
        ESP_LOGE(TAG, "Invalid firmware file: %s", firmware_path);
        return ESP_ERR_INVALID_ARG;
    }
    
    FILE *file = sd_manager_open_file(firmware_path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open firmware file: %s", firmware_path);
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_err_t ret = validate_firmware_header(file);
    if (ret != ESP_OK) {
        fclose(file);
        return ret;
    }
    
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    ESP_LOGI(TAG, "Firmware file size: %zu bytes", file_size);
    
    const esp_partition_t *update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    
    if (!update_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        fclose(file);
        return ESP_ERR_NOT_FOUND;
    }
    
    if (file_size > update_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %" PRIu32, file_size, update_partition->size);
        fclose(file);
        return ESP_ERR_INVALID_SIZE;
    }
    
    if (progress_callback) progress_callback(0, file_size, "Erasing partition...");
    ret = esp_partition_erase_range(update_partition, 0, update_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase partition: %s", esp_err_to_name(ret));
        fclose(file);
        return ret;
    }
    
    if (progress_callback) progress_callback(0, file_size, "Starting firmware write...");
    esp_ota_handle_t update_handle = 0;
    ret = esp_ota_begin(update_partition, file_size, &update_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(ret));
        fclose(file);
        return ret;
    }
    
    // Flash firmware in chunks
    uint8_t buffer[BUFFER_SIZE];
    size_t bytes_written = 0;
    
    while (bytes_written < file_size) {
        size_t bytes_to_read = (file_size - bytes_written > BUFFER_SIZE) ? BUFFER_SIZE : (file_size - bytes_written);
        size_t bytes_read = fread(buffer, 1, bytes_to_read, file);
        
        if (bytes_read == 0) {
            ESP_LOGE(TAG, "Failed to read from file");
            esp_ota_abort(update_handle);
            fclose(file);
            return ESP_ERR_INVALID_STATE;
        }
        
        ret = esp_ota_write(update_handle, buffer, bytes_read);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(ret));
            esp_ota_abort(update_handle);
            fclose(file);
            return ret;
        }
        
        bytes_written += bytes_read;
        
        if (progress_callback) {
            progress_callback(bytes_written, file_size, "Writing firmware...");
        }
    }
    
    fclose(file);
    
    if (progress_callback) progress_callback(file_size, file_size, "Finalizing...");
    ret = esp_ota_end(update_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Firmware flashed successfully");
    return ESP_OK;
}

bool firmware_loader_is_firmware_ready(void) {
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (!ota_partition) {
        return false;
    }
    
    esp_image_header_t header;
    esp_err_t ret = esp_partition_read(ota_partition, 0, &header, sizeof(header));
    if (ret != ESP_OK) {
        return false;
    }
    
    return (header.magic == ESP_IMAGE_HEADER_MAGIC);
}

esp_err_t firmware_loader_get_firmware_info(esp_app_desc_t *app_desc) {
    if (!app_desc) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Get the OTA_0 partition where user firmware is stored
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    
    if (!ota_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    // Try to get app description from OTA partition
    esp_err_t ret = esp_ota_get_partition_description(ota_partition, app_desc);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No valid firmware found in OTA_0 partition");
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Found firmware: %s v%s", app_desc->project_name, app_desc->version);
    return ESP_OK;
}

esp_err_t firmware_loader_unload_firmware(void) {
    ESP_LOGI(TAG, "=== UNLOADING/EJECTING FIRMWARE ===");
    
    // Get the OTA_0 partition where user firmware is stored
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    
    if (!ota_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    // Check if there's firmware currently loaded
    esp_app_desc_t app_desc;
    esp_err_t ret = esp_ota_get_partition_description(ota_partition, &app_desc);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "No firmware found in OTA_0 partition - nothing to unload");
        return ESP_OK; // Not an error, just nothing to do
    }
    
    ESP_LOGI(TAG, "Found firmware to unload: %s v%s", app_desc.project_name, app_desc.version);
    
    // Clear any NVS boot flags
    nvs_handle_t nvs_handle;
    ret = nvs_open("launcher", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        uint8_t boot_firmware_once = 0;
        nvs_set_blob(nvs_handle, "boot_fw_once", &boot_firmware_once, sizeof(boot_firmware_once));
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "Cleared NVS boot flags");
    } else {
        ESP_LOGW(TAG, "Failed to open NVS to clear boot flags: %s", esp_err_to_name(ret));
    }
    
    // Ensure factory (launcher) partition is set as boot partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition) {
        ret = esp_ota_set_boot_partition(factory_partition);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Boot partition set back to launcher (factory)");
        } else {
            ESP_LOGW(TAG, "Failed to set boot partition to factory: %s", esp_err_to_name(ret));
        }
    }
    
    // Erase the OTA partition to completely remove the firmware
    ESP_LOGI(TAG, "Erasing OTA_0 partition to remove firmware...");
    ret = esp_partition_erase_range(ota_partition, 0, ota_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase OTA partition: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ Firmware successfully unloaded/ejected from OTA partition");
    ESP_LOGI(TAG, "✓ Boot partition restored to launcher");
    ESP_LOGI(TAG, "✓ NVS boot flags cleared");
    
    return ESP_OK;
}