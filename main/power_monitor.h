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

#endif // POWER_MONITOR_H