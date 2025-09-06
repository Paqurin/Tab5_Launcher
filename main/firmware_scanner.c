#include "firmware_loader.h"
#include "sd_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

static const char *TAG = "FIRMWARE_SCANNER";

static bool is_firmware_file(const char *filename) {
    size_t len = strlen(filename);
    if (len < 4) return false;
    return (strcmp(&filename[len-4], ".bin") == 0);
}

int firmware_loader_scan_firmware_files(const char *directory, firmware_info_t *firmware_list, int max_count) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGW(TAG, "SD card not mounted");
        return 0;
    }
    
    file_entry_t entries[32];
    int entry_count = sd_manager_scan_directory(directory, entries, 32, false); // Don't show hidden files for firmware
    int firmware_count = 0;
    
    for (int i = 0; i < entry_count && firmware_count < max_count; i++) {
        if (!entries[i].is_directory && is_firmware_file(entries[i].name)) {
            // Copy filename with bounds checking
            strncpy(firmware_list[firmware_count].filename, entries[i].name, MAX_FIRMWARE_NAME_LEN - 1);
            firmware_list[firmware_count].filename[MAX_FIRMWARE_NAME_LEN - 1] = '\0';
            
            // Build full path with explicit bounds checking
            size_t dir_len = strlen(directory);
            size_t name_len = strlen(entries[i].name);
            
            // Check if the combined path will fit (directory + "/" + filename + null terminator)
            if (dir_len + 1 + name_len + 1 <= MAX_FIRMWARE_PATH_LEN) {
                // Use explicit string operations to avoid compiler warnings
                strcpy(firmware_list[firmware_count].full_path, directory);
                strcat(firmware_list[firmware_count].full_path, "/");
                strcat(firmware_list[firmware_count].full_path, entries[i].name);
            } else {
                ESP_LOGW(TAG, "Path too long, skipping: %.*s/%.*s", 
                        (int)dir_len, directory, (int)name_len, entries[i].name);
                continue;
            }
            
            // Get file size - build SD card path with explicit bounds checking
            size_t sd_prefix_len = 8; // "/sdcard" length
            if (sd_prefix_len + dir_len + 1 + name_len + 1 <= 512) {
                char full_sd_path[512];
                strcpy(full_sd_path, "/sdcard");
                strcat(full_sd_path, directory);
                strcat(full_sd_path, "/");
                strcat(full_sd_path, entries[i].name);
                
                struct stat file_stat;
                if (stat(full_sd_path, &file_stat) == 0) {
                    firmware_list[firmware_count].size = file_stat.st_size;
                } else {
                    firmware_list[firmware_count].size = 0;
                }
            } else {
                ESP_LOGW(TAG, "SD path too long, setting size to 0: %.*s/%.*s", 
                        (int)dir_len, directory, (int)name_len, entries[i].name);
                firmware_list[firmware_count].size = 0;
            }
            
            firmware_count++;
        }
    }
    
    ESP_LOGI(TAG, "Found %d firmware files in %s", firmware_count, directory);
    return firmware_count;
}