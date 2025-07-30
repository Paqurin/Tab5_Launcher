#include "firmware_loader.h"
#include "sd_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_format.h"
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