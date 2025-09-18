#ifndef GUI_SCREEN_CALCULATOR_H
#define GUI_SCREEN_CALCULATOR_H

#include "lvgl.h"

// Calculator screen object
extern lv_obj_t *calculator_screen;

// Function declarations
void create_calculator_screen(void);
void show_calculator_screen(void);
void calculator_screen_back(void);
void destroy_calculator_screen(void);

#endif // GUI_SCREEN_CALCULATOR_H