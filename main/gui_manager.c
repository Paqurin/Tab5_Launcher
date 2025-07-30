#include "gui_manager.h"
#include "gui_screens.h"
#include "gui_progress.h"
#include "gui_state.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"

static const char *TAG = "GUI_MANAGER";

esp_err_t gui_manager_init(lv_display_t *disp) {
    ESP_LOGI(TAG, "Initializing GUI Manager");
    
    // Always reset boot partition to factory (launcher) on startup
    // This ensures that after running user firmware, we always boot back to launcher
    const esp_partition_t *factory_partition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (factory_partition) {
        esp_err_t ret = esp_ota_set_boot_partition(factory_partition);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Boot partition reset to launcher (factory)");
        } else {
            ESP_LOGW(TAG, "Failed to reset boot partition to factory: %s", esp_err_to_name(ret));
        }
    }
    
    // Initialize progress handling
    gui_progress_init();
    
    // Create all screens
    gui_screens_init();
    
    // Always start with the main launcher screen
    ESP_LOGI(TAG, "Starting with main launcher screen");
    lv_screen_load(main_screen);
    
    return ESP_OK;
}

void gui_manager_update(void) {
    // Handle screen transitions and progress state
    update_progress_ui();
    
    // Process LVGL tasks
    lv_timer_handler();
}