#include "gui_screens.h"
#include "gui_events.h"
#include "sd_manager.h"
#include "firmware_loader.h"
#include "esp_log.h"

static const char *TAG = "GUI_MAIN";

lv_obj_t *main_screen = NULL;

void create_main_screen(void) {
    main_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(main_screen);
    lv_label_set_text(title, "Simplified Launcher");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // SD Card status
    lv_obj_t *sd_status = lv_label_create(main_screen);
    if (sd_manager_is_mounted()) {
        lv_label_set_text(sd_status, LV_SYMBOL_SD_CARD " SD Card: Mounted");
        lv_obj_set_style_text_color(sd_status, lv_color_hex(0x00ff00), 0);
    } else {
        lv_label_set_text(sd_status, LV_SYMBOL_SD_CARD " SD Card: Not Found");
        lv_obj_set_style_text_color(sd_status, lv_color_hex(0xff0000), 0);
    }
    lv_obj_align(sd_status, LV_ALIGN_TOP_MID, 0, 60);
    
    // File Manager button
    lv_obj_t *file_mgr_btn = lv_button_create(main_screen);
    lv_obj_set_size(file_mgr_btn, 200, 60);
    lv_obj_align(file_mgr_btn, LV_ALIGN_CENTER, 0, -60);
    lv_obj_add_event_cb(file_mgr_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *file_mgr_label = lv_label_create(file_mgr_btn);
    lv_label_set_text(file_mgr_label, LV_SYMBOL_DIRECTORY " File Manager");
    lv_obj_center(file_mgr_label);
    
    // Firmware Loader button
    lv_obj_t *fw_loader_btn = lv_button_create(main_screen);
    lv_obj_set_size(fw_loader_btn, 200, 60);
    lv_obj_align(fw_loader_btn, LV_ALIGN_CENTER, 0, 10);
    lv_obj_add_event_cb(fw_loader_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *fw_loader_label = lv_label_create(fw_loader_btn);
    lv_label_set_text(fw_loader_label, LV_SYMBOL_DOWNLOAD " Firmware Loader");
    lv_obj_center(fw_loader_label);
    
    // Run Firmware button
    lv_obj_t *run_fw_btn = lv_button_create(main_screen);
    lv_obj_set_size(run_fw_btn, 200, 60);
    lv_obj_align(run_fw_btn, LV_ALIGN_CENTER, 0, 80);
    lv_obj_add_event_cb(run_fw_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)2);
    
    lv_obj_t *run_fw_label = lv_label_create(run_fw_btn);
    
    // Check if firmware is available
    if (firmware_loader_is_firmware_ready()) {
        lv_label_set_text(run_fw_label, LV_SYMBOL_PLAY " Run Firmware");
        lv_obj_set_style_text_color(run_fw_label, lv_color_hex(0x00ff00), 0);
    } else {
        lv_label_set_text(run_fw_label, LV_SYMBOL_CLOSE " No Firmware");
        lv_obj_set_style_text_color(run_fw_label, lv_color_hex(0xff0000), 0);
        lv_obj_add_state(run_fw_btn, LV_STATE_DISABLED);
    }
    lv_obj_center(run_fw_label);
}

void update_main_screen(void) {
    if (main_screen) {
        lv_obj_clean(main_screen);
        create_main_screen();
    }
}