#pragma once

#include "esp_err.h"

/**
 * @brief Take a screenshot and save it to SD card
 *
 * @return esp_err_t ESP_OK on success, error code on failure
 */
esp_err_t screenshot_take_and_save(void);