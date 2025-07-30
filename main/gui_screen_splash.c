#include "gui_screens.h"
#include "gui_events.h"
#include "esp_log.h"

static const char *TAG = "GUI_SPLASH";

lv_obj_t *splash_screen = NULL;

void create_splash_screen(void) {
    splash_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(splash_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(splash_screen);
    lv_label_set_text(title, "Launcher");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // Message
    lv_obj_t *msg = lv_label_create(splash_screen);
    lv_label_set_text(msg, "A firmware is detected.\nThe device will boot to the firmware in 5 seconds if there is no operations made.");
    lv_obj_set_style_text_color(msg, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(msg, &lv_font_montserrat_24, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 80);
    
    // Boot firmware button
    lv_obj_t *boot_btn = lv_button_create(splash_screen);
    lv_obj_set_size(boot_btn, 200, 60);
    lv_obj_align(boot_btn, LV_ALIGN_CENTER, 0, -30);
    lv_obj_add_event_cb(boot_btn, splash_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *boot_label = lv_label_create(boot_btn);
    lv_label_set_text(boot_label, LV_SYMBOL_PLAY " Boot Firmware Now");
    lv_obj_set_style_text_font(boot_label, &lv_font_montserrat_24, 0);
    lv_obj_center(boot_label);
    
    // Stay in launcher button
    lv_obj_t *stay_btn = lv_button_create(splash_screen);
    lv_obj_set_size(stay_btn, 200, 60);
    lv_obj_align(stay_btn, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(stay_btn, splash_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *stay_label = lv_label_create(stay_btn);
    lv_label_set_text(stay_label, LV_SYMBOL_SETTINGS " Enter Launcher");
    lv_obj_set_style_text_font(stay_label, &lv_font_montserrat_24, 0);
    lv_obj_center(stay_label);
}