#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "esp_log.h"
#include "firmware_loader.h"

static const char *TAG = "GUI_SPLASH";

lv_obj_t *splash_screen = NULL;

// Event handler for the background tap
static void splash_background_event_handler(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        // Boot firmware when tapping anywhere on the background
        ESP_LOGI(TAG, "Background tapped - booting firmware");
        
        // Disable boot screen timeout
        extern bool boot_screen_active;
        boot_screen_active = false;
        
        esp_err_t ret = firmware_loader_boot_firmware_once();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure firmware boot: %s", esp_err_to_name(ret));
        }
        // Note: If successful, the device will reboot automatically
    }
}

void create_splash_screen(void) {
    splash_screen = lv_obj_create(NULL);
    lv_obj_add_style(splash_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Make the entire screen clickable for firmware boot
    lv_obj_add_event_cb(splash_screen, splash_background_event_handler, LV_EVENT_CLICKED, NULL);
    
    // Title
    lv_obj_t *title = lv_label_create(splash_screen);
    lv_label_set_text(title, "Launcher");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // Message
    lv_obj_t *msg = lv_label_create(splash_screen);
    lv_label_set_text(msg, "A firmware is detected.\n\n"
                           "Tap anywhere to boot firmware\n"
                           "or use the button below to enter launcher.");
    lv_obj_set_style_text_color(msg, THEME_SUCCESS_COLOR, 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(msg, THEME_FONT_MEDIUM, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -50);
    
    // Only one button at the bottom - Enter Launcher
    lv_obj_t *launcher_btn = lv_button_create(splash_screen);
    lv_obj_set_size(launcher_btn, 250, 70);
    lv_obj_align(launcher_btn, LV_ALIGN_BOTTOM_MID, 0, -30);
    apply_button_style(launcher_btn);
    lv_obj_add_event_cb(launcher_btn, splash_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    // Prevent the button from propagating click events to the background
    lv_obj_add_flag(launcher_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_remove_flag(launcher_btn, LV_OBJ_FLAG_EVENT_BUBBLE);
    
    lv_obj_t *launcher_label = lv_label_create(launcher_btn);
    lv_label_set_text(launcher_label, LV_SYMBOL_SETTINGS " Enter Launcher");
    lv_obj_center(launcher_label);
}

void destroy_splash_screen(void) {
    if (splash_screen) {
        lv_obj_del(splash_screen);
        splash_screen = NULL;
        ESP_LOGI(TAG, "Splash screen destroyed");
    }
}