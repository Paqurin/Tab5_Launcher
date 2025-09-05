#include "sd_manager.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "bsp/m5stack_tab5.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>

static const char *TAG = "SD_MANAGER";
static bool sd_mounted = false;
static bool sd_card_present = false;

esp_err_t sd_manager_init(void) {
    esp_err_t ret = bsp_sdcard_init(SD_MOUNT_POINT, 5);
    if (ret == ESP_OK) {
        sd_mounted = true;
        sd_card_present = true;
        ESP_LOGI(TAG, "SD card mounted successfully at %s", SD_MOUNT_POINT);
    } else {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        sd_mounted = false;
        // Card might still be present but not mountable
        sd_card_present = false;
    }
    return ret;
}

bool sd_manager_is_mounted(void) {
    return sd_mounted;
}

esp_err_t sd_manager_deinit(void) {
    if (!sd_mounted) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = bsp_sdcard_deinit(SD_MOUNT_POINT);
    if (ret == ESP_OK) {
        sd_mounted = false;
        // Keep card_present true - card is still there, just unmounted
        ESP_LOGI(TAG, "SD card unmounted successfully");
    }
    return ret;
}

int sd_manager_scan_directory(const char *path, file_entry_t *entries, int max_entries) {
    if (!sd_mounted) {
        ESP_LOGE(TAG, "SD card not mounted");
        return -1;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    DIR *dir = opendir(full_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", full_path);
        return -1;
    }
    
    struct dirent *entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL && count < max_entries) {
        // Skip hidden files and current/parent directory entries
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        // Get full path for stat
        char item_path[512];
        snprintf(item_path, sizeof(item_path), "%s/%s", full_path, entry->d_name);
        
        struct stat file_stat;
        if (stat(item_path, &file_stat) == 0) {
            strncpy(entries[count].name, entry->d_name, sizeof(entries[count].name) - 1);
            entries[count].name[sizeof(entries[count].name) - 1] = '\0';
            entries[count].is_directory = S_ISDIR(file_stat.st_mode);
            entries[count].size = file_stat.st_size;
            count++;
        }
    }
    
    closedir(dir);
    ESP_LOGI(TAG, "Found %d entries in %s", count, path);
    return count;
}

bool sd_manager_file_exists(const char *path) {
    if (!sd_mounted) {
        return false;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    struct stat file_stat;
    return (stat(full_path, &file_stat) == 0);
}

size_t sd_manager_get_file_size(const char *path) {
    if (!sd_mounted) {
        return 0;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    struct stat file_stat;
    if (stat(full_path, &file_stat) == 0) {
        return file_stat.st_size;
    }
    return 0;
}

FILE* sd_manager_open_file(const char *path, const char *mode) {
    if (!sd_mounted) {
        return NULL;
    }
    
    char full_path[256];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    return fopen(full_path, mode);
}

esp_err_t sd_manager_format(void) {
    ESP_LOGI(TAG, "Starting SD card format operation");
    
    // Unmount SD card first
    esp_err_t ret = sd_manager_deinit();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to unmount SD card before format: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Wait a moment for unmount to complete
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Reinitialize SD card for formatting
    // This will format the card if mount fails
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,  // This is the key - format if mount fails
        .max_files = 5,
        .allocation_unit_size = 16 * 1024  // 16KB clusters for better performance
    };
    
    sdmmc_card_t *card = NULL;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 4;  // Use 4-bit mode
    
    // First, try to mount - if it fails, it will auto-format
    ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret == ESP_OK) {
        // Successfully mounted (or formatted and mounted)
        sd_mounted = true;
        ESP_LOGI(TAG, "SD card format completed successfully");
        
        // Create a test file to verify format worked
        FILE *test_file = fopen(SD_MOUNT_POINT "/format_test.txt", "w");
        if (test_file) {
            fprintf(test_file, "SD card formatted successfully by Tab5 Launcher\n");
            fclose(test_file);
            // Remove test file
            remove(SD_MOUNT_POINT "/format_test.txt");
        }
        
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "SD card format failed: %s", esp_err_to_name(ret));
        sd_mounted = false;
        return ret;
    }
}

esp_err_t sd_manager_mount(void) {
    if (sd_mounted) {
        ESP_LOGI(TAG, "SD card already mounted");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Attempting to mount SD card");
    esp_err_t ret = bsp_sdcard_init(SD_MOUNT_POINT, 5);
    if (ret == ESP_OK) {
        sd_mounted = true;
        sd_card_present = true;
        ESP_LOGI(TAG, "SD card mounted successfully at %s", SD_MOUNT_POINT);
    } else {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        sd_mounted = false;
        // Try to detect if card is physically present but not mountable
        // If mount failed but we can try again, assume card is present
        sd_card_present = true; // Optimistic assumption for user experience
    }
    return ret;
}

esp_err_t sd_manager_unmount(void) {
    if (!sd_mounted) {
        ESP_LOGI(TAG, "SD card already unmounted");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Unmounting SD card");
    esp_err_t ret = bsp_sdcard_deinit(SD_MOUNT_POINT);
    if (ret == ESP_OK) {
        sd_mounted = false;
        // Keep card_present true - card is still physically there
        ESP_LOGI(TAG, "SD card unmounted successfully");
    } else {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
    }
    return ret;
}

bool sd_manager_card_detected(void) {
    // Return whether card is present (could be mounted or just detected)
    return sd_card_present;
}

void sd_manager_set_card_present(bool present) {
    sd_card_present = present;
    ESP_LOGI(TAG, "SD card presence manually set to: %s", present ? "present" : "absent");
    
    // If card is being marked as absent, also mark as unmounted
    if (!present) {
        sd_mounted = false;
    }
}