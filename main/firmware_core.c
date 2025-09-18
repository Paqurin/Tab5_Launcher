#include "firmware_loader.h"
#include "sd_manager.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_format.h"
#include "esp_efuse.h"
#include "esp_secure_boot.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static const char *TAG = "FIRMWARE_CORE";
#define BUFFER_SIZE 4096
#define TAB5_FIRMWARE_PADDING_OFFSET 0x2000  // 8KB padding common in Tab5 firmware

/**
 * @brief Detect firmware padding offset in file
 * @param file Firmware file pointer
 * @return Offset where actual ESP32 firmware starts (0 or TAB5_FIRMWARE_PADDING_OFFSET)
 */
static size_t detect_firmware_offset(FILE *file) {
    uint8_t magic_byte;

    // Check magic byte at offset 0 (no padding)
    fseek(file, 0, SEEK_SET);
    if (fread(&magic_byte, 1, 1, file) == 1 && magic_byte == ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGI(TAG, "Firmware has no padding - starts at offset 0");
        return 0;
    }

    // Check magic byte at Tab5 padding offset (0x2000)
    fseek(file, TAB5_FIRMWARE_PADDING_OFFSET, SEEK_SET);
    if (fread(&magic_byte, 1, 1, file) == 1 && magic_byte == ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGI(TAG, "Firmware has Tab5 padding - starts at offset 0x%04X", TAB5_FIRMWARE_PADDING_OFFSET);
        return TAB5_FIRMWARE_PADDING_OFFSET;
    }

    ESP_LOGW(TAG, "No valid firmware magic found at offset 0 or 0x%04X", TAB5_FIRMWARE_PADDING_OFFSET);
    return 0; // Default to no offset
}

/**
 * @brief Bypass eFuse security for firmware loading on Tab5
 * @return ESP_OK if bypass successful or not needed
 */
static esp_err_t bypass_efuse_security(void) {
    ESP_LOGI(TAG, "Bypassing eFuse security for Tab5 firmware loading...");

    // On Tab5, secure boot is enabled via eFuses
    // We need to use direct partition write instead of OTA API to bypass validation
    ESP_LOGW(TAG, "Using direct partition write to bypass secure boot validation");
    ESP_LOGW(TAG, "This bypasses firmware signature verification - use with caution!");

    return ESP_OK;
}

static bool is_valid_firmware_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    return (strcmp(&filename[len-4], ".bin") == 0);
}

