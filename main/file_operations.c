#include "file_operations.h"
#include "sd_manager.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

static const char *TAG = "FILE_OPS";

// Buffer size for copy operations (512 bytes like bmorcelli)
#define COPY_BUFFER_SIZE 512
#define MAX_PATH_LENGTH 512  // Increased to prevent truncation warnings

// Static buffer for copy/paste operations
static char clipboard_path[MAX_PATH_LENGTH] = {0};
static bool clipboard_is_cut = false;
static bool clipboard_has_content = false;

esp_err_t file_ops_create_directory(const char *path) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    if (mkdir(full_path, 0755) == 0) {
        ESP_LOGI(TAG, "Directory created: %s", full_path);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to create directory: %s (errno: %d)", full_path, errno);
        return ESP_FAIL;
    }
}

esp_err_t file_ops_delete_file(const char *path) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    if (remove(full_path) == 0) {
        ESP_LOGI(TAG, "File deleted: %s", full_path);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to delete file: %s (errno: %d)", full_path, errno);
        return ESP_FAIL;
    }
}

static esp_err_t delete_directory_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        return ESP_FAIL;
    }
    
    struct dirent *entry;
    char item_path[MAX_PATH_LENGTH];
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(item_path, sizeof(item_path), "%s/%s", path, entry->d_name);
        
        struct stat file_stat;
        if (stat(item_path, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                delete_directory_recursive(item_path);
            } else {
                remove(item_path);
            }
        }
    }
    
    closedir(dir);
    return (rmdir(path) == 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t file_ops_delete_directory(const char *path) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    esp_err_t ret = delete_directory_recursive(full_path);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Directory deleted: %s", full_path);
    } else {
        ESP_LOGE(TAG, "Failed to delete directory: %s", full_path);
    }
    return ret;
}

esp_err_t file_ops_rename(const char *old_path, const char *new_path) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    char old_full[MAX_PATH_LENGTH];
    char new_full[MAX_PATH_LENGTH];
    snprintf(old_full, sizeof(old_full), "%s%s", SD_MOUNT_POINT, old_path);
    snprintf(new_full, sizeof(new_full), "%s%s", SD_MOUNT_POINT, new_path);
    
    if (rename(old_full, new_full) == 0) {
        ESP_LOGI(TAG, "Renamed: %s -> %s", old_full, new_full);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to rename: %s -> %s (errno: %d)", old_full, new_full, errno);
        return ESP_FAIL;
    }
}

esp_err_t file_ops_copy_file(const char *src_path, const char *dst_path) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    char src_full[MAX_PATH_LENGTH];
    char dst_full[MAX_PATH_LENGTH];
    snprintf(src_full, sizeof(src_full), "%s%s", SD_MOUNT_POINT, src_path);
    snprintf(dst_full, sizeof(dst_full), "%s%s", SD_MOUNT_POINT, dst_path);
    
    FILE *src = fopen(src_full, "rb");
    if (!src) {
        ESP_LOGE(TAG, "Failed to open source file: %s", src_full);
        return ESP_FAIL;
    }
    
    FILE *dst = fopen(dst_full, "wb");
    if (!dst) {
        fclose(src);
        ESP_LOGE(TAG, "Failed to open destination file: %s", dst_full);
        return ESP_FAIL;
    }
    
    // Copy file in 512-byte chunks (like bmorcelli)
    uint8_t buffer[COPY_BUFFER_SIZE];
    size_t bytes_read;
    size_t total_copied = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_read, dst);
        if (bytes_written != bytes_read) {
            ESP_LOGE(TAG, "Write error during copy");
            fclose(src);
            fclose(dst);
            remove(dst_full);
            return ESP_FAIL;
        }
        total_copied += bytes_written;
    }
    
    fclose(src);
    fclose(dst);
    
    ESP_LOGI(TAG, "Copied %zu bytes: %s -> %s", total_copied, src_path, dst_path);
    return ESP_OK;
}

static esp_err_t copy_directory_recursive(const char *src_path, const char *dst_path) {
    // Create destination directory
    if (mkdir(dst_path, 0755) != 0 && errno != EEXIST) {
        return ESP_FAIL;
    }
    
    DIR *dir = opendir(src_path);
    if (!dir) {
        return ESP_FAIL;
    }
    
    struct dirent *entry;
    char src_item[MAX_PATH_LENGTH];
    char dst_item[MAX_PATH_LENGTH];
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(src_item, sizeof(src_item), "%s/%s", src_path, entry->d_name);
        snprintf(dst_item, sizeof(dst_item), "%s/%s", dst_path, entry->d_name);
        
        struct stat file_stat;
        if (stat(src_item, &file_stat) == 0) {
            if (S_ISDIR(file_stat.st_mode)) {
                copy_directory_recursive(src_item, dst_item);
            } else {
                // Remove SD_MOUNT_POINT prefix for file_ops_copy_file
                const char *src_rel = src_item + strlen(SD_MOUNT_POINT);
                const char *dst_rel = dst_item + strlen(SD_MOUNT_POINT);
                file_ops_copy_file(src_rel, dst_rel);
            }
        }
    }
    
    closedir(dir);
    return ESP_OK;
}

