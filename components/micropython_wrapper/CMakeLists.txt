# MicroPython wrapper component for Tab5 Launcher
# Provides Python execution capabilities within ESP-IDF application

cmake_minimum_required(VERSION 3.16)

# MicroPython paths
set(MICROPY_PORT_DIR ${CMAKE_CURRENT_LIST_DIR}/../micropython/ports/esp32)
set(MICROPY_DIR ${CMAKE_CURRENT_LIST_DIR}/../micropython)

# For now, use stub implementation to get basic integration working
set(USE_MICROPYTHON_STUB ON)
message(STATUS "Using MicroPython stub implementation for initial integration")

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
    # Real MicroPython implementation - add core sources
    set(MICROPY_SOURCES
        # Core Python sources
        "${MICROPY_DIR}/py/runtime.c"
        "${MICROPY_DIR}/py/compile.c"
        "${MICROPY_DIR}/py/lexer.c"
        "${MICROPY_DIR}/py/parse.c"
        "${MICROPY_DIR}/py/scope.c"
        "${MICROPY_DIR}/py/emitcommon.c"
        "${MICROPY_DIR}/py/emitbc.c"
        "${MICROPY_DIR}/py/bc.c"
        "${MICROPY_DIR}/py/vm.c"
        "${MICROPY_DIR}/py/gc.c"
        "${MICROPY_DIR}/py/malloc.c"
        "${MICROPY_PORT_DIR}/mphalport.c"
        "${MICROPY_DIR}/py/obj.c"
        "${MICROPY_DIR}/py/objarray.c"
        "${MICROPY_DIR}/py/objbool.c"
        "${MICROPY_DIR}/py/objboundmeth.c"
        "${MICROPY_DIR}/py/objcell.c"
        "${MICROPY_DIR}/py/objclosure.c"
        "${MICROPY_DIR}/py/objcomplex.c"
        "${MICROPY_DIR}/py/objdict.c"
        "${MICROPY_DIR}/py/objenumerate.c"
        "${MICROPY_DIR}/py/objexcept.c"
        "${MICROPY_DIR}/py/objfilter.c"
        "${MICROPY_DIR}/py/objfloat.c"
        "${MICROPY_DIR}/py/objfun.c"
        "${MICROPY_DIR}/py/objgenerator.c"
        "${MICROPY_DIR}/py/objgetitemiter.c"
        "${MICROPY_DIR}/py/objint.c"
        "${MICROPY_DIR}/py/objlist.c"
        "${MICROPY_DIR}/py/objmap.c"
        "${MICROPY_DIR}/py/objmodule.c"
        "${MICROPY_DIR}/py/objnamedtuple.c"
        "${MICROPY_DIR}/py/objnone.c"
        "${MICROPY_DIR}/py/objobject.c"
        "${MICROPY_DIR}/py/objpolyiter.c"
        "${MICROPY_DIR}/py/objproperty.c"
        "${MICROPY_DIR}/py/objrange.c"
        "${MICROPY_DIR}/py/objreversed.c"
        "${MICROPY_DIR}/py/objset.c"
        "${MICROPY_DIR}/py/objsingleton.c"
        "${MICROPY_DIR}/py/objslice.c"
        "${MICROPY_DIR}/py/objstr.c"
        "${MICROPY_DIR}/py/objstringio.c"
        "${MICROPY_DIR}/py/objstrunicode.c"
        "${MICROPY_DIR}/py/objtuple.c"
        "${MICROPY_DIR}/py/objtype.c"
        "${MICROPY_DIR}/py/objzip.c"
        "${MICROPY_DIR}/py/opmethods.c"
        "${MICROPY_DIR}/py/pairheap.c"
        "${MICROPY_DIR}/py/qstr.c"
        "${MICROPY_DIR}/py/reader.c"
        "${MICROPY_DIR}/py/repl.c"
        "${MICROPY_DIR}/py/ringbuf.c"
        "${MICROPY_DIR}/py/scheduler.c"
        "${MICROPY_DIR}/py/sequence.c"
        "${MICROPY_DIR}/py/stackctrl.c"
        "${MICROPY_DIR}/py/stream.c"
        "${MICROPY_DIR}/py/unicode.c"
        "${MICROPY_DIR}/py/warning.c"

        # Shared modules
        "${MICROPY_DIR}/shared/runtime/interrupt_char.c"
        "${MICROPY_DIR}/shared/runtime/pyexec.c"
        "${MICROPY_DIR}/shared/runtime/stdout_helpers.c"
        "${MICROPY_DIR}/shared/runtime/sys_stdio_mphal.c"

        # Essential modules for embedded use
        "${MICROPY_DIR}/py/builtinevex.c"
        "${MICROPY_DIR}/py/builtinimport.c"
        "${MICROPY_DIR}/py/modarray.c"
        "${MICROPY_DIR}/py/modbuiltins.c"
        "${MICROPY_DIR}/py/modcollections.c"
        "${MICROPY_DIR}/py/modgc.c"
        "${MICROPY_DIR}/py/modio.c"
        "${MICROPY_DIR}/py/modmath.c"
        "${MICROPY_DIR}/py/modmicropython.c"
        "${MICROPY_DIR}/py/modstruct.c"
        "${MICROPY_DIR}/py/modsys.c"
        "${MICROPY_DIR}/py/modthread.c"

        # ESP32 port specific sources
        "${MICROPY_PORT_DIR}/gccollect.c"
        "${MICROPY_PORT_DIR}/uart.c"
        "${MICROPY_PORT_DIR}/modos.c"
        "${MICROPY_PORT_DIR}/modmachine.c"
        "${MICROPY_PORT_DIR}/modesp32.c"
        "${MICROPY_PORT_DIR}/machine_pin.c"
        "${MICROPY_PORT_DIR}/machine_uart.c"
        "${MICROPY_PORT_DIR}/machine_i2c.c"
        "${MICROPY_PORT_DIR}/machine_hw_spi.c"
        "${MICROPY_PORT_DIR}/machine_adc.c"
        "${MICROPY_PORT_DIR}/machine_dac.c"
        "${MICROPY_PORT_DIR}/machine_i2s.c"
    )

    # Add MicroPython sources to component
    target_sources(${COMPONENT_LIB} PRIVATE ${MICROPY_SOURCES})

    # Add MicroPython include directories
    # Put our ESP32-P4 config first to override ESP32 port defaults
    target_include_directories(${COMPONENT_LIB} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
        "${MICROPY_DIR}/ports/esp32/boards/M5STACK_TAB5"
        "${MICROPY_DIR}"
        "${MICROPY_PORT_DIR}"
        "${MICROPY_DIR}/py"
        "${MICROPY_DIR}/shared"
        "${MICROPY_DIR}/shared/runtime"
        "${MICROPY_DIR}/extmod"
        "${MICROPY_DIR}/lib"
    )

    # Configure MicroPython compilation
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MICROPY_ESP_IDF_4=1
        MICROPY_PORT_CONFIG_H=\"mpconfigport.h\"
        MICROPY_VFS_FAT=1
        MICROPY_PY_JSON=1
        "MICROPY_HEAP_SIZE=(128*1024)"
        "MICROPY_TASK_STACK_SIZE=(16*1024)"
        MICROPY_ENABLE_GC=1
        MICROPY_GC_ALLOC_THRESHOLD=1
        MICROPY_PY_THREAD=1
        MICROPY_ENABLE_COMPILER=1
        MICROPY_READER_VFS=1
        MICROPY_ALLOC_PATH_MAX=128
        MICROPY_COMP_MODULE_CONST=1
        MICROPY_COMP_CONST=1
        MICROPY_COMP_DOUBLE_TUPLE_ASSIGN=1
        MICROPY_COMP_TRIPLE_TUPLE_ASSIGN=1
        MICROPY_ENABLE_DOC_STRING=0
        MICROPY_ERROR_REPORTING=MICROPY_ERROR_REPORTING_NORMAL
        MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG=0
        MICROPY_PY_BUILTINS_BYTEARRAY=1
        MICROPY_PY_BUILTINS_ENUMERATE=1
        MICROPY_PY_BUILTINS_FILTER=1
        MICROPY_PY_BUILTINS_REVERSED=1
        MICROPY_PY_BUILTINS_SET=1
        MICROPY_PY_BUILTINS_SLICE=1
        MICROPY_PY_BUILTINS_STR_UNICODE=1
        MICROPY_PY_BUILTINS_ZIP=1
        MICROPY_PY_MATH=1
        MICROPY_PY_IO=1
        MICROPY_PY_STRUCT=1
        MICROPY_PY_SYS=1
        MICROPY_PY_SYS_MAXSIZE=1
        MICROPY_PY_SYS_MODULES=1
        MICROPY_PY_SYS_PATH=1
        "MICROPY_PY_SYS_PLATFORM=\"esp32p4\""
        MICROPY_PY_COLLECTIONS=1
        "MP_SSIZE_MAX=(0x7fffffff)"
        "MICROPY_GIT_TAG=\"v1.24.1-esp32p4-tab5\""
        MICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool
        MICROPY_MODULE_FROZEN_MPY=1

        # ESP32-P4 specific
        MICROPY_PY_BLUETOOTH=0
        MICROPY_PY_NETWORK_WLAN=0
        MICROPY_PY_ESPNOW=0
        MICROPY_PY_SOCKET=0
        MICROPY_PY_WEBREPL=0
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
        -Wno-stringop-truncation
        -Wno-array-bounds
        -Wno-format-truncation
    )

    # Create required qstr header files to avoid build errors
    set(QSTR_DEFS_COLLECTED "${CMAKE_CURRENT_BINARY_DIR}/qstrdefs.collected.h")
    set(QSTR_DEFS_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/genhdr/qstrdefs.generated.h")

    # Create genhdr directory
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/genhdr")

    # Create dummy qstr files
    file(WRITE ${QSTR_DEFS_COLLECTED} "// Dummy qstr definitions\n")
    file(WRITE ${QSTR_DEFS_GENERATED} "// Dummy generated qstr definitions\n")

    # Add include path for generated headers
    target_include_directories(${COMPONENT_LIB} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
    )

    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        "QSTR_DEFS_COLLECTED_H=\"${QSTR_DEFS_COLLECTED}\""
    )

    message(STATUS "Using real MicroPython implementation with ${CMAKE_LIST_LENGTH} sources")
endif()