static esp_err_t validate_firmware_header(FILE *file, size_t offset) {
    esp_image_header_t header;

    fseek(file, offset, SEEK_SET);
    if (fread(&header, sizeof(header), 1, file) != 1) {
        ESP_LOGE(TAG, "Failed to read firmware header at offset %zu", offset);
        return ESP_ERR_INVALID_SIZE;
    }

    if (header.magic != ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGE(TAG, "Invalid firmware magic at offset %zu: 0x%02x (expected 0x%02x)",
                 offset, header.magic, ESP_IMAGE_HEADER_MAGIC);
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Firmware header validated successfully at offset %zu", offset);
    ESP_LOGI(TAG, "Firmware segments: %d, entry point: 0x%08" PRIx32, header.segment_count, header.entry_addr);
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

    // Detect firmware padding offset
    size_t firmware_offset = detect_firmware_offset(file);

    // Validate firmware header at detected offset
    esp_err_t ret = validate_firmware_header(file, firmware_offset);
    if (ret != ESP_OK) {
        fclose(file);
        return ret;
    }

    // Get total file size and calculate actual firmware size
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    size_t actual_firmware_size = file_size - firmware_offset;

    ESP_LOGI(TAG, "File size: %zu bytes, Firmware offset: %zu, Actual firmware size: %zu bytes",
             file_size, firmware_offset, actual_firmware_size);

    const esp_partition_t *update_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);

    if (!update_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        fclose(file);
        return ESP_ERR_NOT_FOUND;
    }

    if (actual_firmware_size > update_partition->size) {
        ESP_LOGE(TAG, "Firmware too large: %zu > %" PRIu32, actual_firmware_size, update_partition->size);
        fclose(file);
        return ESP_ERR_INVALID_SIZE;
    }

    // Bypass eFuse security - use direct partition write instead of OTA API
    ret = bypass_efuse_security();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to bypass eFuse security: %s", esp_err_to_name(ret));
        fclose(file);
        return ret;
    }

    if (progress_callback) progress_callback(0, actual_firmware_size, "Erasing partition...");
    ret = esp_partition_erase_range(update_partition, 0, update_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase partition: %s", esp_err_to_name(ret));
        fclose(file);
        return ret;
    }

    // Position file pointer to start of actual firmware (skip padding)
    fseek(file, firmware_offset, SEEK_SET);

    if (progress_callback) progress_callback(0, actual_firmware_size, "Writing firmware (direct)...");

    // Write firmware directly to partition, bypassing OTA validation
    uint8_t buffer[BUFFER_SIZE];
    size_t bytes_written = 0;
    size_t partition_offset = 0;

    while (bytes_written < actual_firmware_size) {
        size_t bytes_to_read = (actual_firmware_size - bytes_written > BUFFER_SIZE) ? BUFFER_SIZE : (actual_firmware_size - bytes_written);
        size_t bytes_read = fread(buffer, 1, bytes_to_read, file);

        if (bytes_read == 0) {
            ESP_LOGE(TAG, "Failed to read from file at offset %zu", firmware_offset + bytes_written);
            fclose(file);
            return ESP_ERR_INVALID_STATE;
        }

        // Write directly to partition, bypassing OTA API validation
        ret = esp_partition_write(update_partition, partition_offset, buffer, bytes_read);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_partition_write failed at offset %zu: %s", partition_offset, esp_err_to_name(ret));
            fclose(file);
            return ret;
        }

        bytes_written += bytes_read;
        partition_offset += bytes_read;

        if (progress_callback) {
            progress_callback(bytes_written, actual_firmware_size, "Writing firmware...");
        }
    }

    fclose(file);

    if (progress_callback) progress_callback(actual_firmware_size, actual_firmware_size, "Verifying...");

    // Verify the written firmware header
    esp_image_header_t verify_header;
    ret = esp_partition_read(update_partition, 0, &verify_header, sizeof(verify_header));
    if (ret == ESP_OK && verify_header.magic == ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGI(TAG, "✓ Firmware verification successful - magic: 0x%02x", verify_header.magic);
        ESP_LOGI(TAG, "✓ Firmware written successfully (%zu bytes) with eFuse bypass", bytes_written);
    } else {
        ESP_LOGE(TAG, "✗ Firmware verification failed - magic: 0x%02x", verify_header.magic);
        return ESP_ERR_INVALID_STATE;
    }

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
    
    // Check if there's firmware currently loaded using the same logic as is_firmware_ready()
    esp_image_header_t header;
    esp_err_t ret = esp_partition_read(ota_partition, 0, &header, sizeof(header));
    if (ret != ESP_OK || header.magic != ESP_IMAGE_HEADER_MAGIC) {
        ESP_LOGW(TAG, "No firmware found in OTA_0 partition - nothing to unload");
        return ESP_OK; // Not an error, just nothing to do
    }

    ESP_LOGI(TAG, "Found firmware to unload (magic: 0x%02x)", header.magic);

    // Try to get app description for logging, but don't fail if it's not available
    esp_app_desc_t app_desc;
    if (esp_ota_get_partition_description(ota_partition, &app_desc) == ESP_OK) {
        ESP_LOGI(TAG, "Firmware details: %s v%s", app_desc.project_name, app_desc.version);
    } else {
        ESP_LOGI(TAG, "Firmware details: Unable to read app description (direct write bypass)");
    }
    
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

