#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <stdbool.h>

/**
 * @brief Initialize the INA226 power monitor
 * @return true on success, false on failure
 */
bool power_monitor_init(void);

/**
 * @brief Read bus voltage from INA226
 * @return Voltage in volts, or 0.0 on error
 */
float power_monitor_get_voltage(void);

/**
 * @brief Read bus current from INA226
 * @return Current in milliamps, or 0.0 on error
 */
float power_monitor_get_current_ma(void);

/**
 * @brief Check if device is charging (current > 0)
 * @return true if charging, false if discharging or error
 */
bool power_monitor_is_charging(void);

/**
 * @brief Read all power measurements in a single batched I2C transaction
 * This minimizes I2C bus activity and reduces interference with charging circuit
 * @param voltage Output: voltage in volts
 * @param current_ma Output: current in milliamps
 * @param charging Output: true if charging
 * @return true on success, false on error
 */
bool power_monitor_get_all(float *voltage, float *current_ma, bool *charging);

#endif // POWER_MONITOR_H