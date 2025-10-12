#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "M5Unified.h"
#include "hal.h"

extern "C" {
#include "sd_manager.h"
#include "config_manager.h"
#include "gui_manager.h"
#include "gui_state.h"
#include "firmware_loader.h"
#include "gui_screens.h"
#include "power_monitor.h"
#include "gui_status_bar.h"
#include "wifi_manager.h"
#include "esp_hosted_sdio_config.h"
}
#include "hardware_control.h"  // This already has extern "C" guards

#ifdef CONFIG_IDF_TARGET_ESP32P4
// ESP-Hosted WiFi Remote initialization for ESP32-P4
extern "C" {
#include "esp_wifi_remote.h"
}
#endif


static const char *TAG = "LAUNCHER";
static uint32_t boot_timer_start = 0;
static const uint32_t BOOT_SCREEN_TIMEOUT_MS = 5000; // 5 seconds
static uint32_t last_power_update = 0;
static const uint32_t POWER_UPDATE_INTERVAL_MS = 5000; // Update every 5 seconds (reduced to minimize I2C bus activity and charging interference)
// M5Unified RTC helper functions
static void sync_system_time_from_rtc(void) {
    m5::rtc_datetime_t rtc_time;
    if (M5.Rtc.getDateTime(&rtc_time)) {
        struct tm timeinfo = {
            .tm_sec = rtc_time.time.seconds,
            .tm_min = rtc_time.time.minutes,
            .tm_hour = rtc_time.time.hours,
            .tm_mday = rtc_time.date.date,
            .tm_mon = rtc_time.date.month - 1,  // tm_mon is 0-11
            .tm_year = rtc_time.date.year - 1900,  // tm_year is years since 1900
            .tm_wday = 0
        };

        time_t rtc_timestamp = mktime(&timeinfo);
        struct timeval tv = { .tv_sec = rtc_timestamp, .tv_usec = 0 };
        settimeofday(&tv, NULL);

        ESP_LOGI(TAG, "System time synced from RTC: %04d-%02d-%02d %02d:%02d:%02d",
                 rtc_time.date.year, rtc_time.date.month, rtc_time.date.date,
                 rtc_time.time.hours, rtc_time.time.minutes, rtc_time.time.seconds);
    } else {
        ESP_LOGW(TAG, "Failed to read time from RTC");
    }
}

static void sync_rtc_from_system_time(void) {
    time_t current_time;
    time(&current_time);
    struct tm *timeinfo = localtime(&current_time);

    m5::rtc_datetime_t rtc_time;
    rtc_time.date.year = timeinfo->tm_year + 1900;
    rtc_time.date.month = timeinfo->tm_mon + 1;  // tm_mon is 0-11, RTC expects 1-12
    rtc_time.date.date = timeinfo->tm_mday;
    // Note: rtc_date_t doesn't have weekday field
    rtc_time.time.hours = timeinfo->tm_hour;
    rtc_time.time.minutes = timeinfo->tm_min;
    rtc_time.time.seconds = timeinfo->tm_sec;

    M5.Rtc.setDateTime(rtc_time);
    ESP_LOGI(TAG, "RTC time updated from system time");
}

