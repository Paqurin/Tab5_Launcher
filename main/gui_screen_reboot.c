#include "gui_screens.h"
#include "gui_events.h"
#include "esp_log.h"

static const char *TAG = "GUI_REBOOT";

lv_obj_t *reboot_dialog_screen = NULL;

void create_reboot_dialog_screen(void) {
    reboot_dialog_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(reboot_dialog_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(reboot_dialog_screen);
    lv_label_set_text(title, "Firmware Ready!");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00ff00), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // Main message
    lv_obj_t *msg = lv_label_create(reboot_dialog_screen);
    lv_label_set_text(msg, "Firmware has been configured to boot once.\n\n"
                           "To run the firmware:\n"
                           "1. Press and hold the POWER button\n"
                           "2. Wait for the device to power off\n"
                           "3. Press the POWER button again to boot\n\n"
                           "The firmware will run once, then automatically\n"
                           "return to this launcher.");
    lv_obj_set_style_text_color(msg, lv_color_white(), 0);
    lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(msg, LV_ALIGN_CENTER, 0, -20);
    
    // Power button icon and instruction
    lv_obj_t *power_icon = lv_label_create(reboot_dialog_screen);
    lv_label_set_text(power_icon, LV_SYMBOL_POWER " Hold POWER button to reboot");
    lv_obj_set_style_text_font(power_icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(power_icon, lv_color_hex(0xff6600), 0);
    lv_obj_align(power_icon, LV_ALIGN_BOTTOM_MID, 0, -80);
    
    // Back to launcher button
    lv_obj_t *back_btn = lv_button_create(reboot_dialog_screen);
    lv_obj_set_size(back_btn, 200, 50);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(back_btn, back_button_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_HOME " Back to Launcher");
    lv_obj_center(back_label);
}