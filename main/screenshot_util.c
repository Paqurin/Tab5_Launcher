#include "screenshot_util.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "sd_manager.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <sys/stat.h>

static const char *TAG = "SCREENSHOT";

// BMP file header structures
#pragma pack(push, 1)
typedef struct {
    uint16_t type;      // BM
    uint32_t size;      // File size
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;    // Offset to image data
} bmp_file_header_t;

typedef struct {
    uint32_t size;          // Header size
    int32_t width;          // Image width
    int32_t height;         // Image height
    uint16_t planes;        // Color planes
    uint16_t bits_per_pixel; // Bits per pixel
    uint32_t compression;   // Compression type
    uint32_t image_size;    // Image size
    int32_t x_pixels_per_m; // X pixels per meter
    int32_t y_pixels_per_m; // Y pixels per meter
    uint32_t colors_used;   // Colors used
    uint32_t colors_important; // Important colors
} bmp_info_header_t;
#pragma pack(pop)

esp_err_t screenshot_take_and_save(void) {
    if (!sd_manager_is_mounted()) {
        ESP_LOGE(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    // Get the active screen
    lv_obj_t *screen = lv_screen_active();
    if (!screen) {
        ESP_LOGE(TAG, "No active screen");
        return ESP_ERR_INVALID_STATE;
    }

    // Get display dimensions
    lv_display_t *disp = lv_obj_get_display(screen);
    if (!disp) {
        ESP_LOGE(TAG, "No display found");
        return ESP_ERR_INVALID_STATE;
    }

    int32_t width = lv_display_get_horizontal_resolution(disp);
    int32_t height = lv_display_get_vertical_resolution(disp);

    ESP_LOGI(TAG, "Taking screenshot: %dx%d", (int)width, (int)height);

    // Calculate sizes
    uint32_t row_size = ((width * 3 + 3) / 4) * 4; // BMP rows must be 4-byte aligned
    uint32_t image_size = row_size * height;
    uint32_t file_size = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t) + image_size;

    // Allocate buffer for one row (to save memory)
    uint8_t *row_buffer = heap_caps_malloc(row_size, MALLOC_CAP_8BIT);
    if (!row_buffer) {
        ESP_LOGE(TAG, "Failed to allocate row buffer");
        return ESP_ERR_NO_MEM;
    }

    // Generate simple sequential filename (since no RTC/time sync)
    static uint32_t screenshot_counter = 0;
    screenshot_counter++;

    char filename[64];
    snprintf(filename, sizeof(filename), "/sdcard/screenshot_%05" PRIu32 ".bmp", screenshot_counter);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to create file: %s", filename);
        free(row_buffer);
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Saving screenshot to: %s", filename);

    // Write BMP file header
    bmp_file_header_t file_header = {
        .type = 0x4D42, // "BM"
        .size = file_size,
        .reserved1 = 0,
        .reserved2 = 0,
        .offset = sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t)
    };
    fwrite(&file_header, sizeof(file_header), 1, file);

    // Write BMP info header
    bmp_info_header_t info_header = {
        .size = sizeof(bmp_info_header_t),
        .width = width,
        .height = height,
        .planes = 1,
        .bits_per_pixel = 24,
        .compression = 0,
        .image_size = image_size,
        .x_pixels_per_m = 2835, // ~72 DPI
        .y_pixels_per_m = 2835,
        .colors_used = 0,
        .colors_important = 0
    };
    fwrite(&info_header, sizeof(info_header), 1, file);

    // Force LVGL to render current state to ensure display is up to date
    lv_refr_now(disp);

    // Try to access the BSP display driver directly
    extern lv_display_t *lvDisp;  // From hal.h

    // Get the draw buffer after forcing a refresh
    lv_draw_buf_t *draw_buf = lv_display_get_buf_active(lvDisp ? lvDisp : disp);

    if (!draw_buf || !draw_buf->data) {
        ESP_LOGW(TAG, "No active draw buffer, capturing black screen");

        // Create a black screen as fallback
        for (int32_t y = height - 1; y >= 0; y--) {
            uint8_t *row_ptr = row_buffer;

            for (int32_t x = 0; x < width; x++) {
                *row_ptr++ = 0;  // B
                *row_ptr++ = 0;  // G
                *row_ptr++ = 0;  // R
            }

            // Pad row to 4-byte boundary
            while ((row_ptr - row_buffer) % 4 != 0) {
                *row_ptr++ = 0;
            }

            fwrite(row_buffer, row_ptr - row_buffer, 1, file);
        }
    } else {
        ESP_LOGI(TAG, "Found draw buffer at %p", draw_buf->data);

        // Assume the buffer is in RGB565 format
        uint16_t *fb_data = (uint16_t *)draw_buf->data;

        // Read pixel data row by row (BMP is stored bottom-to-top)
        for (int32_t y = height - 1; y >= 0; y--) {
            uint8_t *row_ptr = row_buffer;

            for (int32_t x = 0; x < width; x++) {
                // Get pixel from framebuffer (RGB565)
                uint16_t pixel565 = fb_data[y * width + x];

                // Convert RGB565 to RGB888
                uint8_t r = ((pixel565 >> 11) & 0x1F) << 3;  // 5 bits -> 8 bits
                uint8_t g = ((pixel565 >> 5) & 0x3F) << 2;   // 6 bits -> 8 bits
                uint8_t b = (pixel565 & 0x1F) << 3;          // 5 bits -> 8 bits

                // Write in BGR order (BMP format)
                *row_ptr++ = b;  // B
                *row_ptr++ = g;  // G
                *row_ptr++ = r;  // R
            }

            // Pad row to 4-byte boundary with zeros
            while ((row_ptr - row_buffer) % 4 != 0) {
                *row_ptr++ = 0;
            }

            fwrite(row_buffer, row_ptr - row_buffer, 1, file);
        }
    }

    fclose(file);
    free(row_buffer);

    ESP_LOGI(TAG, "Screenshot saved successfully: %s", filename);
    return ESP_OK;
}