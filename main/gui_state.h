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

// Progress state
extern volatile bool progress_update_pending;
extern volatile size_t pending_bytes_written;
extern volatile size_t pending_total_bytes;
extern char pending_step_description[128];
extern bool flashing_in_progress;

#endif // GUI_STATE_H