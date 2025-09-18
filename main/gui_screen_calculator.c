#include "gui_screen_calculator.h"
#include "gui_screens.h"
#include "gui_events.h"
#include "gui_styles.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static const char *TAG = "CALCULATOR";

lv_obj_t *calculator_screen = NULL;
static lv_obj_t *display_area = NULL;
static char current_display[32] = "0";
static double current_value = 0.0;
static double stored_value = 0.0;
static char operation = '\0';
static bool new_number = true;

// Forward declarations
static void calc_back_button_event_handler(lv_event_t *e);
static void number_button_event_handler(lv_event_t *e);
static void operation_button_event_handler(lv_event_t *e);
static void equals_button_event_handler(lv_event_t *e);
static void clear_button_event_handler(lv_event_t *e);
static void update_display(void);
static void perform_calculation(void);

void create_calculator_screen(void) {
    if (calculator_screen) {
        return; // Already created
    }

    calculator_screen = lv_obj_create(NULL);
    lv_obj_add_style(calculator_screen, &style_screen, LV_PART_MAIN | LV_STATE_DEFAULT);

    // Create title bar
    lv_obj_t *title_bar = lv_obj_create(calculator_screen);
    lv_obj_set_size(title_bar, lv_pct(100), 60);
    lv_obj_align(title_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_opa(title_bar, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(title_bar, 10, 0);

    // Back button
    lv_obj_t *back_btn = lv_button_create(title_bar);
    lv_obj_set_size(back_btn, 60, 40);
    lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, 0, 0);
    apply_button_style(back_btn);
    lv_obj_add_event_cb(back_btn, calc_back_button_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_center(back_label);

    // Title
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Calculator");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 0);

    // Create main container
    lv_obj_t *main_container = lv_obj_create(calculator_screen);
    lv_obj_set_size(main_container, lv_pct(90), lv_pct(85));
    lv_obj_align(main_container, LV_ALIGN_CENTER, 0, 30);
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(main_container, 20, 0);

    // Display area
    display_area = lv_textarea_create(main_container);
    lv_obj_set_size(display_area, lv_pct(100), 80);
    lv_obj_align(display_area, LV_ALIGN_TOP_MID, 0, 0);
    lv_textarea_set_text(display_area, current_display);
    lv_obj_set_style_bg_color(display_area, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_text_color(display_area, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(display_area, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_align(display_area, LV_TEXT_ALIGN_RIGHT, 0);
    lv_textarea_set_cursor_click_pos(display_area, false);
    lv_obj_add_state(display_area, LV_STATE_DISABLED); // Read-only

    // Button grid container
    lv_obj_t *button_grid = lv_obj_create(main_container);
    lv_obj_set_size(button_grid, lv_pct(100), lv_pct(75));
    lv_obj_align(button_grid, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(button_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(button_grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(button_grid, 5, 0);

    // Button layout: 4x5 grid
    // Row 1: C, +/-, %, ÷
    // Row 2: 7, 8, 9, ×
    // Row 3: 4, 5, 6, -
    // Row 4: 1, 2, 3, +
    // Row 5: 0, ., =

    struct {
        const char *text;
        lv_color_t color;
        int row;
        int col;
        int colspan;
        void (*event_cb)(lv_event_t *e);
        void *user_data;
    } buttons[] = {
        // Row 0
        {"C", lv_color_hex(0xFF6B6B), 0, 0, 1, clear_button_event_handler, NULL},
        {"+/-", lv_color_hex(0x4ECDC4), 0, 1, 1, operation_button_event_handler, (void*)'n'},
        {"%", lv_color_hex(0x4ECDC4), 0, 2, 1, operation_button_event_handler, (void*)'%'},
        {"÷", lv_color_hex(0xFFA726), 0, 3, 1, operation_button_event_handler, (void*)'/'},

        // Row 1
        {"7", lv_color_hex(0x6C5CE7), 1, 0, 1, number_button_event_handler, (void*)'7'},
        {"8", lv_color_hex(0x6C5CE7), 1, 1, 1, number_button_event_handler, (void*)'8'},
        {"9", lv_color_hex(0x6C5CE7), 1, 2, 1, number_button_event_handler, (void*)'9'},
        {"×", lv_color_hex(0xFFA726), 1, 3, 1, operation_button_event_handler, (void*)'*'},

        // Row 2
        {"4", lv_color_hex(0x6C5CE7), 2, 0, 1, number_button_event_handler, (void*)'4'},
        {"5", lv_color_hex(0x6C5CE7), 2, 1, 1, number_button_event_handler, (void*)'5'},
        {"6", lv_color_hex(0x6C5CE7), 2, 2, 1, number_button_event_handler, (void*)'6'},
        {"-", lv_color_hex(0xFFA726), 2, 3, 1, operation_button_event_handler, (void*)'-'},

        // Row 3
        {"1", lv_color_hex(0x6C5CE7), 3, 0, 1, number_button_event_handler, (void*)'1'},
        {"2", lv_color_hex(0x6C5CE7), 3, 1, 1, number_button_event_handler, (void*)'2'},
        {"3", lv_color_hex(0x6C5CE7), 3, 2, 1, number_button_event_handler, (void*)'3'},
        {"+", lv_color_hex(0xFFA726), 3, 3, 1, operation_button_event_handler, (void*)'+'},

        // Row 4
        {"0", lv_color_hex(0x6C5CE7), 4, 0, 2, number_button_event_handler, (void*)'0'},
        {".", lv_color_hex(0x6C5CE7), 4, 2, 1, number_button_event_handler, (void*)'.'},
        {"=", lv_color_hex(0x2ECC71), 4, 3, 1, equals_button_event_handler, NULL},
    };

    int button_width = 60;
    int button_height = 50;
    int button_spacing = 5;

    for (int i = 0; i < sizeof(buttons)/sizeof(buttons[0]); i++) {
        lv_obj_t *btn = lv_button_create(button_grid);

        int width = button_width * buttons[i].colspan + button_spacing * (buttons[i].colspan - 1);
        lv_obj_set_size(btn, width, button_height);

        int x = buttons[i].col * (button_width + button_spacing);
        int y = buttons[i].row * (button_height + button_spacing);
        lv_obj_set_pos(btn, x, y);

        apply_button_style(btn);
        lv_obj_set_style_bg_color(btn, buttons[i].color, 0);
        lv_obj_add_event_cb(btn, buttons[i].event_cb, LV_EVENT_CLICKED, buttons[i].user_data);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, buttons[i].text);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        lv_obj_center(label);
    }

    ESP_LOGI(TAG, "Calculator screen created");
}

static void update_display(void) {
    if (display_area) {
        lv_textarea_set_text(display_area, current_display);
    }
}

static void perform_calculation(void) {
    if (operation == '\0') return;

    switch (operation) {
        case '+':
            stored_value += current_value;
            break;
        case '-':
            stored_value -= current_value;
            break;
        case '*':
            stored_value *= current_value;
            break;
        case '/':
            if (current_value != 0) {
                stored_value /= current_value;
            } else {
                strcpy(current_display, "Error");
                update_display();
                return;
            }
            break;
        case '%':
            stored_value = fmod(stored_value, current_value);
            break;
        default:
            return;
    }

    current_value = stored_value;
    snprintf(current_display, sizeof(current_display), "%.8g", stored_value);
    operation = '\0';
    new_number = true;
}

void show_calculator_screen(void) {
    if (!calculator_screen) {
        create_calculator_screen();
    }
    lv_screen_load(calculator_screen);
}

void calculator_screen_back(void) {
    ESP_LOGI(TAG, "Returning to tools screen");
    lv_screen_load(tools_screen);
}

// Event handlers
static void calc_back_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        calculator_screen_back();
    }
}

static void number_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        char digit = (char)(uintptr_t)lv_event_get_user_data(e);

        if (new_number) {
            if (digit == '.') {
                strcpy(current_display, "0.");
            } else {
                current_display[0] = digit;
                current_display[1] = '\0';
            }
            new_number = false;
        } else {
            if (digit == '.' && strchr(current_display, '.')) {
                return; // Already has decimal point
            }

            size_t len = strlen(current_display);
            if (len < sizeof(current_display) - 1) {
                current_display[len] = digit;
                current_display[len + 1] = '\0';
            }
        }

        current_value = atof(current_display);
        update_display();
    }
}

static void operation_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        char new_op = (char)(uintptr_t)lv_event_get_user_data(e);

        if (new_op == 'n') { // +/- (negate)
            current_value = -current_value;
            snprintf(current_display, sizeof(current_display), "%.8g", current_value);
            update_display();
            return;
        }

        if (operation != '\0' && !new_number) {
            perform_calculation();
        } else {
            stored_value = current_value;
        }

        operation = new_op;
        new_number = true;
    }
}

static void equals_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        perform_calculation();
        update_display();
    }
}

static void clear_button_event_handler(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        strcpy(current_display, "0");
        current_value = 0.0;
        stored_value = 0.0;
        operation = '\0';
        new_number = true;
        update_display();
    }
}