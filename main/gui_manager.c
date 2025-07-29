#include "gui_manager.h"
#include "gui_screens.h"
#include "gui_progress.h"
#include "gui_state.h"
#include "firmware_loader.h"
#include "esp_log.h"

static const char *TAG = "GUI_MANAGER";

esp_err_t gui_manager_init(lv_display_t *disp) {
    ESP_LOGI(TAG, "Initializing GUI Manager");
    
    // Initialize progress handling
    gui_progress_init();
    
    // Create all screens
    gui_screens_init();
    
    // Check if firmware is ready on startup
    if (firmware_loader_is_firmware_ready()) {
        ESP_LOGI(TAG, "New firmware detected, showing splash screen");
        lv_screen_load(splash_screen);
    } else {
        ESP_LOGI(TAG, "No new firmware, showing main screen");
        lv_screen_load(main_screen);
    }
    
    return ESP_OK;
}

void gui_manager_update(void) {
    update_progress_ui();  // Handle pending progress updates and screen transitions
    lv_timer_handler();
}