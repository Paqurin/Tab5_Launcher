#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "sd_manager.h"
#include "firmware_loader.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "GUI_MAIN";

lv_obj_t *main_screen = NULL;

static lv_obj_t *status_bar_voltage = NULL;
static lv_obj_t *status_bar_voltage_unit = NULL;
static lv_obj_t *status_bar_current = NULL;
static lv_obj_t *status_bar_current_unit = NULL;
static lv_obj_t *status_bar_charging = NULL;
static lv_obj_t *status_bar_sdcard = NULL;

void create_main_screen(void) {
    main_screen = lv_obj_create(NULL);
    lv_obj_add_style(main_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create status bar at top
    lv_obj_t *status_bar = lv_obj_create(main_screen);
    lv_obj_set_size(status_bar, lv_pct(100), 40);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_opa(status_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(status_bar, 5, 0);
    
    // Create container for right-aligned status info
    lv_obj_t *status_container = lv_obj_create(status_bar);
    lv_obj_set_size(status_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(status_container, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_opa(status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(status_container, 2, 0);
    lv_obj_set_flex_flow(status_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Left pipe symbol (2 fonts larger = 20pt)
    lv_obj_t *left_pipe = lv_label_create(status_container);
    lv_label_set_text(left_pipe, "|");
    lv_obj_set_style_text_color(left_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(left_pipe, &lv_font_montserrat_20, 0);
    
    // SD card symbol (1 font larger = 18pt)
    status_bar_sdcard = lv_label_create(status_container);
    lv_label_set_text(status_bar_sdcard, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(status_bar_sdcard, lv_color_hex(0x666666), 0); // Gray when not detected
    lv_obj_set_style_text_font(status_bar_sdcard, &lv_font_montserrat_18, 0);
    
    // Middle pipe symbol (2 fonts larger = 20pt)
    lv_obj_t *middle_pipe = lv_label_create(status_container);
    lv_label_set_text(middle_pipe, "|");
    lv_obj_set_style_text_color(middle_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(middle_pipe, &lv_font_montserrat_20, 0);
    
    // Voltage number (1 font larger = 18pt)
    status_bar_voltage = lv_label_create(status_container);
    lv_label_set_text(status_bar_voltage, "8.23");
    lv_obj_set_style_text_color(status_bar_voltage, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar_voltage, &lv_font_montserrat_18, 0);
    
    // Voltage unit (1 font smaller = 14pt)
    status_bar_voltage_unit = lv_label_create(status_container);
    lv_label_set_text(status_bar_voltage_unit, "V");
    lv_obj_set_style_text_color(status_bar_voltage_unit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar_voltage_unit, &lv_font_montserrat_14, 0);
    
    // Right middle pipe symbol (2 fonts larger = 20pt)
    lv_obj_t *right_middle_pipe = lv_label_create(status_container);
    lv_label_set_text(right_middle_pipe, "|");
    lv_obj_set_style_text_color(right_middle_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(right_middle_pipe, &lv_font_montserrat_20, 0);
    
    // Current number (1 font larger = 18pt)
    status_bar_current = lv_label_create(status_container);
    lv_label_set_text(status_bar_current, "410");
    lv_obj_set_style_text_color(status_bar_current, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar_current, &lv_font_montserrat_18, 0);
    
    // Current unit (1 font smaller = 14pt)
    status_bar_current_unit = lv_label_create(status_container);
    lv_label_set_text(status_bar_current_unit, "mA");
    lv_obj_set_style_text_color(status_bar_current_unit, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar_current_unit, &lv_font_montserrat_14, 0);
    
    // Right pipe symbol (2 fonts larger = 20pt)
    lv_obj_t *right_pipe = lv_label_create(status_container);
    lv_label_set_text(right_pipe, "|");
    lv_obj_set_style_text_color(right_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(right_pipe, &lv_font_montserrat_20, 0);
    
    // Charging indicator (larger battery icon)
    status_bar_charging = lv_label_create(status_container);
    lv_label_set_text(status_bar_charging, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_set_style_text_color(status_bar_charging, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar_charging, &lv_font_montserrat_20, 0);
    
    // Final pipe symbol (2 fonts larger = 20pt)
    lv_obj_t *final_pipe = lv_label_create(status_container);
    lv_label_set_text(final_pipe, "|");
    lv_obj_set_style_text_color(final_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(final_pipe, &lv_font_montserrat_20, 0);
    
    // Create centered container for main controls
    lv_obj_t *center_container = lv_obj_create(main_screen);
    lv_obj_set_size(center_container, lv_pct(80), lv_pct(85));
    lv_obj_align(center_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(center_container, 10, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(center_container);
    lv_label_set_text(title, "Simplified Launcher");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // SD Card status
    lv_obj_t *sd_status = lv_label_create(center_container);
    if (sd_manager_is_mounted()) {
        lv_label_set_text(sd_status, LV_SYMBOL_SD_CARD " SD Card: Mounted");
        lv_obj_set_style_text_color(sd_status, THEME_SUCCESS_COLOR, 0);
    } else {
        lv_label_set_text(sd_status, LV_SYMBOL_SD_CARD " SD Card: Not Found");
        lv_obj_set_style_text_color(sd_status, THEME_ERROR_COLOR, 0);
    }
    lv_obj_set_style_text_font(sd_status, THEME_FONT_NORMAL, 0);
    lv_obj_align(sd_status, LV_ALIGN_TOP_MID, 0, 70);
    
    // File Manager button - Bigger size
    lv_obj_t *file_mgr_btn = lv_button_create(center_container);
    lv_obj_set_size(file_mgr_btn, lv_pct(95), 85);
    lv_obj_align(file_mgr_btn, LV_ALIGN_CENTER, 0, -80);
    apply_button_style(file_mgr_btn);
    lv_obj_add_event_cb(file_mgr_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)0);
    
    lv_obj_t *file_mgr_label = lv_label_create(file_mgr_btn);
    lv_label_set_text(file_mgr_label, LV_SYMBOL_DIRECTORY " File Manager");
    lv_obj_center(file_mgr_label);
    
    // Firmware Loader button - Bigger size
    lv_obj_t *fw_loader_btn = lv_button_create(center_container);
    lv_obj_set_size(fw_loader_btn, lv_pct(95), 85);
    lv_obj_align(fw_loader_btn, LV_ALIGN_CENTER, 0, 15);
    apply_button_style(fw_loader_btn);
    lv_obj_add_event_cb(fw_loader_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)1);
    
    lv_obj_t *fw_loader_label = lv_label_create(fw_loader_btn);
    lv_label_set_text(fw_loader_label, LV_SYMBOL_DOWNLOAD " Firmware Loader");
    lv_obj_center(fw_loader_label);
    
    // Run Firmware button - Bigger size
    lv_obj_t *run_fw_btn = lv_button_create(center_container);
    lv_obj_set_size(run_fw_btn, lv_pct(95), 85);
    lv_obj_align(run_fw_btn, LV_ALIGN_CENTER, 0, 110);
    apply_button_style(run_fw_btn);
    lv_obj_add_event_cb(run_fw_btn, main_menu_event_handler, LV_EVENT_CLICKED, (void*)(uintptr_t)2);
    
    lv_obj_t *run_fw_label = lv_label_create(run_fw_btn);
    
    // Check if firmware is available
    if (firmware_loader_is_firmware_ready()) {
        lv_label_set_text(run_fw_label, LV_SYMBOL_PLAY " Run Firmware");
        lv_obj_set_style_text_color(run_fw_label, THEME_SUCCESS_COLOR, 0);
    } else {
        lv_label_set_text(run_fw_label, LV_SYMBOL_CLOSE " No Firmware");
        lv_obj_set_style_text_color(run_fw_label, THEME_ERROR_COLOR, 0);
        lv_obj_add_state(run_fw_btn, LV_STATE_DISABLED);
    }
    lv_obj_center(run_fw_label);
}

void update_status_bar(float voltage, float current_ma, bool charging) {
    static char voltage_str[16];
    static char current_str[16];
    
    if (status_bar_voltage) {
        sprintf(voltage_str, "%.2f", voltage);
        lv_label_set_text(status_bar_voltage, voltage_str);
    }
    if (status_bar_current) {
        sprintf(current_str, "%.0f", current_ma);
        lv_label_set_text(status_bar_current, current_str);
    }
    if (status_bar_charging) {
        if (charging) {
            lv_label_set_text(status_bar_charging, LV_SYMBOL_BATTERY_3 LV_SYMBOL_CHARGE);
            lv_obj_set_style_text_color(status_bar_charging, lv_color_hex(0x00FF00), 0);
        } else if (voltage > 7.5) {
            lv_label_set_text(status_bar_charging, LV_SYMBOL_BATTERY_FULL);
            lv_obj_set_style_text_color(status_bar_charging, lv_color_hex(0x00FF00), 0);
        } else if (voltage > 6.5) {
            lv_label_set_text(status_bar_charging, LV_SYMBOL_BATTERY_3);
            lv_obj_set_style_text_color(status_bar_charging, lv_color_hex(0xFFFFFF), 0);
        } else if (voltage > 5.5) {
            lv_label_set_text(status_bar_charging, LV_SYMBOL_BATTERY_2);
            lv_obj_set_style_text_color(status_bar_charging, lv_color_hex(0xFFFF00), 0);
        } else {
            lv_label_set_text(status_bar_charging, LV_SYMBOL_BATTERY_1);
            lv_obj_set_style_text_color(status_bar_charging, lv_color_hex(0xFF0000), 0);
        }
    }
    
    // Update SD card status with detection logic
    if (status_bar_sdcard) {
        bool card_mounted = sd_manager_is_mounted();
        bool card_detected = sd_manager_card_detected();
        
        // Only try auto-mounting if no card is detected at all
        if (!card_detected && !card_mounted) {
            // Try to mount to see if a card was inserted
            esp_err_t mount_result = sd_manager_mount();
            if (mount_result == ESP_OK) {
                // Card was successfully mounted, update states
                card_detected = true;
                card_mounted = true;
            } else {
                // Still no card or card not readable
                sd_manager_set_card_present(false);
                card_detected = false;
            }
        }
        
        // Check for removal when card is detected but not mounted (white state)
        // Only check every 10 seconds to avoid system instability
        static uint32_t last_removal_check = 0;
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        if (card_detected && !card_mounted && (current_time - last_removal_check >= 10000)) {
            last_removal_check = current_time;
            
            // Try a very quick mount test to see if card is still physically present
            esp_err_t mount_result = sd_manager_mount();
            if (mount_result == ESP_OK) {
                // Card is still there, immediately unmount since user had it unmounted
                sd_manager_unmount();
                // Keep detected state as true
            } else {
                // Card was removed - set to not present
                sd_manager_set_card_present(false);
                card_detected = false;
            }
        }
        
        // Update display based on actual current states
        card_mounted = sd_manager_is_mounted();
        card_detected = sd_manager_card_detected();
        
        if (card_mounted) {
            // Card mounted - show green SD card symbol
            lv_obj_set_style_text_color(status_bar_sdcard, lv_color_hex(0x00FF00), 0);
        } else if (card_detected) {
            // Card detected but not mounted - show white SD card symbol
            lv_obj_set_style_text_color(status_bar_sdcard, lv_color_hex(0xFFFFFF), 0);
        } else {
            // No card detected - show gray SD card symbol
            lv_obj_set_style_text_color(status_bar_sdcard, lv_color_hex(0x666666), 0);
        }
    }
}

void update_main_screen(void) {
    if (main_screen) {
        lv_obj_clean(main_screen);
        create_main_screen();
    }
}