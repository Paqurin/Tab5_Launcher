#include "M5Unified.h"

extern "C" {
#include "power_monitor.h"
#include "esp_log.h"
}

static const char *TAG = "POWER_MONITOR";

// INA226 I2C address (confirmed from M5Stack UserDemo)
#define INA226_ADDR      0x41

// INA226 register addresses
#define INA226_REG_CONFIG    0x00
#define INA226_REG_SHUNT_V   0x01
#define INA226_REG_BUS_V     0x02
#define INA226_REG_POWER     0x03
#define INA226_REG_CURRENT   0x04
#define INA226_REG_CALIB     0x05

static bool initialized = false;
static float currentLSB = 0.0f;
static float powerLSB = 0.0f;

// Convert raw bus voltage to volts (LSB = 1.25mV)
static float raw_to_voltage(uint16_t raw) {
    return (raw * 1.25f) / 1000.0f;
}

// Convert raw current using calculated LSB
static float raw_to_current_ma(int16_t raw) {
    float current_a = (float)raw * currentLSB; // Current in Amperes
    float current_ma = current_a * 1000.0f;    // Convert A to mA
    return current_ma;
}

static bool ina226_read_register(uint8_t reg, uint16_t *value) {
    if (!value) {
        ESP_LOGD(TAG, "Invalid parameters for register read: value=%p", value);
        return false;
    }

    // Read 2 bytes from the register using M5Unified's I2C interface
    uint8_t data[2];
    bool success = M5.In_I2C.readRegister(INA226_ADDR, reg, data, 2, 100000);  // 100ms timeout
    if (!success) {
        ESP_LOGD(TAG, "I2C read failed for register 0x%02x", reg);
        return false;
    }

    // INA226 uses big-endian format
    *value = (data[0] << 8) | data[1];
    ESP_LOGV(TAG, "Read register 0x%02x: 0x%04x (raw bytes: 0x%02x 0x%02x)", reg, *value, data[0], data[1]);
    return true;
}

static bool ina226_write_register(uint8_t reg, uint16_t value) {
    // Prepare data in big-endian format for INA226
    uint8_t data[2] = {
        (uint8_t)((value >> 8) & 0xFF),  // High byte
        (uint8_t)(value & 0xFF)          // Low byte
    };

    // Use M5Unified's I2C interface for writing
    bool success = M5.In_I2C.writeRegister(INA226_ADDR, reg, data, 2, 100000);  // 100ms timeout
    if (!success) {
        ESP_LOGW(TAG, "Failed to write register 0x%02x", reg);
        return false;
    }

    return true;
}

static bool ina226_calibrate(float shunt_resistor, float max_expected_current) {
    // Calculate LSB values exactly as M5Stack UserDemo does
    currentLSB = max_expected_current / 32768.0f;
    powerLSB = currentLSB * 25.0f;
    
    // Calculate calibration register value using M5Stack formula
    float cal_value = 0.00512f / (currentLSB * shunt_resistor);
    uint16_t cal = (uint16_t)cal_value;
    
    ESP_LOGI(TAG, "INA226 Calibration Details:");
    ESP_LOGI(TAG, "  Shunt resistor: %.6fΩ", shunt_resistor);
    ESP_LOGI(TAG, "  Max expected current: %.3fA", max_expected_current);
    ESP_LOGI(TAG, "  Current LSB: %.9fA (%.6fmA per step)", currentLSB, currentLSB * 1000.0f);
    ESP_LOGI(TAG, "  Power LSB: %.9fW", powerLSB);
    ESP_LOGI(TAG, "  Cal calculation: 0.00512 / (%.9f * %.6f) = %.3f", currentLSB, shunt_resistor, cal_value);
    ESP_LOGI(TAG, "  Cal register: 0x%04x (%d)", cal, cal);
    
    bool result = ina226_write_register(INA226_REG_CALIB, cal);
    if (result) {
        ESP_LOGI(TAG, "INA226 calibration register written successfully");
        
        // Verify calibration was written correctly
        uint16_t read_cal;
        if (ina226_read_register(INA226_REG_CALIB, &read_cal)) {
            ESP_LOGI(TAG, "Verified calibration register: 0x%04x (expected 0x%04x)", read_cal, cal);
            if (read_cal != cal) {
                ESP_LOGW(TAG, "Calibration register mismatch! Expected 0x%04x, got 0x%04x", cal, read_cal);
            }
        } else {
            ESP_LOGW(TAG, "Failed to read back calibration register for verification");
        }
    } else {
        ESP_LOGE(TAG, "Failed to write INA226 calibration register");
    }
    
    return result;
}

// Removed power_i2c_bus - using M5Unified I2C interface instead

static bool ina226_init_device(void) {
    // Use M5Unified's I2C interface instead of creating our own bus
    // This avoids I2C bus conflicts that cause communication failures
    ESP_LOGI(TAG, "Using M5Unified I2C interface for power monitoring");

    // Test communication with INA226 using direct register read
    ESP_LOGI(TAG, "Testing INA226 communication by reading config register...");
    uint16_t config;
    if (!ina226_read_register(INA226_REG_CONFIG, &config)) {
        ESP_LOGE(TAG, "Failed to communicate with INA226 at address 0x%02x", INA226_ADDR);
        return false;
    }

    ESP_LOGI(TAG, "Found INA226 at address 0x%02x, config: 0x%04x", INA226_ADDR, config);
    
    // Configure INA226 based on M5Stack UserDemo settings
    // INA226_AVERAGES_16, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT
    uint16_t new_config = 0x4527; // 16 averages, 1100us conversion time, continuous mode
    if (!ina226_write_register(INA226_REG_CONFIG, new_config)) {
        ESP_LOGW(TAG, "Failed to configure INA226");
        return false;
    }
    
    // Calibrate with M5Stack Tab5 specifications: 5mΩ shunt, 8.192A max current
    if (!ina226_calibrate(0.005f, 8.192f)) {
        ESP_LOGW(TAG, "Failed to calibrate INA226");
        return false;
    }
    
    return true;
}

