#include "hal.h"
#include <stdio.h>
#include "esp_log.h"

lv_disp_t *lvDisp = NULL;
lv_indev_t *lvTouchpad = NULL;

extern esp_lcd_touch_handle_t _lcd_touch_handle;

// IO Expander definitions for direct control
#define I2C_DEV_ADDR_PI4IOE1  0x43
#define I2C_MASTER_TIMEOUT_MS 50
#define PI4IO_REG_OUT_SET     0x05

// Bit manipulation macros
#define setbit(x, y) x |= (0x01 << y)
#define clrbit(x, y) x &= ~(0x01 << y)

// Local I2C device handle for touch power control
static i2c_master_dev_handle_t hal_i2c_dev_handle_pi4ioe1 = NULL;

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

static esp_err_t hal_init_touch_power_control(void)
{
    if (hal_i2c_dev_handle_pi4ioe1 != NULL) {
        return ESP_OK; // Already initialized
    }
    
    // Get the I2C bus handle
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();
    if (i2c_bus_handle == NULL) {
        ESP_LOGE("HAL", "Failed to get I2C bus handle");
        return ESP_FAIL;
    }
    
    // Create I2C device handle for PI4IOE1
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_DEV_ADDR_PI4IOE1,
        .scl_speed_hz = 400000,
    };
    
    esp_err_t ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &hal_i2c_dev_handle_pi4ioe1);
    if (ret != ESP_OK) {
        ESP_LOGE("HAL", "Failed to add PI4IOE1 device to I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI("HAL", "✓ Touch power control I2C device initialized");
    return ESP_OK;
}

static esp_err_t hal_touchpad_power_control(bool enable)
{
    ESP_LOGI("HAL", "Touch panel power control: %s", enable ? "ON" : "OFF");
    
    // Ensure I2C device is initialized
    esp_err_t ret = hal_init_touch_power_control();
    if (ret != ESP_OK) {
        return ret;
    }
    
    uint8_t write_buf[2] = {0};
    uint8_t read_buf[1] = {0};
    
    // Read current state of PI4IOE1 output register
    write_buf[0] = PI4IO_REG_OUT_SET;
    ret = i2c_master_transmit_receive(hal_i2c_dev_handle_pi4ioe1, write_buf, 1, read_buf, 1, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE("HAL", "Failed to read IO expander state: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Modify P5 (TP_RST) bit - bit 5 controls touch panel power
    write_buf[0] = PI4IO_REG_OUT_SET;
    write_buf[1] = read_buf[0];
    
    if (enable) {
        setbit(write_buf[1], 5);  // Set P5 HIGH to power on GT911
        ESP_LOGI("HAL", "Setting TP_RST (P5) HIGH - powering ON GT911");
    } else {
        clrbit(write_buf[1], 5);  // Set P5 LOW to power off GT911
        ESP_LOGI("HAL", "Setting TP_RST (P5) LOW - powering OFF GT911");
    }
    
    // Write the modified state back
    ret = i2c_master_transmit(hal_i2c_dev_handle_pi4ioe1, write_buf, 2, I2C_MASTER_TIMEOUT_MS);
    if (ret != ESP_OK) {
        ESP_LOGE("HAL", "Failed to write IO expander state: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI("HAL", "✓ Touch panel power control completed: %s", enable ? "ON" : "OFF");
    return ESP_OK;
}

void hal_init(void)
{
    // Initialize I2C bus
    bsp_i2c_init();
    vTaskDelay(pdMS_TO_TICKS(200));

    // Get I2C bus handle and initialize IO expander
    i2c_master_bus_handle_t i2c_bus_handle = bsp_i2c_get_handle();
    bsp_io_expander_pi4ioe_init(i2c_bus_handle);

    // Initialize display and touch
    bsp_reset_tp();
    lvDisp = bsp_display_start();
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

void hal_touchpad_deinit(void)
{
    ESP_LOGI("HAL", "=== GT911 POWER-OFF DE-INITIALIZATION ===");
    
    // Step 1: Put touch controller to sleep if handle is available
    if (_lcd_touch_handle != NULL) {
        ESP_LOGI("HAL", "Step 1: Putting GT911 touch controller to sleep...");
        esp_err_t ret = esp_lcd_touch_enter_sleep(_lcd_touch_handle);
        if (ret == ESP_OK) {
            ESP_LOGI("HAL", "✓ GT911 entered sleep mode successfully");
        } else {
            ESP_LOGW("HAL", "⚠ Failed to put GT911 to sleep: %s", esp_err_to_name(ret));
        }
        
        // Allow time for sleep command to be processed
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Step 2: Delete the touch handle to free resources
        ESP_LOGI("HAL", "Step 2: Deleting touch handle...");
        ret = esp_lcd_touch_del(_lcd_touch_handle);
        if (ret == ESP_OK) {
            ESP_LOGI("HAL", "✓ Touch handle deleted successfully");
            _lcd_touch_handle = NULL;
        } else {
            ESP_LOGW("HAL", "⚠ Failed to delete touch handle: %s", esp_err_to_name(ret));
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    } else {
        ESP_LOGW("HAL", "Touch handle is NULL, skipping controller sleep/delete");
    }
    
    // Step 3: Reset interrupt GPIO to prevent spurious signals
    ESP_LOGI("HAL", "Step 3: Resetting touch interrupt GPIO 23...");
    gpio_reset_pin(GPIO_NUM_23);
    gpio_set_direction(GPIO_NUM_23, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_23, GPIO_FLOATING);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Step 4: POWER OFF the GT911 completely via IO expander
    ESP_LOGI("HAL", "Step 4: POWERING OFF GT911 via IO expander...");
    esp_err_t ret = hal_touchpad_power_control(false);
    if (ret == ESP_OK) {
        ESP_LOGI("HAL", "✓ GT911 powered OFF successfully!");
        ESP_LOGI("HAL", "✓ GT911 will be in powered-off state for firmware boot");
    } else {
        ESP_LOGW("HAL", "⚠ Failed to power off GT911, trying BSP reset as fallback");
        bsp_reset_tp();
    }
    
    // Step 5: Hold power off state for sufficient time
    ESP_LOGI("HAL", "Step 5: Holding GT911 in powered-off state...");
    vTaskDelay(pdMS_TO_TICKS(500));
    
    ESP_LOGI("HAL", "✓ GT911 de-initialization complete!");
    ESP_LOGI("HAL", "✓ GT911 is now POWERED OFF and ready for firmware initialization!");
    ESP_LOGI("HAL", "================================================");
}