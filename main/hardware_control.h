#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Hardware switch states
typedef struct {
    bool charge_enable;
    bool charge_qc_enable;
    bool usb_5v_enable;
    bool ext_5v_enable;
    bool ext_antenna_enable;
} hardware_switches_t;

// Hardware detection states
typedef struct {
    bool usb_c_detected;
    bool usb_a_detected;
    bool headphone_detected;
} hardware_detection_t;

/**
 * @brief Initialize hardware control system
 * @return ESP_OK on success
 */
esp_err_t hardware_control_init(void);

/**
 * @brief Get current switch states
 * @param switches Pointer to switches structure to fill
 * @return ESP_OK on success
 */
esp_err_t hardware_get_switches(hardware_switches_t *switches);

/**
 * @brief Get current detection states
 * @param detection Pointer to detection structure to fill
 * @return ESP_OK on success
 */
esp_err_t hardware_get_detection(hardware_detection_t *detection);

/**
 * @brief Set battery charging enable
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t hardware_set_charge_enable(bool enable);

/**
 * @brief Set quick charge enable
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t hardware_set_charge_qc_enable(bool enable);

/**
 * @brief Set USB-A 5V enable
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t hardware_set_usb_5v_enable(bool enable);

/**
 * @brief Set external 5V enable
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t hardware_set_ext_5v_enable(bool enable);

/**
 * @brief Set external antenna enable
 * @param enable true to enable, false to disable
 * @return ESP_OK on success
 */
esp_err_t hardware_set_ext_antenna_enable(bool enable);

/**
 * @brief Get battery charging enable state
 * @return true if enabled, false if disabled
 */
bool hardware_get_charge_enable(void);

/**
 * @brief Get quick charge enable state
 * @return true if enabled, false if disabled
 */
bool hardware_get_charge_qc_enable(void);

/**
 * @brief Get USB-A 5V enable state
 * @return true if enabled, false if disabled
 */
bool hardware_get_usb_5v_enable(void);

/**
 * @brief Get external 5V enable state
 * @return true if enabled, false if disabled
 */
bool hardware_get_ext_5v_enable(void);

/**
 * @brief Get external antenna enable state
 * @return true if enabled, false if disabled
 */
bool hardware_get_ext_antenna_enable(void);

/**
 * @brief Get USB-C detection state
 * @return true if USB-C is connected, false otherwise
 */
bool hardware_get_usb_c_detect(void);

/**
 * @brief Get USB-A detection state
 * @return true if USB-A is connected, false otherwise
 */
bool hardware_get_usb_a_detect(void);

/**
 * @brief Get headphone detection state
 * @return true if headphone is connected, false otherwise
 */
bool hardware_get_headphone_detect(void);

#ifdef __cplusplus
}
#endif