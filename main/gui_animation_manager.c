#include "gui_animation_manager.h"
#include "config_manager.h"
#include "esp_log.h"

static const char *TAG = "GUI_ANIM_MGR";

// Temporarily disable animations for performance testing - can re-enable later
static bool animations_enabled = false; // true;

void gui_animation_manager_init(void) {
    ESP_LOGI(TAG, "Initializing global animation manager");

    // Initialize from current configuration
    gui_animation_manager_update_from_config();

    ESP_LOGI(TAG, "Global animation manager initialized - animations %s",
             animations_enabled ? "enabled" : "disabled");
}

void gui_animation_manager_set_enabled(bool enabled) {
    if (animations_enabled != enabled) {
        animations_enabled = enabled;
        ESP_LOGI(TAG, "Global animations %s", enabled ? "enabled" : "disabled");
    }
}

bool gui_animation_manager_is_enabled(void) {
    return animations_enabled;
}

uint32_t gui_animation_manager_get_time(uint32_t default_time) {
    return animations_enabled ? default_time : 0;
}

lv_anim_enable_t gui_animation_manager_get_anim_flag(void) {
    return animations_enabled ? LV_ANIM_ON : LV_ANIM_OFF;
}

void gui_animation_manager_update_from_config(void) {
    // Temporarily force animations OFF for performance testing
    // TODO: Re-enable config-based animations later
    /*
    launcher_config_t *config = config_manager_get_current();
    if (config) {
        gui_animation_manager_set_enabled(config->system.enable_animations);
    } else {
        ESP_LOGW(TAG, "No configuration available, using default animation settings");
        gui_animation_manager_set_enabled(true);
    }
    */
    ESP_LOGI(TAG, "Animations temporarily disabled for performance testing");
    gui_animation_manager_set_enabled(false);
}