#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
static constexpr const char *kTag = "main";

extern "C" void app_main(void) {
	// Only used for pytest_boot
	ESP_LOGI(kTag, "Clock Slave startup");

	while (true) {
		// Delay to simulate load
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}