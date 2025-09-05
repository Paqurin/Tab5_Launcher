#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "esp_err.h"
#include <stdio.h>
#include <stdbool.h>

#define SD_MOUNT_POINT "/sdcard"
#define MAX_FILENAME_LEN 64

typedef struct {
    char name[MAX_FILENAME_LEN];
    bool is_directory;
    size_t size;
} file_entry_t;

/**
 * @brief Initialize SD card manager
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_manager_init(void);

/**
 * @brief Check if SD card is mounted
 * @return true if mounted, false otherwise
 */
bool sd_manager_is_mounted(void);

/**
 * @brief Deinitialize SD card manager
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_manager_deinit(void);

/**
 * @brief Scan directory and return file entries
 * @param path Directory path to scan (relative to SD root)
 * @param entries Array to store file entries
 * @param max_entries Maximum number of entries to return
 * @return Number of entries found, -1 on error
 */
int sd_manager_scan_directory(const char *path, file_entry_t *entries, int max_entries);

/**
 * @brief Check if file exists
 * @param path File path (relative to SD root)
 * @return true if file exists, false otherwise
 */
bool sd_manager_file_exists(const char *path);

/**
 * @brief Get file size
 * @param path File path (relative to SD root)
 * @return File size in bytes, 0 if file doesn't exist
 */
size_t sd_manager_get_file_size(const char *path);

/**
 * @brief Open file for reading/writing
 * @param path File path (relative to SD root)
 * @param mode File open mode ("r", "w", "rb", "wb", etc.)
 * @return File pointer on success, NULL on error
 */
FILE* sd_manager_open_file(const char *path, const char *mode);

/**
 * @brief Format SD card as FAT32
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_manager_format(void);

/**
 * @brief Mount SD card (for remounting after unmount)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_manager_mount(void);

/**
 * @brief Unmount SD card (for safe removal or troubleshooting)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sd_manager_unmount(void);

/**
 * @brief Check if SD card hardware is detected (regardless of mount status)
 * @return true if SD card is physically detected, false otherwise
 */
bool sd_manager_card_detected(void);

/**
 * @brief Set SD card presence status (for manual detection updates)
 * @param present true if card is present, false otherwise
 */
void sd_manager_set_card_present(bool present);

#endif // SD_MANAGER_H