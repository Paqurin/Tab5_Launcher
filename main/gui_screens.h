#ifndef GUI_SCREENS_H
#define GUI_SCREENS_H

#include "lvgl.h"

// Screen objects (extern declarations)
extern lv_obj_t *main_screen;
extern lv_obj_t *file_manager_screen;
extern lv_obj_t *firmware_loader_screen;
extern lv_obj_t *progress_screen;
extern lv_obj_t *splash_screen;

// UI element objects
extern lv_obj_t *file_list;
extern lv_obj_t *firmware_list;
extern lv_obj_t *current_path_label;
extern lv_obj_t *flash_btn;
extern lv_obj_t *status_label;
extern lv_obj_t *progress_bar;
extern lv_obj_t *progress_label;
extern lv_obj_t *progress_step_label;

/**
 * @brief Create all screens
 */
void gui_screens_init(void);

/**
 * @brief Create main menu screen
 */
void create_main_screen(void);

/**
 * @brief Create file manager screen
 */
void create_file_manager_screen(void);

/**
 * @brief Create firmware loader screen
 */
void create_firmware_loader_screen(void);

/**
 * @brief Create progress screen
 */
void create_progress_screen(void);

/**
 * @brief Create splash screen
 */
void create_splash_screen(void);

/**
 * @brief Update file list display
 */
void update_file_list(void);

/**
 * @brief Update firmware list display
 */
void update_firmware_list(void);

#endif // GUI_SCREENS_H