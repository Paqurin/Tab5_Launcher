#ifndef HAL_H
#define HAL_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display and input device handles
extern lv_display_t *lvDisp;
extern lv_indev_t *lvTouchpad;

// HAL initialization functions
void hal_init(void);
void hal_touchpad_init(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_H