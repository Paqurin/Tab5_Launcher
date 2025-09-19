/*
 * ESP32-P4 specific initialization code for MicroPython
 *
 * This file contains ESP32-P4 specific initialization routines
 * that are called during MicroPython startup.
 */

#include "py/runtime.h"
#include "py/mphal.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_psram.h"
#include "driver/gpio.h"

static const char *TAG = "esp32p4_init";

// ESP32-P4 specific hardware initialization
void esp32p4_board_init(void) {
    ESP_LOGI(TAG, "ESP32-P4 board initialization");

    // Initialize PSRAM if available
    if (esp_psram_is_initialized()) {
        ESP_LOGI(TAG, "PSRAM initialized: %d MB", esp_psram_get_size() / (1024 * 1024));
    } else {
        ESP_LOGW(TAG, "PSRAM not available or initialization failed");
    }

    // Configure GPIO for ESP32-P4 specific features
    // Note: These are generic settings, board-specific configs should override

    // Configure LED GPIO if available (adjust pin number for your board)
    #ifdef MICROPY_HW_LED1_PIN
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MICROPY_HW_LED1_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);
    gpio_set_level(MICROPY_HW_LED1_PIN, 0);
    #endif

    // Initialize I2C pins (if defined)
    #if defined(MICROPY_HW_I2C_SCL) && defined(MICROPY_HW_I2C_SDA)
    // I2C initialization will be handled by machine_i2c.c
    ESP_LOGI(TAG, "I2C pins configured: SCL=%d, SDA=%d", MICROPY_HW_I2C_SCL, MICROPY_HW_I2C_SDA);
    #endif

    // Initialize SPI pins (if defined)
    #if defined(MICROPY_HW_SPI_SCK) && defined(MICROPY_HW_SPI_MOSI) && defined(MICROPY_HW_SPI_MISO)
    ESP_LOGI(TAG, "SPI pins configured: SCK=%d, MOSI=%d, MISO=%d",
             MICROPY_HW_SPI_SCK, MICROPY_HW_SPI_MOSI, MICROPY_HW_SPI_MISO);
    #endif

    // M5Stack Tab5 specific initialization
    #if MICROPY_HW_TAB5_PINS
    ESP_LOGI(TAG, "M5Stack Tab5 pin configuration enabled");

    // Configure display control pins
    gpio_config_t display_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << MICROPY_HW_TAB5_LCD_CS) |
                       (1ULL << MICROPY_HW_TAB5_LCD_DC) |
                       (1ULL << MICROPY_HW_TAB5_LCD_RST),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&display_conf);

    // Configure touch interrupt pin
    gpio_config_t touch_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << MICROPY_HW_TAB5_TOUCH_INT),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&touch_conf);
    #endif

    ESP_LOGI(TAG, "ESP32-P4 board initialization complete");
}

// ESP32-P4 specific cleanup (called on reset/shutdown)
void esp32p4_board_deinit(void) {
    ESP_LOGI(TAG, "ESP32-P4 board cleanup");

    // Add any specific cleanup code here
    // For example, turn off external peripherals, save state, etc.
}

// Board-specific pin validation
bool esp32p4_pin_is_valid(int pin) {
    // ESP32-P4 valid GPIO pins
    // Note: This is a simplified check, actual valid pins depend on board design
    if (pin < 0 || pin >= 55) {
        return false;
    }

    // Some pins may be reserved for specific functions
    // Add board-specific restrictions here

    return true;
}

// Get board-specific information
const char* esp32p4_get_board_info(void) {
    #if MICROPY_HW_TAB5_PINS
    return "ESP32-P4 (M5Stack Tab5 configuration)";
    #else
    return "ESP32-P4 Generic";
    #endif
}

// ESP32-P4 specific capability check
bool esp32p4_has_capability(const char* capability) {
    if (strcmp(capability, "wifi") == 0) {
        return false; // ESP32-P4 has no WiFi
    }
    if (strcmp(capability, "bluetooth") == 0) {
        return false; // ESP32-P4 has no Bluetooth
    }
    if (strcmp(capability, "psram") == 0) {
        return esp_psram_is_initialized();
    }
    if (strcmp(capability, "dsi_display") == 0) {
        return true; // ESP32-P4 supports DSI displays
    }
    if (strcmp(capability, "dual_core") == 0) {
        return true; // ESP32-P4 is dual core
    }

    return false;
}