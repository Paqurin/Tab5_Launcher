#include "hardware_control.h"
#include "bsp/m5stack_tab5.h"
#include "esp_log.h"

static const char *TAG = "HARDWARE_CTRL";

// Internal state tracking (since BSP doesn't provide getters)
static hardware_switches_t current_switches = {
    .charge_enable = true,      // Default enabled
    .charge_qc_enable = true,   // Default enabled - needed for battery charging
    .usb_5v_enable = true,      // Default enabled
    .ext_5v_enable = false,     // Default disabled
    .ext_antenna_enable = false // Default internal antenna
};

static bool hardware_initialized = false;

esp_err_t hardware_control_init(void)
{
    if (hardware_initialized) {
        ESP_LOGW(TAG, "Hardware control already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing hardware control system");

    // Initialize the IO expanders first - they need the I2C bus handle from M5Unified
    i2c_master_bus_handle_t i2c_bus = bsp_i2c_get_handle();
    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "I2C bus not available from BSP - M5Unified must be initialized first");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Initializing IO expanders for hardware control");
    bsp_io_expander_pi4ioe_init(i2c_bus);

    // Initialize with default states
    esp_err_t ret;

    ret = hardware_set_charge_enable(current_switches.charge_enable);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize charge enable");
        return ret;
    }

    ret = hardware_set_charge_qc_enable(current_switches.charge_qc_enable);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize charge QC enable");
        return ret;
    }

    ret = hardware_set_usb_5v_enable(current_switches.usb_5v_enable);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize USB 5V enable");
        return ret;
    }

    ret = hardware_set_ext_5v_enable(current_switches.ext_5v_enable);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external 5V enable");
        return ret;
    }

    ret = hardware_set_ext_antenna_enable(current_switches.ext_antenna_enable);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external antenna enable");
        return ret;
    }

    hardware_initialized = true;
    ESP_LOGI(TAG, "Hardware control system initialized successfully");
    return ESP_OK;
}

esp_err_t hardware_get_switches(hardware_switches_t *switches)
{
    if (!switches) {
        return ESP_ERR_INVALID_ARG;
    }

    *switches = current_switches;
    return ESP_OK;
}

esp_err_t hardware_get_detection(hardware_detection_t *detection)
{
    if (!detection) {
        return ESP_ERR_INVALID_ARG;
    }

    detection->usb_c_detected = bsp_usb_c_detect();
    // bsp_usb_a_detect() is not implemented in BSP, return false for now
    detection->usb_a_detected = false;
    detection->headphone_detected = bsp_headphone_detect();

    return ESP_OK;
}

esp_err_t hardware_set_charge_enable(bool enable)
{
    ESP_LOGI(TAG, "Setting charge enable: %s", enable ? "ON" : "OFF");
    bsp_set_charge_en(enable);
    current_switches.charge_enable = enable;
    return ESP_OK;
}

esp_err_t hardware_set_charge_qc_enable(bool enable)
{
    ESP_LOGI(TAG, "Setting charge QC enable: %s", enable ? "ON" : "OFF");
    bsp_set_charge_qc_en(enable);
    current_switches.charge_qc_enable = enable;
    return ESP_OK;
}

esp_err_t hardware_set_usb_5v_enable(bool enable)
{
    ESP_LOGI(TAG, "Setting USB 5V enable: %s", enable ? "ON" : "OFF");
    bsp_set_usb_5v_en(enable);
    current_switches.usb_5v_enable = enable;
    return ESP_OK;
}

esp_err_t hardware_set_ext_5v_enable(bool enable)
{
    ESP_LOGI(TAG, "Setting external 5V enable: %s", enable ? "ON" : "OFF");
    bsp_set_ext_5v_en(enable);
    current_switches.ext_5v_enable = enable;
    return ESP_OK;
}

esp_err_t hardware_set_ext_antenna_enable(bool enable)
{
    ESP_LOGI(TAG, "Setting external antenna enable: %s", enable ? "ON" : "OFF");
    bsp_set_ext_antenna_enable(enable);
    current_switches.ext_antenna_enable = enable;
    return ESP_OK;
}

bool hardware_get_charge_enable(void)
{
    return current_switches.charge_enable;
}

bool hardware_get_charge_qc_enable(void)
{
    return current_switches.charge_qc_enable;
}

bool hardware_get_usb_5v_enable(void)
{
    return current_switches.usb_5v_enable;
}

bool hardware_get_ext_5v_enable(void)
{
    return current_switches.ext_5v_enable;
}

bool hardware_get_ext_antenna_enable(void)
{
    return current_switches.ext_antenna_enable;
}

bool hardware_get_usb_c_detect(void)
{
    return bsp_usb_c_detect();
}

bool hardware_get_usb_a_detect(void)
{
    // bsp_usb_a_detect() is not implemented in BSP, return false for now
    return false;
}

bool hardware_get_headphone_detect(void)
{
    return bsp_headphone_detect();
}