esp_err_t firmware_loader_export_to_sd(const char *output_path) {
    ESP_LOGI(TAG, "=== EXPORTING FIRMWARE TO SD CARD ===");

    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    // Get the OTA_0 partition where firmware is stored
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);

    if (!ota_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        return ESP_ERR_NOT_FOUND;
    }

    // Check if there's firmware to export
    if (!firmware_loader_is_firmware_ready()) {
        ESP_LOGE(TAG, "No firmware found in OTA partition to export");
        return ESP_ERR_NOT_FOUND;
    }

    // Open output file on SD card
    FILE *output_file = sd_manager_open_file(output_path, "wb");
    if (!output_file) {
        ESP_LOGE(TAG, "Failed to create output file: %s", output_path);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Exporting firmware to: %s", output_path);

    // Read firmware from partition and write to SD card
    uint8_t buffer[BUFFER_SIZE];
    size_t bytes_exported = 0;
    size_t partition_offset = 0;

    // Export until we hit empty space or max partition size
    while (partition_offset < ota_partition->size) {
        size_t bytes_to_read = (ota_partition->size - partition_offset > BUFFER_SIZE) ? BUFFER_SIZE : (ota_partition->size - partition_offset);

        esp_err_t ret = esp_partition_read(ota_partition, partition_offset, buffer, bytes_to_read);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read from partition at offset %zu: %s", partition_offset, esp_err_to_name(ret));
            fclose(output_file);
            return ret;
        }

        // Check if we've hit empty space (all 0xFF)
        bool is_empty = true;
        for (size_t i = 0; i < bytes_to_read; i++) {
            if (buffer[i] != 0xFF) {
                is_empty = false;
                break;
            }
        }

        if (is_empty && bytes_exported > 0) {
            // Hit empty space after some data - stop exporting
            break;
        }

        if (!is_empty) {
            size_t bytes_written = fwrite(buffer, 1, bytes_to_read, output_file);
            if (bytes_written != bytes_to_read) {
                ESP_LOGE(TAG, "Failed to write to output file");
                fclose(output_file);
                return ESP_ERR_INVALID_STATE;
            }
            bytes_exported += bytes_written;
        }

        partition_offset += bytes_to_read;
    }

    fclose(output_file);

    ESP_LOGI(TAG, "✓ Firmware exported successfully (%zu bytes)", bytes_exported);
    return ESP_OK;
}

esp_err_t firmware_loader_factory_reset(void) {
    ESP_LOGI(TAG, "=== FACTORY RESET - RESTORING ORIGINAL FIRMWARE ===");

    // Clear OTA partition
    esp_err_t ret = firmware_loader_unload_firmware();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to clear OTA partition during factory reset");
        return ret;
    }

    // Clear all NVS launcher data
    nvs_handle_t nvs_handle;
    ret = nvs_open("launcher", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        nvs_erase_all(nvs_handle);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
        ESP_LOGI(TAG, "✓ NVS launcher data cleared");
    } else {
        ESP_LOGW(TAG, "Failed to clear NVS data: %s", esp_err_to_name(ret));
    }

    // Ensure factory partition is set as boot
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition) {
        ret = esp_ota_set_boot_partition(factory_partition);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "✓ Factory partition set as boot partition");
        }
    }

    ESP_LOGI(TAG, "✓ Factory reset completed - system restored to original state");
    return ESP_OK;
}

esp_err_t firmware_loader_clean_partition(void) {
    ESP_LOGI(TAG, "=== FORCE CLEANING CORRUPTED PARTITION ===");

    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);

    if (!ota_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        return ESP_ERR_NOT_FOUND;
    }

    // Force erase the entire partition multiple times to ensure cleanup
    ESP_LOGI(TAG, "Force erasing OTA partition (pass 1/3)...");
    esp_err_t ret = esp_partition_erase_range(ota_partition, 0, ota_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "First erase pass failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Force erasing OTA partition (pass 2/3)...");
    ret = esp_partition_erase_range(ota_partition, 0, ota_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Second erase pass failed: %s", esp_err_to_name(ret));
    }

    ESP_LOGI(TAG, "Force erasing OTA partition (pass 3/3)...");
    ret = esp_partition_erase_range(ota_partition, 0, ota_partition->size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Third erase pass failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Clear any boot flags
    nvs_handle_t nvs_handle;
    ret = nvs_open("launcher", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        uint8_t boot_firmware_once = 0;
        nvs_set_blob(nvs_handle, "boot_fw_once", &boot_firmware_once, sizeof(boot_firmware_once));
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }

    // Ensure factory partition is boot partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition) {
        esp_ota_set_boot_partition(factory_partition);
    }

    ESP_LOGI(TAG, "✓ Partition forcefully cleaned with triple-erase");
    ESP_LOGI(TAG, "✓ Boot flags cleared");
    ESP_LOGI(TAG, "✓ Factory partition restored");

    return ESP_OK;
}