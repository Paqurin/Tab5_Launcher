#include "sd_manager.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "bsp/m5stack_tab5.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "SD_MANAGER";
static bool sd_mounted = false;

esp_err_t sd_manager_init(void) {
    esp_err_t ret = bsp_sdcard_init(SD_MOUNT_POINT, 5);
    if (ret == ESP_OK) {
        sd_mounted = true;
        ESP_LOGI(TAG, "SD card mounted successfully at %s", SD_MOUNT_POINT);
    } else {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        sd_mounted = false;
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