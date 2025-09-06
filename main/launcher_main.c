#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "hal.h"
#include "sd_manager.h"
#include "config_manager.h"
#include "gui_manager.h"
#include "gui_state.h"
#include "firmware_loader.h"
#include "gui_screens.h"
#include "power_monitor.h"

static const char *TAG = "LAUNCHER";
static uint32_t boot_timer_start = 0;
static const uint32_t BOOT_SCREEN_TIMEOUT_MS = 5000; // 5 seconds
static uint32_t last_power_update = 0;
static const uint32_t POWER_UPDATE_INTERVAL_MS = 1000; // Update every 1 second

void app_main(void) {
    ESP_LOGI(TAG, "Starting Simplified Launcher");
    
    // CRITICAL FIX: Ensure launcher (factory) is always the default boot partition
    // This prevents firmware from permanently taking over the boot process
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    
    if (factory_partition && running_partition) {
        ESP_LOGI(TAG, "Currently running from: %s", running_partition->label);
        
        // Always ensure factory is the default boot partition
        // This is safe to call even if factory is already the default
        esp_err_t ret = esp_ota_set_boot_partition(factory_partition);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Factory partition confirmed as default boot partition");
        } else {
            ESP_LOGW(TAG, "Failed to set factory as default boot partition: %s", esp_err_to_name(ret));
        }
        
        // If we're running from OTA partition, it means we just booted firmware
        // The rollback mechanism should handle returning to factory on next boot
        if (running_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_0) {
            ESP_LOGI(TAG, "Running from firmware partition - this is a one-time boot");
            ESP_LOGI(TAG, "System will return to launcher on next restart");
        }
    }
    
    // Initialize hardware
    ESP_LOGI(TAG, "Initializing hardware...");
    hal_init();
    hal_touchpad_init();
    
    // Initialize configuration manager (SPIFFS)
    ESP_LOGI(TAG, "Initializing configuration manager...");
    if (config_manager_init() != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize configuration manager - using defaults");
    }
    
    // Initialize SD card (can be configured via config manager)
    ESP_LOGI(TAG, "Initializing SD card...");
    launcher_config_t *config = config_manager_get_current();
    if (config->system.auto_mount_sd) {
        if (sd_manager_init() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SD card");
        }
    } else {
        ESP_LOGI(TAG, "SD card auto-mount disabled in configuration");
    }
    
    // Initialize power monitor
    ESP_LOGI(TAG, "Initializing power monitor...");
    if (power_monitor_init()) {
        ESP_LOGI(TAG, "Power monitor initialized successfully");
    } else {
        ESP_LOGW(TAG, "Failed to initialize power monitor - status bar will show placeholder values");
    }
    
    // Initialize firmware loader and boot manager
    ESP_LOGI(TAG, "Initializing firmware loader...");
    firmware_loader_init();
    
    ESP_LOGI(TAG, "Initializing boot manager...");
    esp_err_t ret = firmware_loader_init_boot_manager();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize boot manager: %s", esp_err_to_name(ret));
    }
    
    // Initialize GUI
    ESP_LOGI(TAG, "Initializing GUI...");
    gui_manager_init((lv_display_t*)lvDisp);
    
    ESP_LOGI(TAG, "Launcher initialized successfully");
    
    // Unlock display
    bsp_display_unlock();
    
    // Check if firmware is available and show appropriate screen
    if (firmware_loader_is_firmware_ready()) {
        ESP_LOGI(TAG, "Firmware detected, showing boot screen for %d seconds", (int)(BOOT_SCREEN_TIMEOUT_MS / 1000));
        boot_screen_active = true;
        boot_timer_start = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Show splash screen with boot option
        lv_screen_load(splash_screen);
    } else {
        ESP_LOGI(TAG, "No firmware detected, going directly to launcher");
        lv_screen_load(main_screen);
    }
    
    // Main loop
    while (1) {
        // Handle boot screen timeout
        if (boot_screen_active) {
            uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
            if (current_time - boot_timer_start >= BOOT_SCREEN_TIMEOUT_MS) {
                ESP_LOGI(TAG, "Boot screen timeout, auto-booting firmware");
                boot_screen_active = false;
                firmware_loader_boot_firmware_once();
            }
        }
        
        // Handle return to main screen after flash completion
        if (should_show_main) {
            should_show_main = false;
            update_main_screen(); // Refresh the main screen to show updated firmware status
            lv_screen_load(main_screen);
        }
        
        // Update power readings periodically
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (current_time - last_power_update >= POWER_UPDATE_INTERVAL_MS) {
            last_power_update = current_time;
            
            float voltage = power_monitor_get_voltage();
            float current_ma = power_monitor_get_current_ma();
            bool charging = power_monitor_is_charging();
            
            ESP_LOGI(TAG, "Power readings: %.2fV, %.1fmA, charging: %s", voltage, current_ma, charging ? "yes" : "no");
            
            // Only update if we're on the main screen
            lv_obj_t *active_screen = lv_screen_active();
            if (active_screen == main_screen) {
                update_status_bar(voltage, current_ma, charging);
            }
        }
        
        gui_manager_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}