extern "C" void app_main(void) {
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
    
    // Initialize M5Unified (includes RTC initialization)
    ESP_LOGI(TAG, "Initializing M5Unified...");
    M5.begin();

    // Initialize hardware
    ESP_LOGI(TAG, "Initializing hardware...");
    hal_init();
    hal_touchpad_init();

    // Initialize configuration manager (SPIFFS) first - no I2C dependencies
    ESP_LOGI(TAG, "Initializing configuration manager...");
    if (config_manager_init() != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize configuration manager - using defaults");
    }

    // Add delay to allow M5Unified I2C initialization to stabilize
    ESP_LOGI(TAG, "Allowing M5Unified I2C to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(500));

    // Initialize hardware control system AFTER M5Unified I2C stabilizes
    ESP_LOGI(TAG, "Initializing hardware control...");
    if (hardware_control_init() != ESP_OK) {
        ESP_LOGW(TAG, "Failed to initialize hardware control - switches may not work properly");
    }

    // NOTE: WiFi initialization is now LAZY - it will initialize when the user
    // opens WiFi settings or attempts to connect. This speeds up boot time and
    // prevents unnecessary hardware initialization.
    ESP_LOGI(TAG, "WiFi will initialize on-demand when accessed from UI");

    // Initialize SD card (can be configured via config manager)
    // NOTE: SD card uses GPIOs 39-44 (SDIO), ESP32-C6 uses GPIOs 8-15 (separate SDIO interface)
    ESP_LOGI(TAG, "Initializing SD card...");
    launcher_config_t *config = config_manager_get_current();
    ESP_LOGI(TAG, "SD auto-mount setting: %s", config->system.auto_mount_sd ? "enabled" : "disabled");
    if (config->system.auto_mount_sd) {
        ESP_LOGI(TAG, "Attempting SD card auto-mount...");
        if (sd_manager_init() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SD card");
        } else {
            ESP_LOGI(TAG, "SD card initialized successfully");
        }
    } else {
        ESP_LOGI(TAG, "SD card auto-mount disabled in configuration");
    }
    
    // Add additional delay before power monitor to avoid I2C conflicts
    ESP_LOGI(TAG, "Allowing I2C bus to stabilize before power monitor...");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize power monitor (may fail due to I2C conflicts - that's OK)
    ESP_LOGI(TAG, "Initializing power monitor...");
    if (power_monitor_init()) {
        ESP_LOGI(TAG, "Power monitor initialized successfully");
    } else {
        ESP_LOGW(TAG, "Power monitor initialization failed - continuing with placeholder values");
        ESP_LOGW(TAG, "This is expected if I2C bus conflicts exist - system will continue normally");
    }
    
    // Initialize firmware loader and boot manager
    ESP_LOGI(TAG, "Initializing firmware loader...");
    firmware_loader_init();
    
    ESP_LOGI(TAG, "Initializing boot manager...");
    esp_err_t ret = firmware_loader_init_boot_manager();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize boot manager: %s", esp_err_to_name(ret));
    }
    
    // Add system stability check before GUI initialization
    ESP_LOGI(TAG, "Performing system stability check...");
    vTaskDelay(pdMS_TO_TICKS(200));

    // Check if we have basic system functionality before proceeding
    bool system_stable = true;
    if (lvDisp == NULL) {
        ESP_LOGE(TAG, "LVGL display not available, cannot initialize GUI");
        system_stable = false;
    }

    // Initialize GUI
    ESP_LOGI(TAG, "Initializing GUI...");
    if (system_stable) {
        ESP_LOGI(TAG, "System stable - initializing GUI with M5Unified LVGL display");
        gui_manager_init((lv_display_t*)lvDisp);
    } else {
        ESP_LOGE(TAG, "System not stable - cannot proceed with GUI initialization");
        // System will halt here rather than corrupt display
        while(1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    // Sync system time from hardware RTC
    ESP_LOGI(TAG, "Syncing time from hardware RTC...");
    sync_system_time_from_rtc();

    // Verify system time is reasonable (not epoch)
    time_t current_time;
    time(&current_time);
    if (current_time < 946684800) { // Less than year 2000
        ESP_LOGW(TAG, "RTC time appears invalid, setting default time");
        m5::rtc_datetime_t default_time;
        default_time.date.year = 2025;
        default_time.date.month = 1;
        default_time.date.date = 1;
        default_time.time.hours = 12;
        default_time.time.minutes = 0;
        default_time.time.seconds = 0;
        M5.Rtc.setDateTime(default_time);
        ESP_LOGI(TAG, "Default time set in RTC");
        sync_system_time_from_rtc();  // Sync system time from the newly set RTC
    }

    // Initialize global status bar with M5Unified display
    if (lvDisp != NULL) {
        ESP_LOGI(TAG, "Initializing global status bar with M5Unified display...");
        if (gui_status_bar_init_global(lv_layer_top()) != ESP_OK) {
            ESP_LOGW(TAG, "Failed to initialize global status bar");
        }
    } else {
        ESP_LOGE(TAG, "LVGL display not available, cannot initialize status bar");
    }

    ESP_LOGI(TAG, "Launcher initialized successfully");
    
    // Check if firmware is available and show appropriate screen
    if (firmware_loader_is_firmware_ready()) {
        ESP_LOGI(TAG, "Firmware detected, showing boot screen for %d seconds", (int)(BOOT_SCREEN_TIMEOUT_MS / 1000));
        boot_screen_active = true;
        boot_timer_start = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Show splash screen with boot option
        lv_screen_load_anim(splash_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    } else {
        ESP_LOGI(TAG, "No firmware detected, going directly to launcher");
        lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
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
            lv_screen_load_anim(main_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
        }
        
        // Update power readings and time periodically
        uint32_t current_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (current_time_ms - last_power_update >= POWER_UPDATE_INTERVAL_MS) {
            last_power_update = current_time_ms;

            // Use batched read to minimize I2C bus activity and reduce charging interference
            float voltage, current_ma;
            bool charging;
            power_monitor_get_all(&voltage, &current_ma, &charging);

            ESP_LOGI(TAG, "Power readings: %.2fV, %.1fmA, charging: %s", voltage, current_ma, charging ? "yes" : "no");

            // Check for SD card auto-mounting if enabled
            launcher_config_t *config = config_manager_get_current();
            if (config->system.auto_mount_sd && !sd_manager_is_mounted()) {
                // Try to mount SD card if it's detected but not mounted
                // This handles the case where user inserts card after boot
                ESP_LOGI(TAG, "Attempting SD card auto-mount...");
                if (sd_manager_mount() == ESP_OK) {
                    ESP_LOGI(TAG, "SD card auto-mounted successfully");
                } else {
                    ESP_LOGD(TAG, "SD card auto-mount failed - card may not be present");
                }
            }

            // Update global status bar (now used across all screens)
            gui_status_bar_t *global_bar = gui_status_bar_get_global();
            if (global_bar) {
                ESP_LOGD(TAG, "Updating status bar: power, SD card, and time");
                gui_status_bar_update_power(global_bar, voltage, current_ma, charging);
                gui_status_bar_update_sdcard(global_bar);
                gui_status_bar_update_time(global_bar);
                ESP_LOGD(TAG, "Status bar update complete");
            } else {
                ESP_LOGW(TAG, "Global status bar not available for updates");
            }
        }

        // No need for NVS time saving - RTC handles persistence

        gui_manager_update();
        vTaskDelay(pdMS_TO_TICKS(20)); // Increased from 10ms to 20ms for better performance
    }
}