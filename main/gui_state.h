#ifndef GUI_STATE_H
#define GUI_STATE_H

#include "sd_manager.h"
#include "firmware_loader.h"
#include <stdbool.h>

// State variables
extern char current_directory[512];
extern file_entry_t current_entries[32];
extern int current_entry_count;
extern firmware_info_t firmware_files[16];
extern int firmware_count;
extern int selected_firmware;

// File selection state
extern bool file_selection_enabled;
extern bool selected_files[32];  // Parallel array to current_entries
extern int selected_file_count;

// Clipboard state
extern bool clipboard_has_content;
extern bool clipboard_is_cut;  // true for cut/move, false for copy
extern char clipboard_files[32][256];  // Full paths of copied/cut files
extern int clipboard_file_count;

// Progress state
extern volatile bool progress_update_pending;
extern volatile size_t pending_bytes_written;
extern volatile size_t pending_total_bytes;
extern char pending_step_description[128];
extern bool flashing_in_progress;

// Boot screen state
extern bool boot_screen_active;
extern bool should_show_main;

#endif // GUI_STATE_H