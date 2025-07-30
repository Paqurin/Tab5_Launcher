#ifndef FIRMWARE_LOADER_H
#define FIRMWARE_LOADER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_FIRMWARE_NAME_LEN 64
#define MAX_FIRMWARE_PATH_LEN 256

typedef struct {
    char filename[MAX_FIRMWARE_NAME_LEN];
    char full_path[MAX_FIRMWARE_PATH_LEN];
    size_t size;
} firmware_info_t;

/**
 * @brief Progress callback function type
 * @param bytes_written Number of bytes written so far
 * @param total_bytes Total number of bytes to write
 * @param step_description Current step description
 */
typedef void (*firmware_progress_callback_t)(size_t bytes_written, size_t total_bytes, const char *step_description);

/**
 * @brief Initialize firmware loader
 * @return ESP_OK on success
 */
esp_err_t firmware_loader_init(void);

/**
 * @brief Initialize boot manager and NVS
 * @return ESP_OK on success
 */
esp_err_t firmware_loader_init_boot_manager(void);

/**
 * @brief Flash firmware from SD card
 * @param firmware_path Path to firmware file on SD card
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t firmware_loader_flash_from_sd(const char *firmware_path);

/**
 * @brief Flash firmware from SD card with progress callback
 * @param firmware_path Path to firmware file on SD card
 * @param progress_callback Callback function for progress updates
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t firmware_loader_flash_from_sd_with_progress(const char *firmware_path, firmware_progress_callback_t progress_callback);

/**
 * @brief Check if a firmware is installed and ready to boot
 * @return true if firmware is ready, false otherwise
 */
bool firmware_loader_is_firmware_ready(void);

/**
 * @brief Scan directory for firmware files
 * @param directory Directory to scan
 * @param firmware_list Array to store firmware info
 * @param max_count Maximum number of firmware files to return
 * @return Number of firmware files found
 */
int firmware_loader_scan_firmware_files(const char *directory, firmware_info_t *firmware_list, int max_count);

/**
 * @brief Boot firmware once without changing default boot partition
 * This uses OTA rollback mechanism to ensure launcher remains default
 * @return ESP_OK (never returns)
 */
esp_err_t firmware_loader_boot_firmware_once(void);

/**
 * @brief Restart device to boot new firmware (legacy function name)
 * This is an alias for firmware_loader_boot_firmware_once()
 * @return ESP_OK (never returns)
 */
esp_err_t firmware_loader_restart_to_new_firmware(void);

#endif // FIRMWARE_LOADER_H