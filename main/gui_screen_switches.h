#pragma once

#include "lvgl.h"

// Global screen object
extern lv_obj_t *switches_screen;

// Function declarations
void create_switches_screen(void);
void show_switches_screen(void);
void switches_screen_back(void);
void destroy_switches_screen(void);