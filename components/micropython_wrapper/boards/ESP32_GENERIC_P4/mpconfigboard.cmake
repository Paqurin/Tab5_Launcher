# ESP32-P4 MicroPython Board Configuration
# Build settings for ESP32-P4 generic board

set(IDF_TARGET esp32p4)

# ESP32-P4 specific build flags
set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    boards/sdkconfig.spiram_sx
    boards/ESP32_GENERIC_P4/sdkconfig.board
)

# ESP32-P4 doesn't have WiFi/Bluetooth - disable related components
set(MICROPY_PY_BLUETOOTH OFF)
set(MICROPY_PY_NETWORK_WLAN OFF)
set(MICROPY_PY_ESPNOW OFF)

# Enable ESP32-P4 specific features
set(MICROPY_PY_MACHINE_DAC ON)
set(MICROPY_PY_MACHINE_I2S ON)
set(MICROPY_PY_MACHINE_SDCARD ON)

# File system support
set(MICROPY_VFS_FAT ON)
set(MICROPY_VFS_LFS2 ON)

# Memory optimization for embedded use
set(MICROPY_PY_ASYNCIO OFF)
set(MICROPY_PY_COLLECTIONS_DEFAULTDICT OFF)
set(MICROPY_PY_COLLECTIONS_NAMEDTUPLE OFF)

# Keep essential modules
set(MICROPY_PY_JSON ON)
set(MICROPY_PY_RE ON)
set(MICROPY_PY_UBINASCII ON)
set(MICROPY_PY_UHASHLIB ON)

# ESP32-P4 compiler optimizations
list(APPEND MICROPY_DEF_BOARD
    MICROPY_HW_ENABLE_SDCARD=1
    MICROPY_VFS_FAT=1
    MICROPY_VFS_LFS2=1
    MICROPY_PY_MACHINE_I2S=1
    MICROPY_PY_MACHINE_DAC=1
    MICROPY_ALLOC_PATH_MAX=128
)

# PSRAM configuration for ESP32-P4
if(CONFIG_SPIRAM)
    list(APPEND MICROPY_DEF_BOARD
        MICROPY_HW_PSRAM_SIZE=CONFIG_SPIRAM_SIZE
        MICROPY_ALLOC_PARSE_CHUNK_INIT=16
    )
endif()

# Board specific includes
set(MICROPY_SOURCE_BOARD
    ${MICROPY_BOARD_DIR}/esp32p4_init.c
)