#include "gui_status_bar.h"
#include "gui_styles.h"
#include "sd_manager.h"
// #include "wifi_manager.h"  // Temporarily disabled for flicker testing
#include "esp_log.h"
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "GUI_STATUS_BAR";

gui_status_bar_t* gui_status_bar_create(lv_obj_t *parent) {
    ESP_LOGI(TAG, "Creating persistent status bar");
    
    gui_status_bar_t *status_bar = malloc(sizeof(gui_status_bar_t));
    if (!status_bar) {
        ESP_LOGE(TAG, "Failed to allocate memory for status bar");
        return NULL;
    }
    
    // Create status bar container at top
    status_bar->container = lv_obj_create(parent);
    lv_obj_set_size(status_bar->container, lv_pct(100), 40);
    lv_obj_align(status_bar->container, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(status_bar->container, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_opa(status_bar->container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(status_bar->container, 5, 0);
    
    // Create container for right-aligned status info
    lv_obj_t *status_container = lv_obj_create(status_bar->container);
    lv_obj_set_size(status_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(status_container, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_opa(status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(status_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(status_container, 2, 0);
    lv_obj_set_flex_flow(status_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Create container for left-aligned WiFi status
    lv_obj_t *wifi_container = lv_obj_create(status_bar->container);
    lv_obj_set_size(wifi_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(wifi_container, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_opa(wifi_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(wifi_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(wifi_container, 2, 0);
    
    // WiFi status indicator (left side)
    status_bar->wifi_label = lv_label_create(wifi_container);
    lv_label_set_text(status_bar->wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(status_bar->wifi_label, lv_color_hex(0x666666), 0); // Gray when disconnected
    lv_obj_set_style_text_font(status_bar->wifi_label, &lv_font_montserrat_18, 0);
    
    // Left pipe symbol
    lv_obj_t *left_pipe = lv_label_create(status_container);
    lv_label_set_text(left_pipe, "|");
    lv_obj_set_style_text_color(left_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(left_pipe, &lv_font_montserrat_20, 0);
    
    // SD card symbol
    status_bar->sdcard_label = lv_label_create(status_container);
    lv_label_set_text(status_bar->sdcard_label, LV_SYMBOL_SD_CARD);
    lv_obj_set_style_text_color(status_bar->sdcard_label, lv_color_hex(0x666666), 0); // Gray when not detected
    lv_obj_set_style_text_font(status_bar->sdcard_label, &lv_font_montserrat_18, 0);
    
    // Middle pipe symbol
    lv_obj_t *middle_pipe = lv_label_create(status_container);
    lv_label_set_text(middle_pipe, "|");
    lv_obj_set_style_text_color(middle_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(middle_pipe, &lv_font_montserrat_20, 0);
    
    // Voltage number
    status_bar->voltage_label = lv_label_create(status_container);
    lv_label_set_text(status_bar->voltage_label, "8.23");
    lv_obj_set_style_text_color(status_bar->voltage_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar->voltage_label, &lv_font_montserrat_18, 0);
    
    // Voltage unit
    status_bar->voltage_unit_label = lv_label_create(status_container);
    lv_label_set_text(status_bar->voltage_unit_label, "V");
    lv_obj_set_style_text_color(status_bar->voltage_unit_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar->voltage_unit_label, &lv_font_montserrat_14, 0);
    
    // Right middle pipe symbol
    lv_obj_t *right_middle_pipe = lv_label_create(status_container);
    lv_label_set_text(right_middle_pipe, "|");
    lv_obj_set_style_text_color(right_middle_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(right_middle_pipe, &lv_font_montserrat_20, 0);
    
    // Current number
    status_bar->current_label = lv_label_create(status_container);
    lv_label_set_text(status_bar->current_label, "410");
    lv_obj_set_style_text_color(status_bar->current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar->current_label, &lv_font_montserrat_18, 0);
    
    // Current unit
    status_bar->current_unit_label = lv_label_create(status_container);
    lv_label_set_text(status_bar->current_unit_label, "mA");
    lv_obj_set_style_text_color(status_bar->current_unit_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar->current_unit_label, &lv_font_montserrat_14, 0);
    
    // Right pipe symbol
    lv_obj_t *right_pipe = lv_label_create(status_container);
    lv_label_set_text(right_pipe, "|");
    lv_obj_set_style_text_color(right_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(right_pipe, &lv_font_montserrat_20, 0);
    
    // Charging indicator
    status_bar->charging_label = lv_label_create(status_container);
    lv_label_set_text(status_bar->charging_label, LV_SYMBOL_BATTERY_EMPTY);
    lv_obj_set_style_text_color(status_bar->charging_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_bar->charging_label, &lv_font_montserrat_20, 0);
    
    // Final pipe symbol
    lv_obj_t *final_pipe = lv_label_create(status_container);
    lv_label_set_text(final_pipe, "|");
    lv_obj_set_style_text_color(final_pipe, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_font(final_pipe, &lv_font_montserrat_20, 0);
    
    ESP_LOGI(TAG, "Persistent status bar created successfully");
    return status_bar;
}

void gui_status_bar_destroy(gui_status_bar_t *status_bar) {
    if (status_bar) {
        ESP_LOGI(TAG, "Destroying status bar");
        if (status_bar->container) {
            lv_obj_del(status_bar->container);
        }
        free(status_bar);
    }
}

void gui_status_bar_update_power(gui_status_bar_t *status_bar, float voltage, float current_ma, bool charging) {
    if (!status_bar) return;
    
    static char voltage_str[16];
    static char current_str[16];
    
    if (status_bar->voltage_label) {
        sprintf(voltage_str, "%.2f", voltage);
        lv_label_set_text(status_bar->voltage_label, voltage_str);
    }
    
    if (status_bar->current_label) {
        sprintf(current_str, "%.0f", current_ma);
        lv_label_set_text(status_bar->current_label, current_str);
    }
    
    if (status_bar->charging_label) {
        if (charging) {
            lv_label_set_text(status_bar->charging_label, LV_SYMBOL_BATTERY_3 LV_SYMBOL_CHARGE);
            lv_obj_set_style_text_color(status_bar->charging_label, lv_color_hex(0x00FF00), 0);
        } else if (voltage > 7.5) {
            lv_label_set_text(status_bar->charging_label, LV_SYMBOL_BATTERY_FULL);
            lv_obj_set_style_text_color(status_bar->charging_label, lv_color_hex(0x00FF00), 0);
        } else if (voltage > 6.5) {
            lv_label_set_text(status_bar->charging_label, LV_SYMBOL_BATTERY_3);
            lv_obj_set_style_text_color(status_bar->charging_label, lv_color_hex(0xFFFFFF), 0);
        } else if (voltage > 5.5) {
            lv_label_set_text(status_bar->charging_label, LV_SYMBOL_BATTERY_2);
            lv_obj_set_style_text_color(status_bar->charging_label, lv_color_hex(0xFFFF00), 0);
        } else {
            lv_label_set_text(status_bar->charging_label, LV_SYMBOL_BATTERY_1);
            lv_obj_set_style_text_color(status_bar->charging_label, lv_color_hex(0xFF0000), 0);
        }
    }
}

void gui_status_bar_update_wifi(gui_status_bar_t *status_bar, bool connected, int8_t rssi) {
    if (!status_bar || !status_bar->wifi_label) return;
    
    if (connected) {
        // Show connected WiFi with signal strength indication
        if (rssi > -50) {
            lv_obj_set_style_text_color(status_bar->wifi_label, lv_color_hex(0x00FF00), 0); // Strong signal - green
        } else if (rssi > -70) {
            lv_obj_set_style_text_color(status_bar->wifi_label, lv_color_hex(0xFFFF00), 0); // Medium signal - yellow
        } else {
            lv_obj_set_style_text_color(status_bar->wifi_label, lv_color_hex(0xFF8800), 0); // Weak signal - orange
        }
    } else {
        // Disconnected - gray
        lv_obj_set_style_text_color(status_bar->wifi_label, lv_color_hex(0x666666), 0);
    }
}

void gui_status_bar_update_sdcard(gui_status_bar_t *status_bar) {
    if (!status_bar || !status_bar->sdcard_label) return;
    
    bool card_mounted = sd_manager_is_mounted();
    bool card_detected = sd_manager_card_detected();
    
    if (card_mounted) {
        // Card mounted - show green SD card symbol
        lv_obj_set_style_text_color(status_bar->sdcard_label, lv_color_hex(0x00FF00), 0);
    } else if (card_detected) {
        // Card detected but not mounted - show white SD card symbol
        lv_obj_set_style_text_color(status_bar->sdcard_label, lv_color_hex(0xFFFFFF), 0);
    } else {
        // No card detected - show gray SD card symbol
        lv_obj_set_style_text_color(status_bar->sdcard_label, lv_color_hex(0x666666), 0);
    }
}

void gui_status_bar_set_visible(gui_status_bar_t *status_bar, bool visible) {
    if (!status_bar || !status_bar->container) return;
    
    if (visible) {
        lv_obj_clear_flag(status_bar->container, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(status_bar->container, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t* gui_status_bar_get_container(gui_status_bar_t *status_bar) {
    return status_bar ? status_bar->container : NULL;
}