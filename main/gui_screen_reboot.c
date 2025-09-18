#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "esp_log.h"

static const char *TAG = "GUI_REBOOT";

lv_obj_t *reboot_dialog_screen = NULL;

void create_reboot_dialog_screen(void) {
    reboot_dialog_screen = lv_obj_create(NULL);
    lv_obj_add_style(reboot_dialog_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create a container for the left half of the screen
    lv_obj_t *left_container = lv_obj_create(reboot_dialog_screen);
    lv_obj_set_size(left_container, lv_pct(50), lv_pct(100));
    lv_obj_align(left_container, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_opa(left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(left_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(left_container, 10, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(left_container);
    lv_label_set_text(title, "Firmware Ready!");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // Main message
    lv_obj_t *msg = lv_label_create(left_container);
    lv_label_set_text(msg, "Firmware has been configured to boot once.\n\n"
                           "To run the firmware:\n"
                           "1. Press and hold the POWER button\n"
                           "2. Wait for the device to power off\n"
                           "3. Press the POWER button again to boot\n\n"
                           "The firmware will run once, then automatically\n"
                           "return to this launcher.");
    apply_text_style(msg);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -20);
    
    // Power button icon and instruction
    lv_obj_t *power_icon = lv_label_create(left_container);
    lv_label_set_text(power_icon, LV_SYMBOL_POWER " Hold POWER button to reboot");
    lv_obj_set_style_text_font(power_icon, THEME_FONT_NORMAL, 0);
    lv_obj_set_style_text_color(power_icon, lv_color_hex(0xff6600), 0);
    lv_obj_align(power_icon, LV_ALIGN_BOTTOM_MID, 0, -90);
    
    // Back to launcher button
    lv_obj_t *back_btn = lv_button_create(left_container);
    lv_obj_set_size(back_btn, lv_pct(80), 60);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_HOME " Back to Launcher");
    lv_obj_center(back_label);
}

void destroy_reboot_dialog_screen(void) {
    if (reboot_dialog_screen) {
        lv_obj_del(reboot_dialog_screen);
        reboot_dialog_screen = NULL;
        ESP_LOGI(TAG, "Reboot dialog screen destroyed");
    }
}