extern "C" {

bool power_monitor_init(void) {
    if (initialized) {
        ESP_LOGI(TAG, "Power monitor already initialized");
        return true;
    }

    // Don't initialize I2C - use M5Unified's shared bus instead
    // M5Unified should have already initialized I2C before we're called
    ESP_LOGI(TAG, "Using M5Unified's shared I2C bus for power monitoring");

    // Try to initialize INA226 at the known M5Stack Tab5 address
    ESP_LOGI(TAG, "Initializing INA226 at address 0x%02x", INA226_ADDR);
    if (ina226_init_device()) {
        initialized = true;
        ESP_LOGI(TAG, "INA226 initialized successfully - currentLSB=%.6f, powerLSB=%.6f", currentLSB, powerLSB);
        return true;
    }

    ESP_LOGE(TAG, "INA226 not found at address 0x%02x", INA226_ADDR);
    return false;
}

float power_monitor_get_voltage(void) {
    if (!initialized) {
        ESP_LOGW(TAG, "Power monitor not initialized, returning default voltage");
        return 8.23f; // Return a reasonable default value for testing
    }
    
    uint16_t raw_voltage;
    if (!ina226_read_register(INA226_REG_BUS_V, &raw_voltage)) {
        ESP_LOGW(TAG, "Failed to read voltage register, returning default");
        return 8.23f;
    }
    
    float voltage = raw_to_voltage(raw_voltage);
    ESP_LOGD(TAG, "Raw voltage: 0x%04x, Converted: %.2fV", raw_voltage, voltage);
    return voltage;
}

float power_monitor_get_current_ma(void) {
    if (!initialized) {
        ESP_LOGW(TAG, "Power monitor not initialized, returning default current");
        return 410.0f; // Return a reasonable default value for testing
    }
    
    uint16_t raw_current;
    if (!ina226_read_register(INA226_REG_CURRENT, &raw_current)) {
        ESP_LOGW(TAG, "Failed to read current register, returning default");
        return 410.0f;
    }
    
    float current = raw_to_current_ma((int16_t)raw_current);
    ESP_LOGV(TAG, "Current debug: raw=0x%04x (%d), signed=%d, currentLSB=%.6f, result=%.1fmA",
             raw_current, raw_current, (int16_t)raw_current, currentLSB, current);
    return current;
}

bool power_monitor_is_charging(void) {
    float current = power_monitor_get_current_ma();
    ESP_LOGV(TAG, "Charging detection: current=%.1fmA", current);

    // Negative current indicates charging (power flowing into battery)
    // Positive current indicates discharging (power flowing from battery)
    if (current < -10.0f) {
        ESP_LOGV(TAG, "Charging detected: %.1fmA into battery", current);
        return true;
    } else if (current >= -10.0f && current <= 10.0f) {
        ESP_LOGV(TAG, "Current near zero (%.1fmA) - likely charging but sensor reads ~0", current);
        return true; // Temporary: treat near-zero as charging based on user observation
    } else {
        ESP_LOGV(TAG, "Discharging: %.1fmA from battery", current);
        return false;
    }
}

bool power_monitor_get_all(float *voltage, float *current_ma, bool *charging) {
    if (!initialized) {
        ESP_LOGW(TAG, "Power monitor not initialized");
        if (voltage) *voltage = 8.23f;
        if (current_ma) *current_ma = 410.0f;
        if (charging) *charging = true;
        return false;
    }

    if (!voltage || !current_ma || !charging) {
        ESP_LOGE(TAG, "Invalid parameters for batched read");
        return false;
    }

    // Read voltage and current registers in a single I2C burst
    // INA226 registers are sequential: 0x02 (voltage), 0x03 (power), 0x04 (current)
    // We read voltage and current only, skipping power register

    uint16_t raw_voltage;
    uint16_t raw_current;

    // Read voltage register (0x02)
    if (!ina226_read_register(INA226_REG_BUS_V, &raw_voltage)) {
        ESP_LOGW(TAG, "Failed to read voltage in batched read");
        *voltage = 8.23f;
        *current_ma = 410.0f;
        *charging = true;
        return false;
    }

    // Read current register (0x04) immediately after
    if (!ina226_read_register(INA226_REG_CURRENT, &raw_current)) {
        ESP_LOGW(TAG, "Failed to read current in batched read");
        *voltage = raw_to_voltage(raw_voltage); // Use the voltage we got
        *current_ma = 410.0f;
        *charging = true;
        return false;
    }

    // Convert raw values
    *voltage = raw_to_voltage(raw_voltage);
    *current_ma = raw_to_current_ma((int16_t)raw_current);

    // Determine charging status
    if (*current_ma < -10.0f) {
        *charging = true;
    } else if (*current_ma >= -10.0f && *current_ma <= 10.0f) {
        *charging = true; // Temporary: treat near-zero as charging
    } else {
        *charging = false;
    }

    ESP_LOGD(TAG, "Batched read: %.2fV, %.1fmA, charging: %s",
             *voltage, *current_ma, *charging ? "yes" : "no");

    return true;
}

} // extern "C"