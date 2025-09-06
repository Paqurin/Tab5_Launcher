#include "gui_state.h"

// State variables
char current_directory[512] = "/";
file_entry_t current_entries[32];
int current_entry_count = 0;
firmware_info_t firmware_files[16];
int firmware_count = 0;
int selected_firmware = -1;

// File selection state
bool file_selection_enabled = false;
bool selected_files[32] = {false};  // Initialize all to false
int selected_file_count = 0;

// Clipboard state
bool clipboard_has_content = false;
bool clipboard_is_cut = false;  // true for cut/move, false for copy
char clipboard_files[32][256] = {0};  // Full paths of copied/cut files
int clipboard_file_count = 0;

// Progress state
volatile bool progress_update_pending = false;
volatile size_t pending_bytes_written = 0;
volatile size_t pending_total_bytes = 0;
char pending_step_description[128] = {0};
bool flashing_in_progress = false;

// Boot screen state
bool boot_screen_active = false;
bool should_show_main = false;