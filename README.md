# Tab5-Launcher

## English
A port of the M5Launcher (sort of) for the Tab5, with LVGL GUI.

### What can it do?
You can select and load firmwares from the SD card (must be formatted in FAT32) and flash them into the device.

### Known issues:
 - In order to make the touch panel work, you must reboot the device manually after flashing a firmware.
 - Sometimes when flashing a firmware LVGL will run into errors rendering the progress bar so the UI freezes (The flashing process is still going though).
 - The file manager is not working due to path issues

## 中文
将 M5Launcher "移植"到Tab5设备上，使用LVGL实现GUI。

### 它能做什么？
从SD卡中加载.bin格式的固件文件（SD卡必须使用FAT32文件系统）并将其烧录到设备，然后运行固件。

### 已知的问题
 - 重启到固件时，必须手动重启，否则触控会无法工作
 - 有时烧录固件时，LVGL会出错导致进度条（界面）停止重绘，但是固件烧录进程依然会正常工作。
 - 文件管理器目前无法正常工作，因为默认路径不对。