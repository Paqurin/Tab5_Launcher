#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "esp_log.h"

static const char *TAG = "GUI_SPLASH";

lv_obj_t *splash_screen = NULL;

void create_splash_screen(void) {
    splash_screen = lv_obj_create(NULL);
    lv_obj_add_style(splash_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Title
    lv_obj_t *title = lv_label_create(splash_screen);
    lv_label_set_text(title, "Launcher");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // Message
    lv_obj_t *msg = lv_label_create(splash_screen);
    lv_label_set_text(msg, "A firmware is detected.\nThe device will boot to the firmware in 5 seconds if there is no operations made.");
    lv_obj_set_style_text_color(msg, THEME_SUCCESS_COLOR, 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(msg, THEME_FONT_MEDIUM, 0);
    lv_obj_align(msg, LV_ALIGN_TOP_MID, 0, 90);
    
    // Boot firmware button
    lv_obj_t *boot_btn = lv_button_create(splash_screen);
    lv_obj_set_size(boot_btn, 250, 70);
    lv_obj_align(boot_btn, LV_ALIGN_CENTER, 0, -30);
    apply_button_style(boot_btn);
    lv_obj_add_event_cb(boot_btn, splash_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *boot_label = lv_label_create(boot_btn);
    lv_label_set_text(boot_label, LV_SYMBOL_PLAY " Boot Firmware Now");
    lv_obj_center(boot_label);
    
    // Stay in launcher button
    lv_obj_t *stay_btn = lv_button_create(splash_screen);
    lv_obj_set_size(stay_btn, 250, 70);
    lv_obj_align(stay_btn, LV_ALIGN_CENTER, 0, 60);
    apply_button_style(stay_btn);
    lv_obj_add_event_cb(stay_btn, splash_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *stay_label = lv_label_create(stay_btn);
    lv_label_set_text(stay_label, LV_SYMBOL_SETTINGS " Enter Launcher");
    lv_obj_center(stay_label);
}