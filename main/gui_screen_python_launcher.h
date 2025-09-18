#ifndef GUI_SCREEN_PYTHON_LAUNCHER_H
#define GUI_SCREEN_PYTHON_LAUNCHER_H

#include "lvgl.h"
#include "esp_err.h"

// Python launcher screen object
extern lv_obj_t *python_launcher_screen;

// Function declarations
void create_python_launcher_screen(void);
void show_python_launcher_screen(void);
void python_launcher_screen_back(void);
esp_err_t python_launcher_execute_script(const char *script_path);
bool python_launcher_is_supported_file(const char *filename);

#endif // GUI_SCREEN_PYTHON_LAUNCHER_H