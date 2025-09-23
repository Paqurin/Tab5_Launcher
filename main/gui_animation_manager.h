#ifndef GUI_ANIMATION_MANAGER_H
#define GUI_ANIMATION_MANAGER_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the global animation manager
 *
 * This system manages all LVGL animations based on user settings.
 * When animations are disabled, all widgets use instant updates.
 * When enabled, widgets use smooth animations.
 */
void gui_animation_manager_init(void);

/**
 * @brief Set global animation state
 *
 * @param enabled true to enable animations, false to disable
 */
void gui_animation_manager_set_enabled(bool enabled);

/**
 * @brief Get current animation state
 *
 * @return true if animations are enabled, false otherwise
 */
bool gui_animation_manager_is_enabled(void);

/**
 * @brief Get appropriate animation time for current setting
 *
 * @param default_time Default animation time in ms
 * @return 0 if animations disabled, default_time if enabled
 */
uint32_t gui_animation_manager_get_time(uint32_t default_time);

/**
 * @brief Get appropriate animation flag for current setting
 *
 * @return LV_ANIM_ON if animations enabled, LV_ANIM_OFF if disabled
 */
lv_anim_enable_t gui_animation_manager_get_anim_flag(void);

/**
 * @brief Update animation manager from configuration
 *
 * Call this when the user changes animation settings
 */
void gui_animation_manager_update_from_config(void);

#ifdef __cplusplus
}
#endif

#endif /* GUI_ANIMATION_MANAGER_H */