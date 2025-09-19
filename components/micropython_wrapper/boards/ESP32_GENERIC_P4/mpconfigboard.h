/*
 * ESP32-P4 MicroPython Board Configuration
 *
 * This board configuration is optimized for ESP32-P4 microcontrollers,
 * particularly targeting the M5Stack Tab5 platform but designed to work
 * with any ESP32-P4 based hardware.
 */

#define MICROPY_HW_BOARD_NAME               "ESP32-P4 Generic"
#define MICROPY_HW_MCU_NAME                 "ESP32-P4"

// ESP32-P4 specific features
#define MICROPY_HW_ENABLE_SDCARD            (1)
#define MICROPY_HW_ENABLE_MDNS              (0)  // Disabled due to no WiFi

// Memory configuration optimized for ESP32-P4
#define MICROPY_ALLOC_PATH_MAX              (128)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT      (16)
#define MICROPY_ALLOC_PARSE_RULE_INIT       (8)

// GPIO configuration for ESP32-P4
// Note: These are generic defaults, may need adjustment for specific boards
#define MICROPY_HW_I2C_SCL                  (8)
#define MICROPY_HW_I2C_SDA                  (7)
#define MICROPY_HW_SPI_SCK                  (12)
#define MICROPY_HW_SPI_MOSI                 (11)
#define MICROPY_HW_SPI_MISO                 (13)

// ESP32-P4 doesn't have WiFi/Bluetooth - disable related features
#define MICROPY_PY_BLUETOOTH                (0)
#define MICROPY_PY_NETWORK_WLAN             (0)
#define MICROPY_PY_ESPNOW                   (0)

// Enable ESP32-P4 specific modules
#define MICROPY_PY_MACHINE_SDCARD           (1)
#define MICROPY_PY_MACHINE_I2S              (1)
#define MICROPY_PY_MACHINE_DAC              (1)

// Display support for DSI interface (ESP32-P4 feature)
#define MICROPY_HW_ENABLE_DSI_LCD           (1)

// File system support
#define MICROPY_VFS_FAT                     (1)
#define MICROPY_VFS_LFS2                    (1)

// Networking - Use ESP-IDF networking from C side if needed
#define MICROPY_PY_SOCKET                   (0)
#define MICROPY_PY_WEBREPL                  (0)

// Threading - limited for memory efficiency
#define MICROPY_PY_THREAD                   (1)
#define MICROPY_PY_THREAD_GIL               (1)

// Reduced feature set for memory constraints
#define MICROPY_PY_ASYNCIO                  (0)
#define MICROPY_PY_COLLECTIONS_DEFAULTDICT  (0)
#define MICROPY_PY_COLLECTIONS_NAMEDTUPLE   (0)

// Enable useful modules for embedded development
#define MICROPY_PY_JSON                     (1)
#define MICROPY_PY_RE                       (1)
#define MICROPY_PY_ZLIB                     (1)
#define MICROPY_PY_UBINASCII                (1)
#define MICROPY_PY_UHASHLIB                 (1)

// ESP32-P4 memory map
#define MICROPY_HW_FLASH_SIZE               (16 * 1024 * 1024)  // 16MB typical
#define MICROPY_HW_FLASH_MODE               (ESP_IMAGE_SPI_MODE_DIO)

// PSRAM configuration for ESP32-P4
#define MICROPY_HW_PSRAM_SIZE               (8 * 1024 * 1024)   // 8MB typical
#define MICROPY_ALLOC_PARSE_CHUNK_INIT      (16)

// Pin definitions for M5Stack Tab5 (can be overridden)
#ifndef MICROPY_HW_TAB5_PINS
#define MICROPY_HW_TAB5_PINS                (0)
#endif

#if MICROPY_HW_TAB5_PINS
// M5Stack Tab5 specific pin assignments
#define MICROPY_HW_TAB5_LCD_CS              (39)
#define MICROPY_HW_TAB5_LCD_DC              (40)
#define MICROPY_HW_TAB5_LCD_RST             (41)
#define MICROPY_HW_TAB5_TOUCH_INT           (1)
#define MICROPY_HW_TAB5_TOUCH_RST           (2)
#define MICROPY_HW_TAB5_SD_CS               (10)
#endif

// Bootloader configuration
#define MICROPY_HW_ENABLE_UART_REPL         (1)
#define MICROPY_HW_UART_REPL_BAUD           (115200)

// Debug and development features
#ifdef DEBUG
#define MICROPY_DEBUG_VERBOSE               (1)
#define MICROPY_MEM_STATS                   (1)
#else
#define MICROPY_DEBUG_VERBOSE               (0)
#define MICROPY_MEM_STATS                   (0)
#endif

// ESP32-P4 specific optimizations
#define MICROPY_OPT_COMPUTED_GOTO           (1)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1)
#define MICROPY_OPT_MPZ_BITWISE             (1)

// Compiler optimization for RISC-V
#define MICROPY_COMP_CONST_FOLDING          (1)
#define MICROPY_COMP_MODULE_CONST           (1)
#define MICROPY_COMP_CONST                  (1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN    (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN    (1)