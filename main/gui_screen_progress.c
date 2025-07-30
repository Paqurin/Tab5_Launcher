#include "gui_screens.h"
#include "esp_log.h"

static const char *TAG = "GUI_PROGRESS";

lv_obj_t *progress_screen = NULL;
lv_obj_t *progress_bar = NULL;
lv_obj_t *progress_label = NULL;
lv_obj_t *progress_step_label = NULL;

void create_progress_screen(void) {
    progress_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(progress_screen, lv_color_hex(0x1e1e1e), 0);
    
    // Title
    lv_obj_t *title = lv_label_create(progress_screen);
    lv_label_set_text(title, "Flashing Firmware");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Progress bar
    progress_bar = lv_bar_create(progress_screen);
    lv_obj_set_size(progress_bar, lv_pct(80), 30);
    lv_obj_align(progress_bar, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_range(progress_bar, 0, 100);
    
    // Progress label
    progress_label = lv_label_create(progress_screen);
    lv_label_set_text(progress_label, "0 / 0 bytes (0%)");
    lv_obj_set_style_text_color(progress_label, lv_color_white(), 0);
    lv_obj_align(progress_label, LV_ALIGN_CENTER, 0, 40);
    
    // Step description
    progress_step_label = lv_label_create(progress_screen);
    lv_label_set_text(progress_step_label, "Preparing...");
    lv_obj_set_style_text_color(progress_step_label, lv_color_hex(0x00ff00), 0);
    lv_obj_align(progress_step_label, LV_ALIGN_CENTER, 0, -40);
}