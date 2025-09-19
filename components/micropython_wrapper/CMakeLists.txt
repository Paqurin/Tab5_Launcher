# MicroPython wrapper component for Tab5 Launcher
# Provides Python execution capabilities within ESP-IDF application

cmake_minimum_required(VERSION 3.16)

# MicroPython paths
set(MICROPY_PORT_DIR ${CMAKE_CURRENT_LIST_DIR}/../micropython/ports/esp32)
set(MICROPY_DIR ${CMAKE_CURRENT_LIST_DIR}/../micropython)

# For Option 1: Use stub but with WiFi headers available
# TODO: Enable real implementation when qstring generation is solved
set(USE_MICROPYTHON_STUB ON)
message(STATUS "Using MicroPython stub implementation with WiFi headers available")

# Component sources (always include wrapper files)
set(COMPONENT_SRCS
    "micropython_wrapper.c"
    "python_executor.c"
)

# Include directories
set(COMPONENT_ADD_INCLUDEDIRS
    "include"
)

# Base ESP-IDF requirements
set(COMPONENT_REQUIRES
    driver
    esp_system
    esp_timer
    freertos
    heap
    nvs_flash
    log
    vfs
    fatfs
    console
    esp_wifi
    esp_netif
)

# Register the component
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    REQUIRES ${COMPONENT_REQUIRES}
)

if(USE_MICROPYTHON_STUB)
    # Use stub implementation
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MICROPYTHON_STUB_ONLY=1
    )
    message(STATUS "Using MicroPython stub implementation")
else()
    # Add MicroPython include directories for real implementation
    target_include_directories(${COMPONENT_LIB} PRIVATE
        "${MICROPY_DIR}"
        "${MICROPY_PORT_DIR}"
        "${MICROPY_DIR}/py"
        "${MICROPY_DIR}/shared"
        "${MICROPY_DIR}/shared/runtime"
        "${CMAKE_CURRENT_LIST_DIR}/boards/M5STACK_TAB5"
    )

    # Configure MicroPython compilation with proper syntax
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MICROPY_ESP_IDF_4=1
        MICROPY_HW_BOARD_NAME="M5Stack Tab5"
        MICROPY_HW_TAB5_PINS=1
        MICROPY_VFS_FAT=1
        MICROPY_PY_JSON=1
        "MICROPY_HEAP_SIZE=(128*1024)"
        "MICROPY_TASK_STACK_SIZE=(16*1024)"
        MICROPY_ENABLE_GC=1
        MICROPY_GC_ALLOC_THRESHOLD=1
        MICROPY_PY_THREAD=1
        MICROPY_ENABLE_COMPILER=1
        MICROPY_READER_VFS=1
        MICROPY_PERSISTENT_CODE_LOAD=1
        MICROPY_MODULE_FROZEN_MPY=1
        MICROPY_QSTR_BYTES_IN_HASH=1
        MICROPY_ALLOC_PATH_MAX=256
        MICROPY_EMIT_X64=0
        MICROPY_EMIT_THUMB=0
        MICROPY_EMIT_INLINE_THUMB=0
        MICROPY_COMP_MODULE_CONST=1
        MICROPY_COMP_CONST=1
        MICROPY_COMP_DOUBLE_TUPLE_ASSIGN=1
        MICROPY_COMP_TRIPLE_TUPLE_ASSIGN=1
        MICROPY_COMP_RETURN_IF_EXPR=1
        MICROPY_ENABLE_DOC_STRING=0
        MICROPY_ERROR_REPORTING=MICROPY_ERROR_REPORTING_NORMAL
        MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG=0
        MICROPY_PY_ASYNC_AWAIT=1
        MICROPY_PY_BUILTINS_BYTEARRAY=1
        MICROPY_PY_BUILTINS_DICT_FROMKEYS=1
        MICROPY_PY_BUILTINS_ENUMERATE=1
        MICROPY_PY_BUILTINS_EVAL_EXEC=1
        MICROPY_PY_BUILTINS_FILTER=1
        MICROPY_PY_BUILTINS_FROZENSET=1
        MICROPY_PY_BUILTINS_REVERSED=1
        MICROPY_PY_BUILTINS_SET=1
        MICROPY_PY_BUILTINS_SLICE=1
        MICROPY_PY_BUILTINS_STR_UNICODE=1
        MICROPY_PY_BUILTINS_ZIP=1
        MICROPY_PY_GENERATOR_PEND_THROW=1
        MICROPY_PY_MATH=1
        MICROPY_PY_CMATH=0
        MICROPY_PY_IO=1
        MICROPY_PY_STRUCT=1
        MICROPY_PY_SYS=1
        MICROPY_PY_SYS_MAXSIZE=1
        MICROPY_PY_SYS_MODULES=1
        MICROPY_PY_SYS_PATH=1
        "MICROPY_PY_SYS_PLATFORM=\"esp32p4\""
        MICROPY_PY_SYS_STDFILES=1
        MICROPY_PY_COLLECTIONS=1
        MICROPY_PY_COLLECTIONS_DEQUE=1
        MICROPY_PY_COLLECTIONS_ORDEREDDICT=1
        "MP_SSIZE_MAX=(0x7fffffff)"
        "MICROPY_GIT_TAG=\"v1.24.1-esp32p4-tab5\""
    )

    # Add compiler flags for MicroPython
    target_compile_options(${COMPONENT_LIB} PRIVATE
        -std=gnu99
        -Os
        -ffunction-sections
        -fdata-sections
        -Wall
        -Wno-unused-label
        -Wno-unused-parameter
        -Wno-sign-compare
        -Wno-missing-braces
        -Wno-missing-field-initializers
        -Wno-type-limits
        -Wno-unused-function
        -Wno-unused-variable
    )

    message(STATUS "Using real MicroPython implementation")
endif()