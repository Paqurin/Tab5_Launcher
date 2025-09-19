#include "micropython_wrapper.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "micropython_wrapper";

// Runtime state
static bool initialized = false;
static micropython_config_t current_config;
static TaskHandle_t python_task_handle = NULL;

#ifdef MICROPYTHON_STUB_ONLY

// Stub implementation for development
static void *python_heap = NULL;

esp_err_t micropython_init(const micropython_config_t *config)
{
    if (initialized) {
        ESP_LOGW(TAG, "MicroPython already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing MicroPython (STUB implementation)");

    // Set configuration
    if (config) {
        current_config = *config;
    } else {
        current_config.heap_size = 128 * 1024;
        current_config.stack_size = 16 * 1024;
        current_config.task_priority = 5;
        current_config.use_psram = true;
    }

    // Simulate heap allocation
    if (current_config.use_psram && heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > current_config.heap_size) {
        python_heap = heap_caps_malloc(current_config.heap_size, MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "Allocated %zu bytes for Python heap in PSRAM", current_config.heap_size);
    } else {
        python_heap = heap_caps_malloc(current_config.heap_size, MALLOC_CAP_INTERNAL);
        ESP_LOGI(TAG, "Allocated %zu bytes for Python heap in internal RAM", current_config.heap_size);
    }

    if (!python_heap) {
        ESP_LOGE(TAG, "Failed to allocate Python heap");
        return ESP_ERR_NO_MEM;
    }

    initialized = true;
    ESP_LOGI(TAG, "MicroPython stub initialized successfully");
    ESP_LOGW(TAG, "Using STUB implementation - real Python execution not available");

    return ESP_OK;
}

void micropython_deinit(void)
{
    if (!initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing MicroPython stub");

    if (python_heap) {
        heap_caps_free(python_heap);
        python_heap = NULL;
    }

    initialized = false;
}

bool micropython_is_initialized(void)
{
    return initialized;
}

const char* micropython_get_version(void)
{
    return "MicroPython v1.24.1-stub (ESP32-P4 development)";
}

size_t micropython_get_heap_free(void)
{
    if (!initialized) {
        return 0;
    }
    // Simulate 80% free heap
    return (current_config.heap_size * 4) / 5;
}

size_t micropython_get_heap_total(void)
{
    if (!initialized) {
        return 0;
    }
    return current_config.heap_size;
}

#else

// Real MicroPython implementation
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "py/stackctrl.h"
#include "py/mpthread.h"

// MicroPython heap and task configuration
#define MP_TASK_HEAP_SIZE      (128 * 1024)
#define MP_TASK_STACK_SIZE     (16 * 1024)
#define MP_TASK_PRIORITY       (ESP_TASK_PRIO_MIN + 1)

// Static allocation for MicroPython heap
static uint8_t mp_task_heap[MP_TASK_HEAP_SIZE];
static void *python_heap = mp_task_heap;
static StackType_t *python_task_stack = NULL;
static StaticTask_t python_task_buffer;

// Forward declarations
static void python_task(void *pvParameter);
void gc_collect(void);

esp_err_t micropython_init(const micropython_config_t *config)
{
    if (initialized) {
        ESP_LOGW(TAG, "MicroPython already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing MicroPython runtime");

    // Set configuration
    if (config) {
        current_config = *config;
    } else {
        current_config.heap_size = MP_TASK_HEAP_SIZE;
        current_config.stack_size = MP_TASK_STACK_SIZE;
        current_config.task_priority = MP_TASK_PRIORITY;
        current_config.use_psram = true;
    }

    // Allocate task stack
    if (current_config.use_psram && heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > current_config.stack_size) {
        python_task_stack = heap_caps_malloc(current_config.stack_size, MALLOC_CAP_SPIRAM);
        ESP_LOGI(TAG, "Allocated %zu bytes for Python task stack in PSRAM", current_config.stack_size);
    } else {
        python_task_stack = heap_caps_malloc(current_config.stack_size, MALLOC_CAP_INTERNAL);
        ESP_LOGI(TAG, "Allocated %zu bytes for Python task stack in internal RAM", current_config.stack_size);
    }

    if (!python_task_stack) {
        ESP_LOGE(TAG, "Failed to allocate Python task stack");
        return ESP_ERR_NO_MEM;
    }

    // Create Python task
    python_task_handle = xTaskCreateStatic(
        python_task,
        "micropython",
        current_config.stack_size / sizeof(StackType_t),
        NULL,
        current_config.task_priority,
        python_task_stack,
        &python_task_buffer
    );

    if (!python_task_handle) {
        ESP_LOGE(TAG, "Failed to create Python task");
        heap_caps_free(python_task_stack);
        python_task_stack = NULL;
        return ESP_ERR_NO_MEM;
    }

    initialized = true;
    ESP_LOGI(TAG, "MicroPython runtime initialized successfully");
    ESP_LOGI(TAG, "Heap: %zu bytes, Stack: %zu bytes, Priority: %d",
             current_config.heap_size, current_config.stack_size, current_config.task_priority);

    return ESP_OK;
}

void micropython_deinit(void)
{
    if (!initialized) {
        return;
    }

    ESP_LOGI(TAG, "Deinitializing MicroPython runtime");

    if (python_task_handle) {
        vTaskDelete(python_task_handle);
        python_task_handle = NULL;
    }

    if (python_task_stack) {
        heap_caps_free(python_task_stack);
        python_task_stack = NULL;
    }

    // Clean up MicroPython state
    mp_deinit();

    initialized = false;
    ESP_LOGI(TAG, "MicroPython runtime deinitialized");
}

bool micropython_is_initialized(void)
{
    return initialized;
}

const char* micropython_get_version(void)
{
    return MICROPY_GIT_TAG;
}

size_t micropython_get_heap_free(void)
{
    if (!initialized) {
        return 0;
    }
    return gc_nbytes(GC_INFO_FREE);
}

size_t micropython_get_heap_total(void)
{
    if (!initialized) {
        return 0;
    }
    return gc_nbytes(GC_INFO_ALLOC) + gc_nbytes(GC_INFO_FREE);
}

// MicroPython task implementation
static void python_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting MicroPython task");

    // Initialize stack limit checking
    mp_stack_set_top((void *)((uint32_t)pvParameter + current_config.stack_size));
    mp_stack_set_limit(1024);

    // Initialize MicroPython runtime
    mp_init();

    // Initialize garbage collector with our heap
    gc_init(python_heap, (char*)python_heap + current_config.heap_size);

    ESP_LOGI(TAG, "MicroPython runtime ready");

    // Run forever - the task will handle script execution requests
    while (true) {
        // Process any pending Python work
        mp_handle_pending(true);

        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Garbage collection implementation
void gc_collect(void) {
    // Start the GC
    gc_collect_start();

    // Trace root pointers from C stack
    mp_state_thread_t *ts = mp_thread_get_state();
    if (ts) {
        gc_collect_root((void**)&ts, sizeof(mp_state_thread_t) / sizeof(uintptr_t));

        // Trace root pointers from Python stack
        if (ts->stack) {
            gc_collect_root(ts->stack, (ts->stack_top - ts->stack) / sizeof(uintptr_t));
        }
    }

    // End the GC
    gc_collect_end();
}

#endif // MICROPYTHON_STUB_ONLY