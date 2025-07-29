#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "hal.h"
#include "sd_manager.h"
#include "gui_manager.h"
#include "gui_state.h"
#include "firmware_loader.h"
#include "gui_screens.h"

static const char *TAG = "LAUNCHER";
static uint32_t boot_timer_start = 0;
static const uint32_t BOOT_SCREEN_TIMEOUT_MS = 5000; // 5 seconds

void app_main(void) {
    ESP_LOGI(TAG, "Starting Simplified Launcher");
    
    // Ensure launcher is always the default boot partition
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition) {
        esp_err_t ret = esp_ota_set_boot_partition(factory_partition);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Boot partition confirmed as launcher (factory)");
        } else {
            ESP_LOGW(TAG, "Failed to set boot partition to factory: %s", esp_err_to_name(ret));
        }
    }
    
    // Initialize hardware
    ESP_LOGI(TAG, "Initializing hardware...");
    hal_init();
    hal_touchpad_init();
    
    // Initialize SD card
    ESP_LOGI(TAG, "Initializing SD card...");
    if (sd_manager_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SD card");
    }
    
    // Initialize firmware loader
    ESP_LOGI(TAG, "Initializing firmware loader...");
    firmware_loader_init();
    
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
                firmware_loader_restart_to_new_firmware();
            }
        }
        
        // Handle return to main screen after flash completion
        if (should_show_main) {
            should_show_main = false;
            update_main_screen(); // Refresh the main screen to show updated firmware status
            lv_screen_load(main_screen);
        }
        
        gui_manager_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}