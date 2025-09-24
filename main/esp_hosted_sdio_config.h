#ifndef ESP_HOSTED_SDIO_CONFIG_H
#define ESP_HOSTED_SDIO_CONFIG_H

#include "bsp/m5stack_tab5.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"

#ifdef __cplusplus
extern "C" {
#endif

// ESP32-P4 SDIO pins for ESP-Hosted communication with ESP32-C6
// Based on M5Stack Tab5 hardware layout - reusing SD card pins for SDIO WiFi
// Note: This may conflict with SD card usage if both are active simultaneously

#define ESP_HOSTED_SDIO_D0_PIN    BSP_SD_D0   // GPIO_NUM_39
#define ESP_HOSTED_SDIO_D1_PIN    BSP_SD_D1   // GPIO_NUM_40
#define ESP_HOSTED_SDIO_D2_PIN    BSP_SD_D2   // GPIO_NUM_41
#define ESP_HOSTED_SDIO_D3_PIN    BSP_SD_D3   // GPIO_NUM_42
#define ESP_HOSTED_SDIO_CMD_PIN   BSP_SD_CMD  // GPIO_NUM_44
#define ESP_HOSTED_SDIO_CLK_PIN   BSP_SD_CLK  // GPIO_NUM_43

// SDIO configuration for ESP-Hosted
#define ESP_HOSTED_SDIO_HOST_ID   SDMMC_HOST_SLOT_1
#define ESP_HOSTED_SDIO_FREQ_KHZ  40000  // 40 MHz SDIO clock
#define ESP_HOSTED_SDIO_BUS_WIDTH 4      // 4-bit SDIO bus

// ESP32-C6 power management via I2C (from BSP functions)
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
 * Powers on the ESP32-C6 coprocessor via I2C commands
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
 * @brief Initialize SDIO pins for ESP-Hosted
 *
 * Configures GPIO pins for SDIO communication with ESP32-C6
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
 *     - true if pins are available for SDIO WiFi
 *     - false if pins are in use by SD card
 */
bool esp_hosted_sdio_pins_available(void);

#ifdef __cplusplus
}
#endif

#endif // ESP_HOSTED_SDIO_CONFIG_H