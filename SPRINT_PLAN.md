# Tab5_Launcher Sprint Implementation Plan

## bmorcelli Launcher Analysis Summary

Based on comprehensive research, the bmorcelli Launcher is a sophisticated ESP32-based launcher with:

### Core Architecture
- **Framework**: Arduino ESP32 v2.0.17d with PlatformIO build system
- **UI Framework**: moononournation/GFX Library (TFT_eSPI/LovyanGFX abstraction)
- **File Systems**: Arduino SD (FAT32) + SPIFFS for configuration
- **Networking**: AsyncWebServer + WebSocket for HTTP interface
- **Configuration**: ArduinoJson v7 for settings management

### Key Features Identified
1. **File Management**: Recursive operations, multi-file support, pagination
2. **Text Editor**: Multi-format support (.txt, .json, .py, .html, .css, .js) with minification
3. **Python Launcher**: MicroPython integration with REPL functionality
4. **Web Interface**: Drag-drop upload, real-time monitoring, mobile-responsive
5. **OTA System**: HTTP/HTTPS downloads, binary validation, rollback capability
6. **Configuration**: JSON-based settings with backup/restore

### Hardware Integration
- **Displays**: TFT via TFT_eSPI/LovyanGFX with touch support
- **Storage**: SD card (FAT32) + SPIFFS internal storage
- **Input**: Touch (XPT2046) + physical buttons
- **Network**: WiFi with HTTP server capabilities

## Sprint Breakdown

### Sprint 1: Foundation & Core Infrastructure (Week 1)
**Goal**: Establish ESP-IDF 5.4.1 + LVGL 9.3.0 foundation

#### Sprint 1 Tasks:

**Task 1.1: Port Arduino SD APIs to ESP-IDF VFS/FAT System**
- **Files**: `main/sd_manager.c`, `main/vfs_integration.c`
- **Description**: Replace Arduino SD library with ESP-IDF VFS/FAT APIs
- **Key Changes**:
  - Replace `SD.begin()` with `esp_vfs_fat_sdspi_mount()`
  - Port file operations to standard POSIX calls (fopen, fread, fwrite)
  - Implement error handling for ESP-IDF error codes
  - Maintain existing sd_manager interface compatibility
- **Success Criteria**: SD card mounting, basic file operations functional

**Task 1.2: Implement SPIFFS Integration for Configuration**
- **Files**: `main/config_manager.c`, `main/spiffs_config.c`
- **Description**: Create SPIFFS-based configuration management
- **Key Changes**:
  - Initialize SPIFFS partition with `esp_vfs_spiffs_register()`
  - Port ArduinoJson to ESP-IDF cJSON library
  - Create configuration backup/restore functionality
  - Add NVS integration for critical settings
- **Success Criteria**: Configuration load/save working with SPIFFS

**Task 1.3: Create LVGL File Browser Foundation**
- **Files**: `main/gui_file_browser_v2.c`, `main/lvgl_file_list.c`
- **Description**: Build modern LVGL file browser replacing current basic version
- **Key Changes**:
  - Create LVGL list widget with smooth scrolling
  - Implement touch-optimized navigation
  - Add file/folder icon display system
  - Create pagination for large directories
- **Success Criteria**: Touch-responsive file browser with smooth scrolling

**Task 1.4: Enhanced Configuration Management System with Settings UI**
- **Files**: `main/gui_screen_settings.c`, `main/gui_screen_settings.h`, `main/gui_screen_main.c`
- **Description**: Create Settings screen with full configuration UI
- **Key Changes**:
  - Add "Settings" button to main screen (below File Manager)
  - Create new Settings screen with LVGL controls:
    - Brightness slider (0-100)
    - Volume slider (0-100)
    - Screen timeout dropdown (Never, 1, 5, 10, 15 minutes)
    - File browser view mode (List/Grid/Detailed)
    - Sort options (Name/Size/Date/Type)
    - Show hidden files checkbox
    - Theme selector (Dark/Light/Custom)
    - Language dropdown
    - SD auto-mount toggle
  - Add "Backup Config to SD" button
  - Add "Restore Config from SD" button
  - Add "Reset to Defaults" button with confirmation
  - Auto-save settings on change
  - Apply settings immediately (brightness, theme, etc.)