esp_err_t file_ops_copy_directory(const char *src_path, const char *dst_path) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    char src_full[MAX_PATH_LENGTH];
    char dst_full[MAX_PATH_LENGTH];
    snprintf(src_full, sizeof(src_full), "%s%s", SD_MOUNT_POINT, src_path);
    snprintf(dst_full, sizeof(dst_full), "%s%s", SD_MOUNT_POINT, dst_path);
    
    esp_err_t ret = copy_directory_recursive(src_full, dst_full);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Directory copied: %s -> %s", src_path, dst_path);
    } else {
        ESP_LOGE(TAG, "Failed to copy directory: %s -> %s", src_path, dst_path);
    }
    return ret;
}

esp_err_t file_ops_copy_to_clipboard(const char *path, bool cut) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    strncpy(clipboard_path, path, sizeof(clipboard_path) - 1);
    clipboard_path[sizeof(clipboard_path) - 1] = '\0';
    clipboard_is_cut = cut;
    clipboard_has_content = true;
    
    ESP_LOGI(TAG, "%s copied to clipboard: %s", cut ? "Cut" : "Copy", path);
    return ESP_OK;
}

esp_err_t file_ops_paste_from_clipboard(const char *dst_dir) {
    if (!clipboard_has_content) {
        ESP_LOGE(TAG, "Clipboard is empty");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Extract filename from clipboard path
    const char *filename = strrchr(clipboard_path, '/');
    if (!filename) {
        filename = clipboard_path;
    } else {
        filename++; // Skip the '/'
    }
    
    // Create destination path - ensure no overflow
    char dst_path[MAX_PATH_LENGTH];
    size_t dst_dir_len = strlen(dst_dir);
    size_t filename_len = strlen(filename);
    if (dst_dir_len + filename_len + 2 >= MAX_PATH_LENGTH) {
        ESP_LOGE(TAG, "Destination path too long");
        return ESP_FAIL;
    }
    // Suppress format-truncation warning as we've already checked the lengths
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, filename);
    #pragma GCC diagnostic pop
    
    // Check if source is a directory - ensure no overflow
    char src_full[MAX_PATH_LENGTH];
    size_t mount_len = strlen(SD_MOUNT_POINT);
    size_t clipboard_len = strlen(clipboard_path);
    if (mount_len + clipboard_len + 1 >= MAX_PATH_LENGTH) {
        ESP_LOGE(TAG, "Source path too long");
        return ESP_FAIL;
    }
    snprintf(src_full, sizeof(src_full), "%s%s", SD_MOUNT_POINT, clipboard_path);
    
    struct stat file_stat;
    if (stat(src_full, &file_stat) != 0) {
        ESP_LOGE(TAG, "Source file/directory not found: %s", clipboard_path);
        return ESP_FAIL;
    }
    
    esp_err_t ret;
    if (S_ISDIR(file_stat.st_mode)) {
        ret = file_ops_copy_directory(clipboard_path, dst_path);
    } else {
        ret = file_ops_copy_file(clipboard_path, dst_path);
    }
    
    if (ret == ESP_OK && clipboard_is_cut) {
        // Delete source after successful copy
        if (S_ISDIR(file_stat.st_mode)) {
            file_ops_delete_directory(clipboard_path);
        } else {
            file_ops_delete_file(clipboard_path);
        }
        clipboard_has_content = false;
    }
    
    return ret;
}

bool file_ops_clipboard_has_content(void) {
    return clipboard_has_content;
}

const char* file_ops_get_clipboard_path(void) {
    return clipboard_has_content ? clipboard_path : NULL;
}

esp_err_t file_ops_get_file_info(const char *path, file_info_t *info) {
    if (!sd_manager_is_mounted() || !info) {
        return ESP_ERR_INVALID_STATE;
    }
    
    char full_path[MAX_PATH_LENGTH];
    snprintf(full_path, sizeof(full_path), "%s%s", SD_MOUNT_POINT, path);
    
    struct stat file_stat;
    if (stat(full_path, &file_stat) != 0) {
        return ESP_FAIL;
    }
    
    // Extract filename
    const char *filename = strrchr(path, '/');
    if (!filename) {
        filename = path;
    } else {
        filename++;
    }
    
    strncpy(info->name, filename, sizeof(info->name) - 1);
    info->name[sizeof(info->name) - 1] = '\0';
    info->size = file_stat.st_size;
    info->is_directory = S_ISDIR(file_stat.st_mode);
    info->modified_time = file_stat.st_mtime;
    
    // Determine file type by extension
    const char *ext = strrchr(filename, '.');
    if (!ext || info->is_directory) {
        info->type = info->is_directory ? FILE_TYPE_DIRECTORY : FILE_TYPE_OTHER;
    } else {
        ext++; // Skip the dot
        if (strcasecmp(ext, "bin") == 0) {
            info->type = FILE_TYPE_BINARY;
        } else if (strcasecmp(ext, "txt") == 0 || strcasecmp(ext, "cfg") == 0 || 
                   strcasecmp(ext, "conf") == 0 || strcasecmp(ext, "json") == 0) {
            info->type = FILE_TYPE_TEXT;
        } else if (strcasecmp(ext, "py") == 0) {
            info->type = FILE_TYPE_PYTHON;
        } else if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "css") == 0 || 
                   strcasecmp(ext, "js") == 0) {
            info->type = FILE_TYPE_WEB;
        } else if (strcasecmp(ext, "jpg") == 0 || strcasecmp(ext, "jpeg") == 0 || 
                   strcasecmp(ext, "png") == 0 || strcasecmp(ext, "bmp") == 0) {
            info->type = FILE_TYPE_IMAGE;
        } else {
            info->type = FILE_TYPE_OTHER;
        }
    }
    
    return ESP_OK;
}