#include "gui_screens.h"
#include "esp_log.h"

static const char *TAG = "GUI_SCREENS";

void gui_screens_init(void) {
    ESP_LOGI(TAG, "Initializing all GUI screens");
    create_main_screen();
    create_file_manager_screen();
    create_firmware_loader_screen();
    create_progress_screen();
    create_splash_screen();
    create_reboot_dialog_screen();
    ESP_LOGI(TAG, "All GUI screens initialized");
}