#include "firmware_loader.h"
#include "sd_manager.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash.h"
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "FIRMWARE_LOADER";

#define BUFFER_SIZE 4096

esp_err_t firmware_loader_init(void) {
    ESP_LOGI(TAG, "Firmware loader initialized");
    return ESP_OK;
}

static bool is_valid_firmware_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    // Check for .bin extension
    return (strcmp(&filename[len-4], ".bin") == 0);
}

static esp_err_t validate_firmware_header(FILE *file) {
    esp_image_header_t header;
    
    // Read the header
    fseek(file, 0, SEEK_SET);
    if (fread(&header, sizeof(header), 1, file) != 1) {
        ESP_LOGE(TAG, "Failed to read firmware header");
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Basic validation
    if (header.magic != ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGE(TAG, "Invalid firmware magic: 0x%02x", header.magic);
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Firmware header validated successfully");
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
    
    // Validate firmware header
    esp_err_t ret = validate_firmware_header(file);
    if (ret != ESP_OK) {
        fclose(file);
        return ret;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    ESP_LOGI(TAG, "Firmware file size: %zu bytes", file_size);
    
    // Always use OTA_0 partition for user firmware
    const esp_partition_t *update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    
    if (!update_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        fclose(file);
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Writing to partition: %s (0x%08" PRIx32 ", %" PRIu32 " bytes)", 
             update_partition->label, update_partition->address, update_partition->size);
    
    // Check if firmware fits in partition
    if (file_size > update_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %" PRIu32, file_size, update_partition->size);
        fclose(file);
        return ESP_ERR_INVALID_SIZE;
    }
    
    // Erase the target partition first
    if (progress_callback) progress_callback(0, file_size, "Erasing partition...");
    ESP_LOGI(TAG, "Erasing target partition...");
    ret = esp_partition_erase_range(update_partition, 0, update_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase partition: %s", esp_err_to_name(ret));
        fclose(file);
        return ret;
    }
    
    // Begin OTA update
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
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        ret = esp_ota_write(update_handle, buffer, bytes_read);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(ret));
            esp_ota_abort(update_handle);
            fclose(file);
            return ret;
        }
        
        bytes_written += bytes_read;
        
        // Update progress callback with reduced frequency to avoid LVGL conflicts
        if (progress_callback && (bytes_written % (32 * 1024) == 0 || bytes_written == file_size)) {
            progress_callback(bytes_written, file_size, "Writing firmware...");
        }
        
        // Log progress every 64KB
        if (bytes_written % (64 * 1024) == 0) {
            ESP_LOGI(TAG, "Progress: %zu / %zu bytes (%.1f%%)", 
                     bytes_written, file_size, (float)bytes_written / file_size * 100);
        }
    }
    
    fclose(file);
    
    ESP_LOGI(TAG, "Firmware written successfully: %zu bytes", bytes_written);
    
    // Finalize OTA update
    if (progress_callback) progress_callback(file_size, file_size, "Finalizing...");
    ret = esp_ota_end(update_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set boot partition to the new firmware and automatically reboot
    if (progress_callback) progress_callback(file_size, file_size, "Setting boot partition...");
    ret = esp_ota_set_boot_partition(update_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Boot partition set to: %s", update_partition->label);
    if (progress_callback) progress_callback(file_size, file_size, "Rebooting to new firmware...");
    ESP_LOGI(TAG, "Firmware flash completed successfully - rebooting to new firmware");
    
    vTaskDelay(pdMS_TO_TICKS(2000)); // Give time for UI update and logs
    esp_restart();
    
    return ESP_OK; // Never reached
}

bool firmware_loader_is_firmware_ready(void) {
    // Check if there's valid firmware in the OTA_0 partition
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    
    if (!ota_partition) {
        return false;
    }
    
    // Check if the partition has valid firmware
    esp_app_desc_t app_desc;
    esp_err_t ret = esp_ota_get_partition_description(ota_partition, &app_desc);
    return (ret == ESP_OK);
}

esp_err_t firmware_loader_restart_to_new_firmware(void) {
    // Boot to the OTA_0 partition where user firmware is stored
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    
    if (ota_partition) {
        esp_err_t ret = esp_ota_set_boot_partition(ota_partition);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret));
            return ret;
        }
        ESP_LOGI(TAG, "Boot partition set to: %s", ota_partition->label);
    }
    
    ESP_LOGI(TAG, "Restarting to user firmware...");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Give time for logs
    esp_restart();
    return ESP_OK; // Never reached
}

int firmware_loader_scan_firmware_files(const char *directory, firmware_info_t *firmware_list, int max_count) {
    ESP_LOGI(TAG, "Scanning directory: %s", directory);
    
    file_entry_t entries[32];
    int entry_count = sd_manager_scan_directory(directory, entries, 32);
    
    ESP_LOGI(TAG, "Found %d entries in directory", entry_count);
    
    if (entry_count <= 0) {
        ESP_LOGW(TAG, "No entries found in directory %s", directory);
        return 0;
    }
    
    int firmware_count = 0;
    for (int i = 0; i < entry_count && firmware_count < max_count; i++) {
        ESP_LOGI(TAG, "Entry %d: %s (dir: %s, size: %zu)", 
                 i, entries[i].name, entries[i].is_directory ? "yes" : "no", entries[i].size);
        
        if (!entries[i].is_directory && is_valid_firmware_file(entries[i].name)) {
            ESP_LOGI(TAG, "Valid firmware file found: %s", entries[i].name);
            
            strncpy(firmware_list[firmware_count].filename, entries[i].name, 
                   sizeof(firmware_list[firmware_count].filename) - 1);
            firmware_list[firmware_count].filename[sizeof(firmware_list[firmware_count].filename) - 1] = '\0';
            firmware_list[firmware_count].size = entries[i].size;
            
            // Create full path with bounds checking
            size_t dir_len = strlen(directory);
            size_t name_len = strlen(entries[i].name);
            
            // Check if the combined path would fit (including '/' and '\0')
            if (dir_len + name_len + 2 < sizeof(firmware_list[firmware_count].full_path)) {
                if (strcmp(directory, "/") == 0) {
                    // Root directory case: "/<filename>"
                    firmware_list[firmware_count].full_path[0] = '/';
                    strcpy(firmware_list[firmware_count].full_path + 1, entries[i].name);
                } else {
                    // Non-root directory case: "<directory>/<filename>"
                    strcpy(firmware_list[firmware_count].full_path, directory);
                    strcat(firmware_list[firmware_count].full_path, "/");
                    strcat(firmware_list[firmware_count].full_path, entries[i].name);
                }
                ESP_LOGI(TAG, "Added firmware: %s -> %s", 
                         firmware_list[firmware_count].filename, 
                         firmware_list[firmware_count].full_path);
                firmware_count++;
            } else {
                ESP_LOGW(TAG, "Firmware path too long, skipping: %s/%s", directory, entries[i].name);
            }
        } else if (!entries[i].is_directory) {
            ESP_LOGI(TAG, "File %s is not a valid firmware file", entries[i].name);
        }
    }
    
    ESP_LOGI(TAG, "Found %d firmware files in %s", firmware_count, directory);
    return firmware_count;
}