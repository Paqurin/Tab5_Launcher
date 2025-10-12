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

    // FULL SCREEN DOUBLE BUFFERING: Allocate full screen buffers for DIRECT mode
    // This eliminates buffer overflow errors and provides smooth, artifact-free rendering
    // ESP32-P4 has 32MB PSRAM at 200MHz - plenty fast enough for full-screen double buffering
    size_t buffer_pixels = width * height;  // Full screen = 1280x720 = 921,600 pixels
    size_t buffer_bytes = buffer_pixels * sizeof(lv_color_t);  // RGB565 = 2 bytes per pixel = 1,843,200 bytes (~1.8MB)

    // Ensure buffer size is aligned to ESP32-P4 cache line size (64 bytes) with padding
    size_t cache_aligned_bytes = ((buffer_bytes + 127) & ~127);  // 128-byte alignment for safety

    ESP_LOGI("HAL", "Allocating full-screen double buffers (%.2fMB each) for DIRECT mode",
             cache_aligned_bytes / (1024.0f * 1024.0f));

    // Use SPIRAM for full-screen buffers
    // - 2 buffers for LVGL double buffering (buf1, buf2)
    // - 1 buffer for byte-swapped DMA transfer (dma_buf) to preserve LVGL buffer integrity
    // SPIRAM at 200MHz is fast enough for smooth rendering
    // Total: 5.4MB uses only ~17% of 32MB PSRAM
    void* buf1 = heap_caps_aligned_alloc(64, cache_aligned_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    void* buf2 = heap_caps_aligned_alloc(64, cache_aligned_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);
    void* dma_buf = heap_caps_aligned_alloc(64, cache_aligned_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA);

    if (!buf1 || !buf2 || !dma_buf) {
        ESP_LOGE("HAL", "Failed to allocate full-screen buffers (%.2fMB each required)",
                 cache_aligned_bytes / (1024.0f * 1024.0f));
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        if (dma_buf) free(dma_buf);
        lv_display_delete(lvDisp);
        lvDisp = NULL;
        return;
    }

    // Log memory usage
    size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t used_buffers = cache_aligned_bytes * 3;

    ESP_LOGI("HAL", "LVGL DIRECT mode buffers allocated successfully:");
    ESP_LOGI("HAL", "  - Buffer size: %.2fMB each (%.2fMB total for double buffering)",
             cache_aligned_bytes / (1024.0f * 1024.0f), used_buffers / (1024.0f * 1024.0f));
    ESP_LOGI("HAL", "  - Buffer dimensions: %dx%d pixels (%d total pixels)",
             (int)width, (int)height, (int)buffer_pixels);
    ESP_LOGI("HAL", "  - PSRAM usage: %.2fMB / %.2fMB (%.1f%% used, %.2fMB free)",
             (total_psram - free_psram) / (1024.0f * 1024.0f),
             total_psram / (1024.0f * 1024.0f),
             ((total_psram - free_psram) * 100.0f) / total_psram,
             free_psram / (1024.0f * 1024.0f));

    // DIRECT RENDERING MODE: Full screen, single frame, no chunking
    // This provides the best performance with full-screen buffers and eliminates buffer overflow errors
    // No partial rendering means no buffer overflow errors from large UI elements
    lv_display_set_buffers(lvDisp, buf1, buf2, cache_aligned_bytes, LV_DISPLAY_RENDER_MODE_DIRECT);

    ESP_LOGI("HAL", "LVGL display configured in DIRECT mode (full-screen rendering)");

    // Store buffer and display information in display user data
    struct display_info_t {
        size_t buffer_pixels;
        uint32_t width;
        uint32_t height;
        void* dma_buffer;  // Separate buffer for byte-swapped DMA transfer
    };
    static display_info_t* display_info = (display_info_t*)malloc(sizeof(display_info_t));
    display_info->buffer_pixels = buffer_pixels;
    display_info->width = width;
    display_info->height = height;
    display_info->dma_buffer = dma_buf;
    lv_display_set_user_data(lvDisp, display_info);

    // Set up flush callback to render via M5GFX with proper DMA synchronization
    lv_display_set_flush_cb(lvDisp, [](lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
        // Calculate area dimensions
        int32_t w = area->x2 - area->x1 + 1;
        int32_t h = area->y2 - area->y1 + 1;
        int32_t total_pixels = w * h;

        ESP_LOGD("HAL", "Flushing area: (%d,%d) to (%d,%d), size: %dx%d (%d pixels)",
                 (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2, (int)w, (int)h, (int)total_pixels);

        // Get display information from user data
        struct display_info_t {
            size_t buffer_pixels;
            uint32_t width;
            uint32_t height;
            void* dma_buffer;
        };
        display_info_t* display_info = (display_info_t*)lv_display_get_user_data(disp);
        if (!display_info) {
            ESP_LOGE("HAL", "No display info available");
            lv_display_flush_ready(disp);
            return;
        }

        uint32_t width = display_info->width;
        uint32_t height = display_info->height;
        void* dma_buf = display_info->dma_buffer;

        // Enhanced validation of flush parameters and area bounds
        if (!px_map || total_pixels <= 0 || total_pixels > (width * height) ||
            area->x1 < 0 || area->y1 < 0 || area->x2 >= (int32_t)width || area->y2 >= (int32_t)height ||
            area->x1 > area->x2 || area->y1 > area->y2) {
            ESP_LOGE("HAL", "Invalid flush parameters: px_map=%p, pixels=%d, area=(%d,%d)-(%d,%d), screen=%dx%d",
                     px_map, (int)total_pixels, (int)area->x1, (int)area->y1, (int)area->x2, (int)area->y2,
                     (int)width, (int)height);
            lv_display_flush_ready(disp);
            return;
        }

        // Calculate buffer size for cache operations with alignment padding
        size_t pixel_data_size = total_pixels * sizeof(lv_color_t);

        // In DIRECT mode, we should never exceed full screen buffer size
        size_t expected_buffer_size = (display_info->buffer_pixels * sizeof(lv_color_t));
        if (pixel_data_size > expected_buffer_size) {
            ESP_LOGE("HAL", "Buffer overflow: data_size=%zu > buffer_size=%zu - should not happen in DIRECT mode",
                     pixel_data_size, expected_buffer_size);
            lv_display_flush_ready(disp);
            return;
        }

        // CRITICAL FIX: Use separate DMA buffer to preserve LVGL buffer integrity
        // In DIRECT mode with full-screen buffers, we MUST transfer the ENTIRE screen every time
        // Partial updates cause smearing because display controller memory gets out of sync

        // Step 1: Ensure LVGL's full buffer rendering is synced to SPIRAM before copy
        if (esp_ptr_external_ram(px_map)) {
            size_t full_buffer_size = expected_buffer_size;
            size_t aligned_buffer_size = ((full_buffer_size + 127) & ~127);
            esp_cache_msync(px_map, aligned_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        }

        // Step 2: Copy ENTIRE buffer to DMA buffer (preserves LVGL buffer for next cycle)
        memcpy(dma_buf, px_map, expected_buffer_size);

        // Step 3: Byte swap ENTIRE buffer in DMA buffer
        lv_draw_sw_rgb565_swap((lv_color_t*)dma_buf, display_info->buffer_pixels);

        // Step 4: Writeback entire DMA buffer to SPIRAM for DMA transfer
        if (esp_ptr_external_ram(dma_buf)) {
            size_t full_buffer_size = expected_buffer_size;
            size_t aligned_buffer_size = ((full_buffer_size + 127) & ~127);
            esp_cache_msync(dma_buf, aligned_buffer_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        }

        // Use M5GFX writePixelsDMA to transfer FULL SCREEN (not just changed region)
        M5.Display.startWrite();
        M5.Display.setAddrWindow(0, 0, width, height);  // Full screen window

        // Perform DMA transfer of ENTIRE screen to prevent smearing
        M5.Display.writePixelsDMA((uint16_t*)dma_buf, display_info->buffer_pixels);

        // Wait for DMA transfer to complete
        M5.Display.waitDMA();

        // End write operation
        M5.Display.endWrite();

        // No swap-back needed - LVGL buffer was never modified
        // DMA buffer can be reused for next flush
        lv_display_flush_ready(disp);
    });

    // Performance optimizations for smooth rendering
    lv_display_set_antialiasing(lvDisp, false);  // Disable antialiasing for better performance
    lv_display_set_dpi(lvDisp, 160);             // Set reasonable DPI for performance

    ESP_LOGI("HAL", "LVGL display created successfully with M5Unified backend and full-screen double buffering");
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
