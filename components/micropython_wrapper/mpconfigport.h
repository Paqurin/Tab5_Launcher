/*
 * ESP32-P4 MicroPython Port Configuration
 * This file configures MicroPython for ESP32-P4 integration within ESP-IDF projects
 */

#ifndef MPCONFIGPORT_H
#define MPCONFIGPORT_H

#include <stdint.h>
#include <alloca.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

// Board and MCU name
#define MICROPY_HW_BOARD_NAME "ESP32-P4 Tab5 Launcher"
#define MICROPY_HW_MCU_NAME "ESP32-P4"

// Memory allocation
#define MICROPY_ALLOC_PATH_MAX          (128)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT  (16)
#define MICROPY_ALLOC_PARSE_RULE_INIT   (8)

// Garbage collection
#define MICROPY_ENABLE_GC               (1)
#define MICROPY_GC_ALLOC_THRESHOLD      (1)
#define MICROPY_MEM_STATS               (0)
#define MICROPY_DEBUG_VERBOSE           (0)

// Compiler features
#define MICROPY_COMP_MODULE_CONST       (1)
#define MICROPY_COMP_CONST              (1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_RETURN_IF_EXPR     (1)

// Python features
#define MICROPY_ENABLE_COMPILER         (1)
#define MICROPY_ENABLE_DOC_STRING       (0)
#define MICROPY_ERROR_REPORTING         (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_PY_ASYNC_AWAIT          (0)  // Disabled for memory
#define MICROPY_PY_GENERATOR_PEND_THROW (0)

// Built-in types
#define MICROPY_PY_BUILTINS_BYTEARRAY   (1)
#define MICROPY_PY_BUILTINS_ENUMERATE   (1)
#define MICROPY_PY_BUILTINS_FILTER      (1)
#define MICROPY_PY_BUILTINS_FROZENSET   (0)  // Disabled for memory
#define MICROPY_PY_BUILTINS_REVERSED    (1)
#define MICROPY_PY_BUILTINS_SET         (1)
#define MICROPY_PY_BUILTINS_SLICE       (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE (1)
#define MICROPY_PY_BUILTINS_ZIP         (1)
#define MICROPY_PY_BUILTINS_COMPILE     (1)
#define MICROPY_PY_BUILTINS_EVAL_EXEC   (1)

// Modules
#define MICROPY_PY_ARRAY                (1)
#define MICROPY_PY_COLLECTIONS          (1)
#define MICROPY_PY_COLLECTIONS_DEQUE    (0)  // Disabled for memory
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (0)  // Disabled for memory
#define MICROPY_PY_MATH                 (1)
#define MICROPY_PY_CMATH                (0)  // Disabled for memory
#define MICROPY_PY_IO                   (1)
#define MICROPY_PY_STRUCT               (1)
#define MICROPY_PY_SYS                  (1)
#define MICROPY_PY_SYS_MAXSIZE          (1)
#define MICROPY_PY_SYS_MODULES          (1)
#define MICROPY_PY_SYS_PATH             (1)
#define MICROPY_PY_SYS_STDFILES         (1)
#define MICROPY_PY_SYS_STDIO_BUFFER     (1)

// Threading
#define MICROPY_PY_THREAD               (1)
#define MICROPY_PY_THREAD_GIL           (1)

// ESP32-P4 specific (no WiFi/Bluetooth)
#define MICROPY_PY_BLUETOOTH            (0)
#define MICROPY_PY_NETWORK_WLAN         (0)
#define MICROPY_PY_ESPNOW               (0)
#define MICROPY_PY_SOCKET               (0)
#define MICROPY_PY_WEBREPL              (0)

// File system
#define MICROPY_VFS                     (1)
#define MICROPY_VFS_FAT                 (1)
#define MICROPY_READER_VFS              (1)

// Machine module
#define MICROPY_PY_MACHINE              (1)
#define MICROPY_PY_MACHINE_PIN_MAKE_NEW mp_pin_make_new
#define MICROPY_PY_MACHINE_I2C          (1)
#define MICROPY_PY_MACHINE_SPI          (1)
#define MICROPY_PY_MACHINE_UART         (1)
#define MICROPY_PY_MACHINE_DAC          (1)
#define MICROPY_PY_MACHINE_I2S          (1)

// String handling
#define MICROPY_PY_USTRING              (1)

// Platform-specific
#define MP_SSIZE_MAX                    (0x7fffffff)
#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((uintptr_t)(p) | 1))
#define MP_PLAT_PRINT_STRN(str, len)    mp_hal_stdout_tx_strn_cooked(str, len)

// Use system malloc/free
#define MICROPY_MEM_ALIGN               (sizeof(void *))

// Type definitions
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef long mp_off_t;

// ESP32-P4 specific optimizations
#define MICROPY_OPT_COMPUTED_GOTO       (1)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (0)  // Disabled for memory

// Hardware features
#define MICROPY_HW_ENABLE_SDCARD        (1)
#define MICROPY_HW_ENABLE_MDNS          (0)

// Stack control
#define MICROPY_STACK_CHECK             (1)

// Additional ESP-IDF integration
#ifndef MICROPY_ESP_IDF_4
#define MICROPY_ESP_IDF_4               (1)
#endif

// Platform string
#define MICROPY_PY_SYS_PLATFORM         "esp32p4"

// Git tag for version
#ifndef MICROPY_GIT_TAG
#define MICROPY_GIT_TAG                 "v1.24.1-esp32p4-tab5"
#endif

// Qstring configuration
#define MICROPY_QSTR_BYTES_IN_HASH      (1)
#define MICROPY_QSTR_EXTRA_POOL         mp_qstr_frozen_const_pool

// Module frozen bytecode
#define MICROPY_MODULE_FROZEN_MPY       (1)
#define MICROPY_PERSISTENT_CODE_LOAD    (1)

// Weak links
#define MP_STATE_PORT MP_STATE_VM

// Function attributes
#define NORETURN __attribute__((noreturn))
#define MP_WEAK __attribute__((weak))

// Include standard headers
#include <sys/types.h>
#include <errno.h>

// External declarations
extern const struct _mp_obj_module_t mp_module_machine;
extern const struct _mp_obj_module_t mp_module_esp32;

// Built-in module table
#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_ROM_QSTR(MP_QSTR_machine), MP_ROM_PTR(&mp_module_machine) }, \

// Root pointers for GC
#define MICROPY_PORT_ROOT_POINTERS \
    const char *readline_hist[8]; \
    void *machine_pin_irq_handler[32]; \

#endif // MPCONFIGPORT_H