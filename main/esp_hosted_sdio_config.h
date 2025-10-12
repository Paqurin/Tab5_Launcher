#ifndef ESP_HOSTED_SDIO_CONFIG_H
#define ESP_HOSTED_SDIO_CONFIG_H

#include "bsp/m5stack_tab5.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"

#ifdef __cplusplus
extern "C" {
#endif

// CRITICAL FIX: ESP32-P4 SDIO pins for ESP-Hosted communication with ESP32-C6
// These are the CORRECT pins for ESP32-C6 WiFi coprocessor (GPIOs 8-15)
// DO NOT confuse with SD card pins (GPIOs 39-44) - they are on separate SDIO interfaces!

// ESP32-C6 SDIO Interface (SDMMC_HOST_SLOT_1)
// Based on M5Stack Tab5 hardware schematic - ESP32-C6 is on SDIO slot 1
#define ESP_HOSTED_SDIO_D0_PIN    GPIO_NUM_12   // SDIO1_D0
#define ESP_HOSTED_SDIO_D1_PIN    GPIO_NUM_13   // SDIO1_D1 (also used for reset - see note below)
#define ESP_HOSTED_SDIO_D2_PIN    GPIO_NUM_14   // SDIO1_D2
#define ESP_HOSTED_SDIO_D3_PIN    GPIO_NUM_15   // SDIO1_D3
#define ESP_HOSTED_SDIO_CMD_PIN   GPIO_NUM_11   // SDIO1_CMD
#define ESP_HOSTED_SDIO_CLK_PIN   GPIO_NUM_10   // SDIO1_CLK

// SD Card Interface (SDMMC_HOST_SLOT_0) - for reference, DO NOT USE for ESP-Hosted
// #define BSP_SD_D0   GPIO_NUM_39  // SDIO0_D0
// #define BSP_SD_D1   GPIO_NUM_40  // SDIO0_D1
// #define BSP_SD_D2   GPIO_NUM_41  // SDIO0_D2
// #define BSP_SD_D3   GPIO_NUM_42  // SDIO0_D3
// #define BSP_SD_CMD  GPIO_NUM_44  // SDIO0_CMD
// #define BSP_SD_CLK  GPIO_NUM_43  // SDIO0_CLK

// SDIO configuration for ESP-Hosted (slot 1, NOT slot 0!)
#define ESP_HOSTED_SDIO_HOST_ID   SDMMC_HOST_SLOT_1  // CRITICAL: Use slot 1 for ESP32-C6
#define ESP_HOSTED_SDIO_FREQ_KHZ  20000              // 20 MHz SDIO clock (reduced for stability)
#define ESP_HOSTED_SDIO_BUS_WIDTH 4                  // 4-bit SDIO bus

// ESP32-C6 reset pin configuration
// IMPORTANT: GPIO 13 (SDIO1_D1) doubles as reset pin in 1-bit mode during initialization
// After reset pulse completes, switch to 4-bit mode for normal operation
#define ESP_HOSTED_C6_RESET_PIN   GPIO_NUM_13        // Same as D1, used for reset sequence
#define ESP_HOSTED_C6_RESET_DELAY_MS  100            // Reset pulse duration (100ms)
#define ESP_HOSTED_C6_BOOT_DELAY_MS   500            // Boot wait after reset (500ms)

// ESP32-C6 power management via I2C (AXP2101 PMIC)
#define ESP_HOSTED_C6_POWER_I2C_ADDR    0x44
#define ESP_HOSTED_C6_POWER_REG         0x05
#define ESP_HOSTED_C6_POWER_ON_VALUE    0x01
#define ESP_HOSTED_C6_POWER_OFF_VALUE   0x00

// ESP-Hosted specific configuration
#define ESP_HOSTED_MAX_RX_BUFFER_SIZE   2048
#define ESP_HOSTED_MAX_TX_BUFFER_SIZE   2048
#define ESP_HOSTED_SDIO_TIMEOUT_MS      1000

/**
 * @brief ESP-Hosted SDIO pin configuration structure
 */
typedef struct {
    gpio_num_t d0_pin;
    gpio_num_t d1_pin;
    gpio_num_t d2_pin;
    gpio_num_t d3_pin;
    gpio_num_t cmd_pin;
    gpio_num_t clk_pin;
    gpio_num_t reset_pin;    // Added reset pin
    uint32_t freq_khz;
    uint8_t bus_width;
} esp_hosted_sdio_pin_config_t;

/**
 * @brief Get default SDIO pin configuration for M5Stack Tab5
 *
 * @return esp_hosted_sdio_pin_config_t Default configuration
 */
esp_hosted_sdio_pin_config_t esp_hosted_get_default_sdio_config(void);

/**
 * @brief Initialize ESP32-C6 power management
 *
 * Powers on the ESP32-C6 coprocessor via I2C commands to AXP2101 PMIC
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_FAIL on I2C communication failure
 */
esp_err_t esp_hosted_init_c6_power(void);

/**
 * @brief Deinitialize ESP32-C6 power management
 *
 * Powers off the ESP32-C6 coprocessor
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_FAIL on I2C communication failure
 */
esp_err_t esp_hosted_deinit_c6_power(void);

/**
 * @brief Reset ESP32-C6 coprocessor
 *
 * Performs hardware reset sequence using GPIO 13 (SDIO1_D1)
 * Must be called BEFORE SDIO initialization
 *
 * @return
 *     - ESP_OK on success
 *     - ESP_FAIL on reset failure
 */
esp_err_t esp_hosted_reset_c6(void);

/**
 * @brief Initialize SDIO pins for ESP-Hosted
 *
 * Configures GPIO pins for SDIO communication with ESP32-C6
 * Must be called AFTER esp_hosted_reset_c6()
 *
 * @param config Pin configuration structure
 * @return
 *     - ESP_OK on success
 *     - ESP_ERR_INVALID_ARG on invalid pin configuration
 */
esp_err_t esp_hosted_init_sdio_pins(const esp_hosted_sdio_pin_config_t *config);

/**
 * @brief Check if SDIO pins are available (not in use by SD card)
 *
 * @return
 *     - true if pins are available for SDIO WiFi (slot 1 always available)
 *     - false if pins are in use by SD card (should never happen, different slots)
 */
bool esp_hosted_sdio_pins_available(void);

#ifdef __cplusplus
}
#endif

#endif // ESP_HOSTED_SDIO_CONFIG_H
