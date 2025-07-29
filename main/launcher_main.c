#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hal.h"
#include "sd_manager.h"
#include "gui_manager.h"
#include "firmware_loader.h"

static const char *TAG = "LAUNCHER";

void app_main(void) {
    ESP_LOGI(TAG, "Starting Simplified Launcher");
    
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
    
    // Main loop
    while (1) {
        gui_manager_update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}