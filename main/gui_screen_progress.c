#include "gui_screens.h"
#include "gui_styles.h"
#include "esp_log.h"

static const char *TAG = "GUI_PROGRESS";

lv_obj_t *progress_screen = NULL;
lv_obj_t *progress_bar = NULL;
lv_obj_t *progress_label = NULL;
lv_obj_t *progress_step_label = NULL;

void create_progress_screen(void) {
    progress_screen = lv_obj_create(NULL);
    lv_obj_add_style(progress_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Create a centered container to match other screens
    lv_obj_t *center_container = lv_obj_create(progress_screen);
    lv_obj_set_size(center_container, lv_pct(80), lv_pct(85));
    lv_obj_align(center_container, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(center_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(center_container, 10, 0);
    
    // Title
    lv_obj_t *title = lv_label_create(center_container);
    lv_label_set_text(title, "Flashing Firmware");
    apply_title_style(title);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Progress bar
    progress_bar = lv_bar_create(center_container);
    lv_obj_set_size(progress_bar, lv_pct(90), 40);
    lv_obj_align(progress_bar, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(progress_bar, 0, 100);
    
    // Style the progress bar to match project theme
    lv_obj_set_style_bg_color(progress_bar, THEME_BG_COLOR, LV_PART_MAIN);
    lv_obj_set_style_border_color(progress_bar, lv_color_hex(0x4A90E2), LV_PART_MAIN);
    lv_obj_set_style_border_width(progress_bar, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x4A90E2), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_INDICATOR);
    
    // Progress label
    progress_label = lv_label_create(center_container);
    lv_label_set_text(progress_label, "0 / 0 bytes (0%)");
    apply_text_style(progress_label);
    lv_obj_align(progress_label, LV_ALIGN_CENTER, 0, 50);
    
    // Step description
    progress_step_label = lv_label_create(center_container);
    lv_label_set_text(progress_step_label, "Preparing...");
    lv_obj_set_style_text_color(progress_step_label, lv_color_hex(0x4A90E2), 0);
    lv_obj_set_style_text_font(progress_step_label, THEME_FONT_NORMAL, 0);
    lv_obj_align(progress_step_label, LV_ALIGN_CENTER, 0, -50);
}

void destroy_progress_screen(void) {
    if (progress_screen) {
        ESP_LOGI(TAG, "Destroying progress screen...");

        // Disable events on screen to prevent corruption during deletion
        lv_obj_remove_event_cb(progress_screen, NULL);

        // Remove all event callbacks from child objects to prevent dangling pointers
        uint32_t child_cnt = lv_obj_get_child_count(progress_screen);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t *child = lv_obj_get_child(progress_screen, i);
            if (child) {
                lv_obj_remove_event_cb(child, NULL);
            }
        }

        // Now safely delete the screen
        lv_obj_del(progress_screen);
        progress_screen = NULL;
        ESP_LOGI(TAG, "Progress screen destroyed");
    }
}