#include "gui_state.h"

// State variables
char current_directory[512] = "/sdcard";
file_entry_t current_entries[32];
int current_entry_count = 0;
firmware_info_t firmware_files[16];
int firmware_count = 0;
int selected_firmware = -1;

// Progress state
volatile bool progress_update_pending = false;
volatile size_t pending_bytes_written = 0;
volatile size_t pending_total_bytes = 0;
char pending_step_description[128] = {0};
bool flashing_in_progress = false;