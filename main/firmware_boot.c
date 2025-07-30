#include "firmware_loader.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_sleep.h"

static const char *TAG = "FIRMWARE_BOOT";
static const char *NVS_NAMESPACE = "launcher";
static const char *NVS_KEY_BOOT_FIRMWARE = "boot_fw_once";

esp_err_t firmware_loader_init_boot_manager(void) {
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
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    uint8_t boot_firmware_once = 0;
    size_t required_size = sizeof(boot_firmware_once);
    ret = nvs_get_blob(nvs_handle, NVS_KEY_BOOT_FIRMWARE, &boot_firmware_once, &required_size);
    
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    
    if (ret == ESP_OK && boot_firmware_once == 1) {
        ESP_LOGI(TAG, "One-time firmware boot completed, clearing flag");
        boot_firmware_once = 0;
        nvs_set_blob(nvs_handle, NVS_KEY_BOOT_FIRMWARE, &boot_firmware_once, sizeof(boot_firmware_once));
        nvs_commit(nvs_handle);
        
        if (factory_partition) {
            ret = esp_ota_set_boot_partition(factory_partition);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "Boot partition restored to launcher (factory)");
            }
        }
    }
    
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t firmware_loader_boot_firmware_once(void) {
    const esp_partition_t *ota_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (!ota_partition) {
        ESP_LOGE(TAG, "OTA_0 partition not found");
        return ESP_ERR_NOT_FOUND;
    }
    
    nvs_handle_t nvs_handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    uint8_t boot_firmware_once = 1;
    ret = nvs_set_blob(nvs_handle, NVS_KEY_BOOT_FIRMWARE, &boot_firmware_once, sizeof(boot_firmware_once));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot flag: %s", esp_err_to_name(ret));
        nvs_close(nvs_handle);
        return ret;
    }
    
    ret = nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_ota_set_boot_partition(ota_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set boot partition: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Restarting to boot firmware once...");
    esp_sleep_enable_timer_wakeup(50000); // 50,000us = 50ms
    esp_deep_sleep_start(); // Use deep sleep to force a full reboot (including the GT911 touch panel)
    return ESP_OK;
}

esp_err_t firmware_loader_restart_to_new_firmware(void) {
    return firmware_loader_boot_firmware_once();
}