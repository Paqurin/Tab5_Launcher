#include "gui_progress.h"
#include "gui_screens.h"
#include "gui_state.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "GUI_PROGRESS";

// Flag to signal screen change from task
static volatile bool should_show_splash = false;
static volatile bool should_show_main = false;

void gui_progress_init(void) {
    progress_update_pending = false;
    pending_bytes_written = 0;
    pending_total_bytes = 0;
    memset(pending_step_description, 0, sizeof(pending_step_description));
    flashing_in_progress = false;
    should_show_splash = false;
    should_show_main = false;
}

void update_progress_ui(void) {
    // Handle screen transitions first
    if (should_show_splash) {
        should_show_splash = false;
        lv_screen_load(splash_screen);
        return;
    }
    
    if (should_show_main) {
        should_show_main = false;
        lv_screen_load(main_screen);
        return;
    }
    
    if (!progress_update_pending || !progress_bar || !progress_label || !progress_step_label) {
        return;
    }
    
    // Throttle UI updates to avoid overwhelming LVGL
    static uint32_t last_update_time = 0;
    uint32_t current_time = lv_tick_get();
    if (current_time - last_update_time < 200) { // Minimum 200ms between updates
        return;
    }
    last_update_time = current_time;
    
    // Update progress bar without animation to avoid conflicts
    int32_t progress_percent = (pending_total_bytes > 0) ? (pending_bytes_written * 100 / pending_total_bytes) : 0;
    lv_bar_set_value(progress_bar, progress_percent, LV_ANIM_OFF);
    
    // Update progress text
    char progress_text[128];
    if (pending_total_bytes > 0) {
        snprintf(progress_text, sizeof(progress_text), "%zu / %zu bytes (%ld%%)", 
                pending_bytes_written, pending_total_bytes, progress_percent);
    } else {
        snprintf(progress_text, sizeof(progress_text), "%s", pending_step_description);
    }
    lv_label_set_text(progress_label, progress_text);
    
    // Update step description
    lv_label_set_text(progress_step_label, pending_step_description);
    
    progress_update_pending = false;
}

void firmware_progress_callback(size_t bytes_written, size_t total_bytes, const char *step_description) {
    // Store the progress data in volatile variables
    // This function may be called from a different task context, so we just store data
    // and let the main GUI task handle the actual UI updates
    pending_bytes_written = bytes_written;
    pending_total_bytes = total_bytes;
    if (step_description) {
        strncpy(pending_step_description, step_description, sizeof(pending_step_description) - 1);
        pending_step_description[sizeof(pending_step_description) - 1] = '\0';
    }
    progress_update_pending = true;
    
    // Remove the delay - let the main task handle timing
    // vTaskDelay(pdMS_TO_TICKS(100));
}

void flash_firmware_task(void *pvParameters) {
    char *firmware_path = (char *)pvParameters;
    
    esp_err_t ret = firmware_loader_flash_from_sd_with_progress(firmware_path, firmware_progress_callback);
    
    if (ret == ESP_OK) {
        // Flash successful - the firmware_loader will handle automatic reboot
        firmware_progress_callback(100, 100, "Flash successful! Rebooting...");
        // Note: esp_restart() will be called by firmware_loader, so we won't reach here
    } else {
        firmware_progress_callback(0, 100, "Flash failed!");
        vTaskDelay(pdMS_TO_TICKS(3000));
        should_show_main = true;  // Go back to main screen on failure
    }
    
    set_flashing_state(false);
    free(firmware_path);
    vTaskDelete(NULL);
}

bool is_flashing_in_progress(void) {
    return flashing_in_progress;
}

void set_flashing_state(bool state) {
    flashing_in_progress = state;
}