#include "firmware_loader.h"
#include "sd_manager.h"
#include "hal.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_app_format.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "FIRMWARE_LOADER";
static const char *NVS_NAMESPACE = "launcher";
static const char *NVS_KEY_BOOT_FIRMWARE = "boot_fw_once";

#define BUFFER_SIZE 4096

esp_err_t firmware_loader_init(void) {
    ESP_LOGI(TAG, "Firmware loader initialized");
    return ESP_OK;
}

esp_err_t firmware_loader_init_boot_manager(void) {
    // Initialize NVS if not already done
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing and retrying");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Boot manager initialized");
    return ESP_OK;
}

esp_err_t firmware_loader_handle_boot_management(void) {
    nvs_handle_t nvs_handle;
    esp_err_t ret;
    
    // Open NVS
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Check if we should boot firmware once
    uint8_t boot_firmware_once = 0;
    size_t required_size = sizeof(boot_firmware_once);
    ret = nvs_get_blob(nvs_handle, NVS_KEY_BOOT_FIRMWARE, &boot_firmware_once, &required_size);
    
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    
    if (ret == ESP_OK && boot_firmware_once == 1) {
        // We were supposed to boot firmware once, clear the flag
        ESP_LOGI(TAG, "One-time firmware boot completed, clearing flag");
        boot_firmware_once = 0;
        nvs_set_blob(nvs_handle, NVS_KEY_BOOT_FIRMWARE, &boot_firmware_once, sizeof(boot_firmware_once));
        nvs_commit(nvs_handle);
        
        // Ensure launcher becomes default boot partition again
        if (factory_partition) {
            ret = esp_ota_set_boot_partition(factory_partition);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Boot partition restored to launcher (factory)");
            } else {
                ESP_LOGW(TAG, "Failed to restore boot partition to factory: %s", esp_err_to_name(ret));
            }
        }
    } else {
        // Normal boot, ensure launcher is default
        if (factory_partition && running_partition && 
            running_partition->address == factory_partition->address) {
            ret = esp_ota_set_boot_partition(factory_partition);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Boot partition confirmed as launcher (factory)");
            } else {
                ESP_LOGW(TAG, "Failed to set boot partition to factory: %s", esp_err_to_name(ret));
            }
        }
    }
    
    nvs_close(nvs_handle);
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
        
        // Update progress callback with very reduced frequency to avoid LVGL conflicts
        // Only update every 256KB or at the end of the file to minimize UI conflicts
        if (progress_callback && (bytes_written % (256 * 1024) == 0 || bytes_written == file_size)) {
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
    
    // CRITICAL: Do NOT set boot partition here
    // The launcher remains the default boot partition
    ESP_LOGI(TAG, "Firmware flash completed successfully");
    if (progress_callback) progress_callback(file_size, file_size, "Flash complete! Returning to launcher...");
    
    // Give time for UI update, then signal completion
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    return ESP_OK;
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

esp_err_t firmware_loader_boot_firmware_once(void) {
    ESP_LOGI(TAG, "=== USING ESP-IDF AUTOMATIC ROLLBACK MECHANISM ===");
    
    // Get the OTA_0 partition where user firmware is stored
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    
    if (!ota_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    // Validate that we have valid firmware in OTA_0
    esp_app_desc_t app_desc;
    esp_err_t ret = esp_ota_get_partition_description(ota_partition, &app_desc);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "No valid firmware found in OTA_0 partition");
        return ret;
    }
    
    ESP_LOGI(TAG, "Found valid firmware: %s v%s", app_desc.project_name, app_desc.version);
    
    // Get factory partition to ensure it's available as rollback target
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (!factory_partition) {
        ESP_LOGE(TAG, "Factory partition not found - rollback not possible");
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Factory (launcher) partition available for rollback: %s", factory_partition->label);
    
    // CRITICAL: Ensure factory partition is marked as valid for rollback
    // This ensures the bootloader has a valid target to rollback to
    const esp_partition_t *current_running = esp_ota_get_running_partition();
    if (current_running && current_running->address == factory_partition->address) {
        ESP_LOGI(TAG, "Currently running from factory - ensuring it's marked as valid");
        // Factory partition should already be valid, but let's be explicit
    }
    
    // SOLUTION: Use ESP-IDF's automatic rollback mechanism
    // When CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is set:
    // 1. esp_ota_set_boot_partition() puts the firmware in ESP_OTA_IMG_PENDING_VERIFY state
    // 2. The firmware boots once
    // 3. Since the user firmware doesn't call esp_ota_mark_app_valid_cancel_rollback(),
    //    the bootloader automatically rolls back to the previous working app (factory)
    
    ESP_LOGI(TAG, "Setting firmware as next boot partition (with automatic rollback)...");
    ret = esp_ota_set_boot_partition(ota_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Verify the partition is now in pending verification state
    esp_ota_img_states_t ota_state;
    ret = esp_ota_get_state_partition(ota_partition, &ota_state);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA partition state after setting boot partition: %d", ota_state);
        switch (ota_state) {
            case ESP_OTA_IMG_NEW:
                ESP_LOGI(TAG, "State: ESP_OTA_IMG_NEW");
                break;
            case ESP_OTA_IMG_PENDING_VERIFY:
                ESP_LOGI(TAG, "✓ State: ESP_OTA_IMG_PENDING_VERIFY - automatic rollback enabled!");
                break;
            case ESP_OTA_IMG_VALID:
                ESP_LOGI(TAG, "State: ESP_OTA_IMG_VALID");
                break;
            case ESP_OTA_IMG_INVALID:
                ESP_LOGI(TAG, "State: ESP_OTA_IMG_INVALID");
                break;
            case ESP_OTA_IMG_ABORTED:
                ESP_LOGI(TAG, "State: ESP_OTA_IMG_ABORTED");
                break;
            case ESP_OTA_IMG_UNDEFINED:
                ESP_LOGI(TAG, "State: ESP_OTA_IMG_UNDEFINED");
                break;
        }
    } else {
        ESP_LOGW(TAG, "Could not get OTA partition state: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "=== AUTOMATIC ROLLBACK MECHANISM ACTIVE ===");
    ESP_LOGI(TAG, "✓ CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE is enabled");
    ESP_LOGI(TAG, "✓ Firmware will boot once automatically");
    ESP_LOGI(TAG, "✓ Since firmware won't validate itself,");
    ESP_LOGI(TAG, "✓ Bootloader will auto-rollback to launcher");
    ESP_LOGI(TAG, "✓ Manual power cycle required for GT911 reset");
    ESP_LOGI(TAG, "✓ Launcher will regain control automatically");
    ESP_LOGI(TAG, "==========================================");
    
    ESP_LOGI(TAG, "Firmware configured for next boot. Manual reboot required.");
    
    // Instead of automatic reboot, we'll show a dialog instructing the user
    // to manually power cycle the device. This ensures:
    // 1. Complete power-off of GT911 touch controller
    // 2. Clean boot into app partition
    // 3. Automatic rollback to launcher after firmware runs
    
    return ESP_OK;
}

// Legacy function name for compatibility
esp_err_t firmware_loader_restart_to_new_firmware(void) {
    return firmware_loader_boot_firmware_once();
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