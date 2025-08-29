#include "hal.h"
#include "bsp/m5stack_tab5.h"
#include "esp_log.h"
#include "esp_lcd_touch.h"

lv_display_t *lvDisp = NULL;
lv_indev_t *lvTouchpad = NULL;

extern esp_lcd_touch_handle_t _lcd_touch_handle;

static void lvgl_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (_lcd_touch_handle == NULL)
    {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    uint16_t touch_x[1];
    uint16_t touch_y[1];
    uint16_t touch_strength[1];
    uint8_t touch_cnt = 0;

    esp_lcd_touch_read_data(_lcd_touch_handle);
    bool touchpad_pressed =
        esp_lcd_touch_get_coordinates(_lcd_touch_handle, touch_x, touch_y, touch_strength, &touch_cnt, 1);

    if (!touchpad_pressed)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_x[0];
        data->point.y = touch_y[0];
    }
}

void hal_init(void)
{
    // Initialize I2C bus
    bsp_i2c_init();
    vTaskDelay(pdMS_TO_TICKS(200));

    // Get I2C bus handle and initialize IO expander
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();
    bsp_io_expander_pi4ioe_init(i2c_bus_handle);

    // Initialize display and touch with PSRAM buffers
    bsp_reset_tp();
    
    // Configure display to use PSRAM for buffers
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size   = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags         = {
#if CONFIG_BSP_LCD_COLOR_FORMAT_RGB888
            .buff_dma = false,
#else
            .buff_dma = true,
#endif
            .buff_spiram = true,  // Enable PSRAM for buffers
            .sw_rotate   = true,
        }
    };
    
    lvDisp = bsp_display_start_with_config(&cfg);
    lv_display_set_rotation(lvDisp, LV_DISPLAY_ROTATION_90);
    bsp_display_backlight_on();
}

void hal_touchpad_init(void)
{
    // Initialize touchpad input
    lvTouchpad = lv_indev_create();
    lv_indev_set_type(lvTouchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvTouchpad, lvgl_read_cb);
    lv_indev_set_display(lvTouchpad, lvDisp);
}

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the touch chip without initializing it again 
//     // would leave init to the user FW which shall fix the problem
//     bsp_reset_tp();
// }

// void hal_touchpad_deinit(void) not needed anymore
// {
//     // I think simply resetting the