#include "M5Unified.h"

extern "C" {
#include "hal.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_cache.h"
#include "esp_memory_utils.h"
#include "lvgl.h"
}

// Global display and touch handles exported for C code
extern "C" {
lv_display_t *lvDisp = NULL;
lv_indev_t *lvTouchpad = NULL;
}

extern "C" {

void hal_init(void)
{
    ESP_LOGI("HAL", "Initializing LVGL and creating display with M5Unified backend");

    // Initialize LVGL first
    lv_init();

    // Set up LVGL tick for timers and animations
    lv_tick_set_cb([]() {
        return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    });

    ESP_LOGI("HAL", "LVGL initialized with tick handler");

    // Set M5 display to landscape orientation first
    M5.Display.setRotation(3);  // Rotate to landscape mode (180 degrees from rotation 1)

    // Now get the correct landscape dimensions
    uint32_t width = M5.Display.width();
    uint32_t height = M5.Display.height();

    ESP_LOGI("HAL", "M5 Display dimensions (landscape): %dx%d", (int)width, (int)height);

    lvDisp = lv_display_create(width, height);
    if (!lvDisp) {
        ESP_LOGE("HAL", "Failed to create LVGL display");
        return;
    }

    // Implement double buffering for smooth transitions and reduced visual artifacts
    // Use 1/8 screen buffer to improve cache alignment and reduce corruption
    size_t buffer_pixels = (width * height) / 8;  // 1/8 screen buffer for better cache boundary alignment
    size_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);

    // Ensure buffer size is aligned to ESP32-P4 cache line size (64 bytes)
    size_t cache_aligned_bytes = (buffer_bytes + 63) & ~63;

    // Allocate first buffer with 64-byte alignment for ESP32-P4 DMA optimization
    // Use cache-aligned size for better DMA performance
    void* buf1 = heap_caps_aligned_alloc(64, cache_aligned_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    void* buf2 = NULL;

    if (buf1) {
        // Try to allocate second buffer for double buffering
        buf2 = heap_caps_aligned_alloc(64, cache_aligned_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);

        if (!buf2) {
            ESP_LOGW("HAL", "Failed to allocate second buffer in PSRAM, trying internal RAM");
            buf2 = heap_caps_aligned_alloc(64, cache_aligned_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
        }
    }

    if (!buf1) {
        ESP_LOGW("HAL", "Failed to allocate LVGL buffers in PSRAM, trying smaller single buffer in internal RAM");
        buffer_pixels = (width * height) / 16;  // Smaller buffer for internal RAM
        buffer_bytes = buffer_pixels * sizeof(lv_color_t);
        buf1 = heap_caps_aligned_alloc(64, buffer_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    }

    if (!buf1) {
        ESP_LOGE("HAL", "Failed to allocate LVGL buffers");
        lv_display_delete(lvDisp);
        lvDisp = NULL;
        return;
    }

    if (buf2) {
        ESP_LOGI("HAL", "LVGL double buffers allocated: %d bytes each (%d pixels, %dx%d lines)",
                 (int)buffer_bytes, (int)buffer_pixels, (int)width, (int)(buffer_pixels / width));
    } else {
        ESP_LOGI("HAL", "LVGL single buffer allocated: %d bytes (%d pixels, %dx%d lines)",
                 (int)buffer_bytes, (int)buffer_pixels, (int)width, (int)(buffer_pixels / width));
    }

    // Use partial rendering mode with double buffering for smooth transitions
    // Note: Not setting explicit color format - let LVGL use default that works with M5GFX
    lv_display_set_buffers(lvDisp, buf1, buf2, cache_aligned_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set up flush callback to render via M5GFX with proper DMA synchronization
    lv_display_set_flush_cb(lvDisp, [](lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
        // Calculate area dimensions
        int32_t w = area->x2 - area->x1 + 1;
        int32_t h = area->y2 - area->y1 + 1;
        int32_t total_pixels = w * h;

        ESP_LOGD("HAL", "Flushing area: (%d,%d) to (%d,%d), size: %dx%d (%d pixels)",
                 (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2, (int)w, (int)h, (int)total_pixels);

        // Validate flush parameters and area bounds
        if (!px_map || total_pixels <= 0 || area->x1 < 0 || area->y1 < 0 ||
            area->x2 >= (int32_t)width || area->y2 >= (int32_t)height) {
            ESP_LOGE("HAL", "Invalid flush parameters: px_map=%p, pixels=%d, area=(%d,%d)-(%d,%d)",
                     px_map, (int)total_pixels, (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2);
            lv_display_flush_ready(disp);
            return;
        }

        // Calculate buffer size for cache operations
        size_t pixel_data_size = total_pixels * sizeof(lv_color_t);

        // ESP32-P4 Cache coherency: Flush cache before DMA write
        if (esp_ptr_external_ram(px_map)) {
            esp_cache_msync(px_map, pixel_data_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        }

        // Use LVGL's built-in RGB565 byte swap function for better DMA compatibility
        // This is more efficient and DMA-safe than manual byte swapping
        lv_draw_sw_rgb565_swap((lv_color_t*)px_map, total_pixels);

        // Additional cache flush after byte swapping to ensure DMA sees correct data
        if (esp_ptr_external_ram(px_map)) {
            esp_cache_msync(px_map, pixel_data_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        }

        // Use M5GFX writePixelsDMA with enhanced synchronization
        M5.Display.startWrite();
        M5.Display.setAddrWindow(area->x1, area->y1, w, h);

        // Perform DMA transfer - writePixelsDMA doesn't return success status
        M5.Display.writePixelsDMA((uint16_t*)px_map, total_pixels);

        // CRITICAL: Wait for DMA transfer to complete before signaling LVGL
        M5.Display.waitDMA();

        // Ensure all DMA operations are complete before proceeding
        if (esp_ptr_external_ram(px_map)) {
            esp_cache_msync(px_map, pixel_data_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        }

        M5.Display.endWrite();

        // Tell LVGL the flush is complete only after transfer finishes
        lv_display_flush_ready(disp);
    });

    // Display rotation handled by M5.Display.setRotation() above

    ESP_LOGI("HAL", "LVGL display created successfully with M5Unified backend");
}

void hal_touchpad_init(void)
{
    ESP_LOGI("HAL", "Creating LVGL touchpad with M5Unified backend");

    if (!lvDisp) {
        ESP_LOGE("HAL", "Cannot create touchpad without display");
        return;
    }

    lvTouchpad = lv_indev_create();
    if (!lvTouchpad) {
        ESP_LOGE("HAL", "Failed to create LVGL touchpad");
        return;
    }

    lv_indev_set_type(lvTouchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(lvTouchpad, [](lv_indev_t * indev, lv_indev_data_t * data) {
        static uint32_t last_touch_time = 0;
        static bool last_touch_state = false;
        const uint32_t DEBOUNCE_MS = 50;  // 50ms debounce time

        // Update M5 touch state
        M5.update();

        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
        bool current_touch = (M5.Touch.getCount() > 0);

        // Implement touch debouncing to prevent rapid events during screen transitions
        if (current_touch != last_touch_state) {
            if (current_time - last_touch_time < DEBOUNCE_MS) {
                // Use previous state during debounce period
                current_touch = last_touch_state;
            } else {
                // Update debounce tracking
                last_touch_time = current_time;
                last_touch_state = current_touch;
            }
        }

        if (current_touch) {
            // Get first touch point
            auto touch = M5.Touch.getDetail(0);
            data->state = LV_INDEV_STATE_PRESSED;
            data->point.x = touch.x;
            data->point.y = touch.y;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
            data->point.x = 0;
            data->point.y = 0;
        }
    });
    lv_indev_set_display(lvTouchpad, lvDisp);

    ESP_LOGI("HAL", "LVGL touchpad created successfully with M5Unified backend and touch debouncing");
}

}