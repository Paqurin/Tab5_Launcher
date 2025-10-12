#include "esp_hosted_sdio_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "bsp/m5stack_tab5.h"
#include "driver/i2c_master.h"
#include <inttypes.h>

static const char *TAG = "ESP_HOSTED_SDIO";

static bool s_sdio_initialized = false;

// Note: esp_hosted_get_default_sdio_config() is provided by esp_hosted component
// Removed to avoid multiple definition linker error

esp_err_t esp_hosted_init_c6_power(void) {
    ESP_LOGI(TAG, "Initializing ESP32-C6 power management");

    // Use BSP function to enable WiFi power
    bsp_set_wifi_power_enable(true);

    // Alternative direct I2C approach if BSP function is not sufficient
    // This matches the power management described in the micropython discussion
    i2c_master_bus_handle_t i2c_handle = bsp_i2c_get_handle();
    if (i2c_handle != NULL) {
        uint8_t power_on_cmd = ESP_HOSTED_C6_POWER_ON_VALUE;
        i2c_master_dev_handle_t dev_handle;

        // Create I2C device handle for power management IC
        i2c_device_config_t dev_cfg = {};
        dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_cfg.device_address = ESP_HOSTED_C6_POWER_I2C_ADDR;
        dev_cfg.scl_speed_hz = 100000;

        esp_err_t ret = i2c_master_bus_add_device(i2c_handle, &dev_cfg, &dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
            return ret;
        }

        // Write power-on command
        uint8_t write_buf[2] = {ESP_HOSTED_C6_POWER_REG, power_on_cmd};
        ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), 1000);

        // Clean up device handle
        i2c_master_bus_rm_device(dev_handle);

        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to power on ESP32-C6 coprocessor via I2C: %s", esp_err_to_name(ret));
            return ret;
        }
    } else {
        ESP_LOGW(TAG, "I2C handle not available, relying on BSP power management only");
    }

    ESP_LOGI(TAG, "ESP32-C6 coprocessor powered on successfully");

    // Small delay to allow ESP32-C6 to boot up
    vTaskDelay(pdMS_TO_TICKS(100));

    return ESP_OK;
}

esp_err_t esp_hosted_deinit_c6_power(void) {
    ESP_LOGI(TAG, "Deinitializing ESP32-C6 power management");

    // Power off ESP32-C6 coprocessor
    i2c_master_bus_handle_t i2c_handle = bsp_i2c_get_handle();
    if (i2c_handle != NULL) {
        uint8_t power_off_cmd = ESP_HOSTED_C6_POWER_OFF_VALUE;
        i2c_master_dev_handle_t dev_handle;

        // Create I2C device handle for power management IC
        i2c_device_config_t dev_cfg = {};
        dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
        dev_cfg.device_address = ESP_HOSTED_C6_POWER_I2C_ADDR;
        dev_cfg.scl_speed_hz = 100000;

        esp_err_t ret = i2c_master_bus_add_device(i2c_handle, &dev_cfg, &dev_handle);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to add I2C device for power-off: %s", esp_err_to_name(ret));
        } else {
            // Write power-off command
            uint8_t write_buf[2] = {ESP_HOSTED_C6_POWER_REG, power_off_cmd};
            ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), 1000);

            // Clean up device handle
            i2c_master_bus_rm_device(dev_handle);

            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to power off ESP32-C6 coprocessor via I2C: %s", esp_err_to_name(ret));
            }
        }
    }

    // Use BSP function to disable WiFi power
    bsp_set_wifi_power_enable(false);

    ESP_LOGI(TAG, "ESP32-C6 coprocessor powered off successfully");
    return ESP_OK;
}

esp_err_t esp_hosted_init_sdio_pins(const esp_hosted_sdio_pin_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid SDIO pin configuration");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Initializing SDIO pins for ESP-Hosted");
    ESP_LOGI(TAG, "D0: GPIO%d, D1: GPIO%d, D2: GPIO%d, D3: GPIO%d",
             config->d0_pin, config->d1_pin, config->d2_pin, config->d3_pin);
    ESP_LOGI(TAG, "CMD: GPIO%d, CLK: GPIO%d", config->cmd_pin, config->clk_pin);
    ESP_LOGI(TAG, "Frequency: %" PRIu32 " kHz, Bus width: %" PRIu8 "-bit", config->freq_khz, config->bus_width);

    // Configure SDIO pins for proper drive strength and pull resistors
    // These settings are critical for SDIO communication stability

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;   // Important for SDIO
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    // Configure data pins D0-D3
    io_conf.pin_bit_mask = (1ULL << config->d0_pin) |
                           (1ULL << config->d1_pin) |
                           (1ULL << config->d2_pin) |
                           (1ULL << config->d3_pin);
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SDIO data pins: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure CMD pin
    io_conf.pin_bit_mask = (1ULL << config->cmd_pin);
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SDIO CMD pin: %s", esp_err_to_name(ret));
        return ret;
    }

    // Configure CLK pin (output only, no pull resistors for clock)
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = (1ULL << config->clk_pin);
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SDIO CLK pin: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set appropriate drive strength for SDIO signals
    gpio_set_drive_capability(config->d0_pin, GPIO_DRIVE_CAP_2);
    gpio_set_drive_capability(config->d1_pin, GPIO_DRIVE_CAP_2);
    gpio_set_drive_capability(config->d2_pin, GPIO_DRIVE_CAP_2);
    gpio_set_drive_capability(config->d3_pin, GPIO_DRIVE_CAP_2);
    gpio_set_drive_capability(config->cmd_pin, GPIO_DRIVE_CAP_2);
    gpio_set_drive_capability(config->clk_pin, GPIO_DRIVE_CAP_3); // Higher drive for clock

    s_sdio_initialized = true;
    ESP_LOGI(TAG, "SDIO pins initialized successfully");

    return ESP_OK;
}

bool esp_hosted_sdio_pins_available(void) {
    // Check if SD card is currently mounted/active
    // This is a simplified check - in practice, you'd want to check
    // if the SDMMC host is currently in use

    // For now, assume pins are available if SDIO hasn't been initialized
    // In a real implementation, this would check SD card mount status
    return !s_sdio_initialized;
}