# Tab5-Launcher

## English
A port of the M5Launcher (sort of) for the Tab5, with LVGL GUI.

### What can it do?
You can select and load firmwares from the SD card (must be formatted in FAT32) and flash them into the device.

### Current Status
**Almost working!** The launcher runs and most core functions work, including:
- File manager with SD card support
- Basic firmware loading capabilities
- Text editor functionality
- Settings and configuration menus

**Still in progress:**
- Python launcher integration
- Complete firmware launching system

### Known issues:
 - Sometimes when flashing a firmware LVGL will run into errors rendering the progress bar so the UI freezes (The flashing process is still going though).
 - Python launcher not yet implemented
 - Firmware launching still being refined

## 中文
将 M5Launcher "移植"到Tab5设备上，使用LVGL实现GUI。

### 它能做什么？
从SD卡中加载.bin格式的固件文件（SD卡必须使用FAT32文件系统）并将其烧录到设备，然后运行固件。

### 当前状态
**基本能用了！** 启动器可以运行，大部分核心功能都工作正常，包括：
- 支持SD卡的文件管理器
- 基本的固件加载功能
- 文本编辑器功能
- 设置和配置菜单

**仍在开发中：**
- Python启动器集成
- 完整的固件启动系统

### 已知的问题
 - 有时烧录固件时，LVGL会出错导致进度条（界面）停止重绘，但是固件烧录进程依然会正常工作。
 - Python启动器尚未实现
 - 固件启动功能仍在完善中

## How to build
You can build using ESP-IDF, simply navigate to the project root and run `idf.py build`.
You should use the ESP-IDF shell in order to run idf.py commands.
Also, you can use your VS Code with the ESP-IDF Extension, simply open the project root directory in VS Code and the extension should automatically kick in.
## 如何编译
你可以使用ESP-IDF编译本项目。在项目根目录下执行`idf.py build`即可。
为了使用idf.py指令，你需要使用ESP-IDF的PowerShell或者CMD。
你也可以使用VS Code的ESP-IDF插件。用VS Code打开本项目根目录，插件会自动帮你配置，只需在VS Code中执行指令即可。