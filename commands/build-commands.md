# Build Commands

This document contains all build commands for the Tab5_Launcher project.

## ESP-IDF Commands

### Environment Setup
```bash
# Set up ESP-IDF 5.4.1 environment (required before any idf.py command)
. ~/esp/esp-idf-5.4.1/export.sh
```

### Standard Build Commands
```bash
# Basic build
idf.py build

# Build with distributed compilation (recommended)
CC=distcc CXX=distcc++ idf.py build
```

### Flash Commands
```bash
# Flash firmware to device
idf.py flash

# Build and flash in one command
CC=distcc CXX=distcc++ idf.py build flash
```

### Monitor and Debug
```bash
# Monitor serial output
idf.py monitor

# Build, flash, and monitor in sequence
CC=distcc CXX=distcc++ idf.py build flash monitor
```

### Clean Commands
```bash
# Clean build artifacts
idf.py clean

# Full clean and rebuild
idf.py fullclean
```

### Configuration
```bash
# Open menuconfig for project settings
idf.py menuconfig

# Set target (should be esp32p4 for Tab5)
idf.py set-target esp32p4
```

## Advanced Build Options

### Distributed Compilation with DistCC
```bash
# Set DistCC hosts for faster compilation
export DISTCC_HOSTS="localhost/4 192.168.1.100/8"

# Build with distributed compilation
CC=distcc CXX=distcc++ idf.py build -j$(distcc -j)
```

### Partition Management
```bash
# Show partition table
idf.py partition-table

# Flash only partition table
idf.py partition-table-flash
```

### Component Management
```bash
# Update component dependencies
idf.py update-dependencies

# Show component information
idf.py show-components
```

## Development Workflow Commands

### Complete Development Cycle
```bash
# 1. Set up environment
. ~/esp/esp-idf-5.4.1/export.sh

# 2. Build with distributed compilation
CC=distcc CXX=distcc++ idf.py build

# 3. Flash to device
idf.py flash

# 4. Monitor output
idf.py monitor
```

### Quick Test Cycle
```bash
# Environment + Build + Flash + Monitor (all in one)
. ~/esp/esp-idf-5.4.1/export.sh && CC=distcc CXX=distcc++ idf.py build flash monitor
```

### Debugging Commands
```bash
# Show build size analysis
idf.py size

# Show detailed size by components
idf.py size-components

# Show memory usage
idf.py size-files
```

## Branch and Version Management

### Git Commands for Development
```bash
# Create new feature branch
git checkout -b feature/pull-down-menu

# Commit with proper message format
git commit -m "Add pull-down status bar menu

- Implement touch gesture detection for status bar
- Add expandable menu with WiFi, SD, brightness controls
- Enhanced user interaction feedback

🤖 Generated with [Claude Code](https://claude.ai/code)

Co-Authored-By: Claude <noreply@anthropic.com>"

# Push to remote
git push -u origin feature/pull-down-menu
```

### Version Tagging
```bash
# Create version tag
git tag -a v0.03 -m "v0.03: Pull-down status bar menu implementation"

# Push tags
git push --tags
```

## Troubleshooting Commands

### Common Issues
```bash
# If ESP-IDF environment is not set up
. ~/esp/esp-idf-5.4.1/export.sh

# If build fails due to dependencies
idf.py clean && idf.py fullclean && CC=distcc CXX=distcc++ idf.py build

# If device not found
ls /dev/ttyACM* /dev/ttyUSB*

# If flash fails, try different baud rate
idf.py -b 115200 flash
```

### Reset and Recovery
```bash
# Erase entire flash
idf.py erase-flash

# Flash bootloader only
idf.py bootloader-flash

# Flash app only (faster for testing)
idf.py app-flash
```