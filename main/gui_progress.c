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

// Progress update timer
static lv_timer_t *progress_timer = NULL;

// Thread-safe progress data structure
typedef struct {
    size_t bytes_written;
    size_t total_bytes;
    char step_description[128];
    bool update_pending;
} progress_data_t;

static progress_data_t progress_data = {0};
static portMUX_TYPE progress_mutex = portMUX_INITIALIZER_UNLOCKED;

// Timer callback for safe UI updates
static void progress_timer_cb(lv_timer_t *timer) {
    if (!progress_bar || !progress_label || !progress_step_label) {
        return;
    }
    
    // Safely copy progress data
    progress_data_t local_data;
    portENTER_CRITICAL(&progress_mutex);
    if (!progress_data.update_pending) {
        portEXIT_CRITICAL(&progress_mutex);
        return;
    }
    local_data = progress_data;
    progress_data.update_pending = false;
    portEXIT_CRITICAL(&progress_mutex);
    
    // Update progress bar without animation to avoid conflicts
    int32_t progress_percent = (local_data.total_bytes > 0) ? 
        (local_data.bytes_written * 100 / local_data.total_bytes) : 0;
    lv_bar_set_value(progress_bar, progress_percent, LV_ANIM_OFF);
    
    // Update progress text
    char progress_text[128];
    if (local_data.total_bytes > 0) {
        snprintf(progress_text, sizeof(progress_text), "%zu / %zu bytes (%ld%%)", 
                local_data.bytes_written, local_data.total_bytes, progress_percent);
    } else {
        snprintf(progress_text, sizeof(progress_text), "%s", local_data.step_description);
    }
    lv_label_set_text(progress_label, progress_text);
    
    // Update step description
    lv_label_set_text(progress_step_label, local_data.step_description);
}

void gui_progress_init(void) {
    progress_update_pending = false;
    pending_bytes_written = 0;
    pending_total_bytes = 0;
    memset(pending_step_description, 0, sizeof(pending_step_description));
    flashing_in_progress = false;
    should_show_splash = false;
    should_show_main = false;
    
    // Initialize progress data
    portENTER_CRITICAL(&progress_mutex);
    memset(&progress_data, 0, sizeof(progress_data));
    portEXIT_CRITICAL(&progress_mutex);
    
    // Create timer for progress updates (200ms interval)
    if (progress_timer == NULL) {
        progress_timer = lv_timer_create(progress_timer_cb, 200, NULL);
        lv_timer_pause(progress_timer);
    }
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
        
        // Stop progress timer when leaving progress screen
        if (progress_timer) {
            lv_timer_pause(progress_timer);
        }
        return;
    }
    
    // Start progress timer when flashing begins
    if (flashing_in_progress && progress_timer) {
        lv_timer_resume(progress_timer);
    }
}

void firmware_progress_callback(size_t bytes_written, size_t total_bytes, const char *step_description) {
    // Thread-safe update of progress data
    portENTER_CRITICAL(&progress_mutex);
    progress_data.bytes_written = bytes_written;
    progress_data.total_bytes = total_bytes;
    if (step_description) {
        strncpy(progress_data.step_description, step_description, sizeof(progress_data.step_description) - 1);
        progress_data.step_description[sizeof(progress_data.step_description) - 1] = '\0';
    }
    progress_data.update_pending = true;
    portEXIT_CRITICAL(&progress_mutex);
    
    // Also update the legacy variables for compatibility
    pending_bytes_written = bytes_written;
    pending_total_bytes = total_bytes;
    if (step_description) {
        strncpy(pending_step_description, step_description, sizeof(pending_step_description) - 1);
        pending_step_description[sizeof(pending_step_description) - 1] = '\0';
    }
    progress_update_pending = true;
}

void flash_firmware_task(void *pvParameters) {
    char *firmware_path = (char *)pvParameters;
    
    ESP_LOGI(TAG, "Starting firmware flash task for: %s", firmware_path);
    
    esp_err_t ret = firmware_loader_flash_from_sd_with_progress(firmware_path, firmware_progress_callback);
    
    if (ret == ESP_OK) {
        // Flash successful - show completion message and return to main screen
        ESP_LOGI(TAG, "Firmware flash completed successfully");
        firmware_progress_callback(100, 100, "Flash complete! Returning to launcher...");
        vTaskDelay(pdMS_TO_TICKS(3000)); // Show completion message for 3 seconds
        should_show_main = true;  // Return to main screen
    } else {
        ESP_LOGE(TAG, "Firmware flash failed with error: %s", esp_err_to_name(ret));
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
    
    // Pause timer when flashing stops
    if (!state && progress_timer) {
        lv_timer_pause(progress_timer);
    }
}