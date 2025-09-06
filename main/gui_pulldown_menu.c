#include "gui_pulldown_menu.h"
#include "gui_styles.h"
#include "gui_screens.h"
#include "gui_screen_wifi_setup.h"
#include "sd_manager.h"
#include "esp_log.h"
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "GUI_PULLDOWN";

// Forward declarations
static void close_btn_event_handler(lv_event_t *e);
static void backdrop_event_handler(lv_event_t *e);
static void brightness_slider_event_handler(lv_event_t *e);
static void wifi_toggle_event_handler(lv_event_t *e);
static void wifi_back_btn_event_handler(lv_event_t *e);
static void sd_toggle_event_handler(lv_event_t *e);
static void slide_anim_cb(void *obj, int32_t value);

gui_pulldown_menu_t* gui_pulldown_menu_create(lv_obj_t *parent) {
    ESP_LOGI(TAG, "Creating pull-down menu");
    
    gui_pulldown_menu_t *menu = calloc(1, sizeof(gui_pulldown_menu_t));
    if (!menu) {
        ESP_LOGE(TAG, "Failed to allocate memory for pull-down menu");
        return NULL;
    }
    
    // Create semi-transparent backdrop (initially hidden)
    menu->backdrop = lv_obj_create(parent);
    lv_obj_set_size(menu->backdrop, lv_pct(100), lv_pct(100));
    lv_obj_align(menu->backdrop, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(menu->backdrop, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(menu->backdrop, LV_OPA_50, 0);
    lv_obj_add_flag(menu->backdrop, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(menu->backdrop, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(menu->backdrop, backdrop_event_handler, LV_EVENT_CLICKED, menu);
    
    // Create menu container (initially positioned above screen)
    menu->container = lv_obj_create(parent);
    lv_obj_set_size(menu->container, lv_pct(100), 250);
    lv_obj_set_pos(menu->container, 0, -250); // Start above screen
    lv_obj_set_style_bg_color(menu->container, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_opa(menu->container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(menu->container, 0, 0);
    lv_obj_set_style_pad_all(menu->container, 20, 0);
    lv_obj_add_flag(menu->container, LV_OBJ_FLAG_HIDDEN);
    
    // Title label
    lv_obj_t *title = lv_label_create(menu->container);
    lv_label_set_text(title, "Quick Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    
    // Close button
    menu->close_btn = lv_button_create(menu->container);
    lv_obj_set_size(menu->close_btn, 40, 40);
    lv_obj_align(menu->close_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(menu->close_btn, lv_color_hex(0xFF5722), 0);
    lv_obj_add_event_cb(menu->close_btn, close_btn_event_handler, LV_EVENT_CLICKED, menu);
    
    lv_obj_t *close_label = lv_label_create(menu->close_btn);
    lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
    lv_obj_center(close_label);
    
    // Brightness control
    lv_obj_t *brightness_container = lv_obj_create(menu->container);
    lv_obj_set_size(brightness_container, lv_pct(90), 60);
    lv_obj_align(brightness_container, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(brightness_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(brightness_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(brightness_container, 0, 0);
    
    menu->brightness_label = lv_label_create(brightness_container);
    lv_label_set_text(menu->brightness_label, LV_SYMBOL_EYE_OPEN " Brightness: 75%");
    lv_obj_align(menu->brightness_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_text_color(menu->brightness_label, lv_color_hex(0xFFFFFF), 0);
    
    menu->brightness_slider = lv_slider_create(brightness_container);
    lv_obj_set_size(menu->brightness_slider, lv_pct(100), 20);
    lv_obj_align(menu->brightness_slider, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(menu->brightness_slider, 10, 100);
    lv_slider_set_value(menu->brightness_slider, 75, LV_ANIM_OFF);
    lv_obj_add_event_cb(menu->brightness_slider, brightness_slider_event_handler, LV_EVENT_VALUE_CHANGED, menu);
    
    // WiFi toggle button
    menu->wifi_toggle = lv_button_create(menu->container);
    lv_obj_set_size(menu->wifi_toggle, lv_pct(90), 50);
    lv_obj_align(menu->wifi_toggle, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_set_style_bg_color(menu->wifi_toggle, lv_color_hex(0x4CAF50), 0);
    lv_obj_add_event_cb(menu->wifi_toggle, wifi_toggle_event_handler, LV_EVENT_CLICKED, menu);
    
    lv_obj_t *wifi_label = lv_label_create(menu->wifi_toggle);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI " WiFi Tools");
    lv_obj_center(wifi_label);
    
    // SD card toggle button
    menu->sd_toggle = lv_button_create(menu->container);
    lv_obj_set_size(menu->sd_toggle, lv_pct(90), 50);
    lv_obj_align(menu->sd_toggle, LV_ALIGN_TOP_MID, 0, 185);
    lv_obj_set_style_bg_color(menu->sd_toggle, sd_manager_is_mounted() ? lv_color_hex(0x4CAF50) : lv_color_hex(0x666666), 0);
    lv_obj_add_event_cb(menu->sd_toggle, sd_toggle_event_handler, LV_EVENT_CLICKED, menu);
    
    lv_obj_t *sd_label = lv_label_create(menu->sd_toggle);
    lv_label_set_text(sd_label, sd_manager_is_mounted() ? LV_SYMBOL_SD_CARD " Unmount SD" : LV_SYMBOL_SD_CARD " Mount SD");
    lv_obj_center(sd_label);
    
    menu->is_open = false;
    
    ESP_LOGI(TAG, "Pull-down menu created successfully");
    return menu;
}

void gui_pulldown_menu_destroy(gui_pulldown_menu_t *menu) {
    if (!menu) return;
    
    if (menu->container) {
        lv_obj_del(menu->container);
    }
    if (menu->backdrop) {
        lv_obj_del(menu->backdrop);
    }
    
    free(menu);
}

static void slide_anim_cb(void *obj, int32_t value) {
    lv_obj_set_y(obj, value);
}

void gui_pulldown_menu_show(gui_pulldown_menu_t *menu) {
    if (!menu || menu->is_open) return;
    
    ESP_LOGI(TAG, "Showing pull-down menu");
    
    // Show backdrop first
    lv_obj_remove_flag(menu->backdrop, LV_OBJ_FLAG_HIDDEN);
    
    // Show container
    lv_obj_remove_flag(menu->container, LV_OBJ_FLAG_HIDDEN);
    
    // Animate slide down
    lv_anim_init(&menu->slide_anim);
    lv_anim_set_var(&menu->slide_anim, menu->container);
    lv_anim_set_values(&menu->slide_anim, -250, 0);
    lv_anim_set_time(&menu->slide_anim, 300);
    lv_anim_set_exec_cb(&menu->slide_anim, slide_anim_cb);
    lv_anim_set_path_cb(&menu->slide_anim, lv_anim_path_ease_out);
    lv_anim_start(&menu->slide_anim);
    
    menu->is_open = true;
}

// Animation completion callback
static void hide_anim_complete_cb(lv_anim_t *a) {
    gui_pulldown_menu_t *menu = (gui_pulldown_menu_t*)lv_anim_get_user_data(a);
    if (menu) {
        lv_obj_add_flag(menu->container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(menu->backdrop, LV_OBJ_FLAG_HIDDEN);
    }
}

void gui_pulldown_menu_hide(gui_pulldown_menu_t *menu) {
    if (!menu || !menu->is_open) return;
    
    ESP_LOGI(TAG, "Hiding pull-down menu");
    
    // Animate slide up
    lv_anim_init(&menu->slide_anim);
    lv_anim_set_var(&menu->slide_anim, menu->container);
    lv_anim_set_values(&menu->slide_anim, 0, -250);
    lv_anim_set_time(&menu->slide_anim, 300);
    lv_anim_set_exec_cb(&menu->slide_anim, slide_anim_cb);
    lv_anim_set_path_cb(&menu->slide_anim, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&menu->slide_anim, hide_anim_complete_cb);
    lv_anim_set_user_data(&menu->slide_anim, menu);
    lv_anim_start(&menu->slide_anim);
    
    menu->is_open = false;
}

void gui_pulldown_menu_toggle(gui_pulldown_menu_t *menu) {
    if (!menu) return;
    
    if (menu->is_open) {
        gui_pulldown_menu_hide(menu);
    } else {
        gui_pulldown_menu_show(menu);
    }
}

bool gui_pulldown_menu_is_open(gui_pulldown_menu_t *menu) {
    return menu ? menu->is_open : false;
}

// Event handlers
static void close_btn_event_handler(lv_event_t *e) {
    gui_pulldown_menu_t *menu = (gui_pulldown_menu_t*)lv_event_get_user_data(e);
    gui_pulldown_menu_hide(menu);
}

static void backdrop_event_handler(lv_event_t *e) {
    gui_pulldown_menu_t *menu = (gui_pulldown_menu_t*)lv_event_get_user_data(e);
    gui_pulldown_menu_hide(menu);
}

static void brightness_slider_event_handler(lv_event_t *e) {
    gui_pulldown_menu_t *menu = (gui_pulldown_menu_t*)lv_event_get_user_data(e);
    int32_t value = lv_slider_get_value(menu->brightness_slider);
    
    char buf[32];
    snprintf(buf, sizeof(buf), LV_SYMBOL_EYE_OPEN " Brightness: %ld%%", value);
    lv_label_set_text(menu->brightness_label, buf);
    
    // TODO: Actually set display brightness
    ESP_LOGI(TAG, "Brightness set to %ld%%", value);
}

static void wifi_toggle_event_handler(lv_event_t *e) {
    gui_pulldown_menu_t *menu = (gui_pulldown_menu_t*)lv_event_get_user_data(e);
    
    ESP_LOGI(TAG, "WiFi Tools button clicked - loading enhanced WiFi setup screen");
    
    // Hide the pulldown menu first
    if (menu) {
        gui_pulldown_menu_hide(menu);
    }
    
    // Create the enhanced WiFi setup screen
    create_wifi_setup_screen();
    
    // Load the WiFi setup screen
    lv_screen_load(wifi_setup_screen);
    
    ESP_LOGI(TAG, "Enhanced WiFi setup screen loaded");
}

static void wifi_back_btn_event_handler(lv_event_t *e) {
    ESP_LOGI(TAG, "WiFi screen back button clicked");
    extern lv_obj_t *main_screen;
    if (main_screen) {
        lv_screen_load(main_screen);
    }
}

static void sd_toggle_event_handler(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    
    if (sd_manager_is_mounted()) {
        ESP_LOGI(TAG, "Unmounting SD card");
        esp_err_t ret = sd_manager_unmount();
        if (ret == ESP_OK) {
            lv_label_set_text(label, LV_SYMBOL_SD_CARD " Mount SD");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x666666), 0);
        }
    } else {
        ESP_LOGI(TAG, "Mounting SD card");
        esp_err_t ret = sd_manager_mount();
        if (ret == ESP_OK) {
            lv_label_set_text(label, LV_SYMBOL_SD_CARD " Unmount SD");
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x4CAF50), 0);
        }
    }
}