- **Success Criteria**: Functional Settings screen with immediate effect on user changes

**Task 1.5: Firmware Unload/Eject Functionality**
- **Files**: `main/firmware_loader.c`, `main/firmware_boot.c`, `main/gui_screen_main.c`
- **Description**: Add ability to unload/eject currently loaded firmware from OTA partition
- **Key Changes**:
  - Create `firmware_loader_unload()` function to clear OTA partition
  - Add "Eject Firmware" button below "Run Firmware" in main screen
  - Clear NVS boot flags when ejecting firmware
  - Update UI to show firmware load/unload status
  - Ensure launcher remains default boot partition after eject
- **Success Criteria**: Safe firmware unload with visual confirmation

### Sprint 2: File Manager Core (Week 2)
**Goal**: Complete file browser with all bmorcelli file operations

#### Sprint 2 Tasks:

**Task 2.1: Implement Core File Operations**
- **Files**: `main/file_operations.c`, `main/file_dialogs.c`
- **Description**: Create, delete, rename, copy, paste operations
- **Key Changes**:
  - Implement file/folder creation with name validation
  - Add delete operations with confirmation dialogs
  - Create rename functionality with duplicate checking
  - Implement copy/paste with progress indication
- **Success Criteria**: All basic file operations working reliably

**Task 2.2: Multi-File Selection System**
- **Files**: `main/file_selection.c`, `main/multi_select_ui.c`
- **Description**: Enable checkbox-based multi-file selection
- **Key Changes**:
  - Add checkbox UI elements to file list
  - Implement select-all/none functionality
  - Create batch operation confirmation dialogs
  - Add selection counter and status display
- **Success Criteria**: Multi-file operations with touch-friendly interface

**Task 2.3: File Type Detection and Icons**
- **Files**: `main/file_types.c`, `main/file_icons.c`
- **Description**: File extension detection with appropriate icons
- **Key Changes**:
  - Create file type detection based on extensions
  - Implement icon system for different file types
  - Add MIME type support for web interface
  - Create file metadata display (size, date)
- **Success Criteria**: Visual file type identification

**Task 2.4: Advanced File Search and Filtering**
- **Files**: `main/file_search.c`, `main/search_ui.c`
- **Description**: File search and filtering capabilities
- **Key Changes**:
  - Implement filename pattern matching
  - Add content search for text files
  - Create filter by file type functionality
  - Add search result navigation
- **Success Criteria**: Fast and accurate file search

### Sprint 3-12: Detailed Implementation Plans
*(Detailed plans for remaining sprints available in separate documentation)*

## Technical Migration Strategy

### Arduino to ESP-IDF API Mapping
```
Arduino SD → ESP-IDF VFS/FAT
Arduino SPIFFS → ESP-IDF SPIFFS component  
ArduinoJson → ESP-IDF cJSON
AsyncWebServer → ESP-IDF esp_http_server
GFX Library → LVGL 9.3.0 with M5Stack BSP
```

### Key Integration Points
1. **Hardware Abstraction**: M5Stack Tab5 BSP integration
2. **Memory Management**: ESP32-P4 PSRAM utilization
3. **Display**: LVGL with DSI interface optimization
4. **Touch**: Capacitive touch with gesture recognition
5. **Storage**: Dual storage (SD + SPIFFS) coordination

### Risk Mitigation
- **Memory Constraints**: Efficient buffer management from start
- **Performance**: Continuous profiling and optimization  
- **Complexity**: Incremental development with thorough testing
- **Hardware Limits**: Work within ESP32-P4 constraints

## Development Workflow
1. **Environment Setup**: ESP-IDF 5.4.1 + M5Stack Tab5 BSP
2. **Build Commands**: Use existing CLAUDE.md build instructions
3. **Testing**: Hardware validation on each feature
4. **Integration**: Maintain compatibility with existing codebase
5. **Documentation**: Update implementation notes continuously

---

*This sprint plan provides comprehensive roadmap for porting bmorcelli Launcher features to Tab5_Launcher with modern LVGL UI and ESP-IDF foundation.*