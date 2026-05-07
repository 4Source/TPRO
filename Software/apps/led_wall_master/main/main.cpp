#include "config_manager.hpp"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "file_manager.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "https_server.hpp"
#include "led_controller.hpp"
#include "light_effect_manager.hpp"
#include "network.hpp"
#include "restserver.hpp"
#include "timeserver.hpp"
#include "websocket_server.hpp"
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <optional>

// TEST Effekte
#include "effect/blinking_effect.hpp"
#include "effect/breathing_effect.hpp"
#include "effect/color_wipe_effect.hpp"
#include "effect/column_scan_effect.hpp"
#include "effect/continent_effect.hpp"
#include "effect/day_night_effect.hpp"
#include "effect/debug_effect.hpp"
#include "effect/dvd_effect.hpp"
#include "effect/equalizer_effect.hpp"
#include "effect/fire_effect.hpp"
#include "effect/matrix_effect.hpp"
#include "effect/plasma_effect.hpp"
#include "effect/rainbow_wave_effect.hpp"
#include "effect/row_scan_effect.hpp"
#include "effect/scrolling_effect.hpp"
#include "effect/static_color_effect.hpp"
#include "effect/timeline_effect.hpp"

static constexpr const char *kTag = "main";

// TODO: Muss wirklich alles davon global sein?
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
std::optional<WebsocketServer> g_ws_server;
static LedFrame main_frame;
static ConfigManager main_config;
static LightEffectManager effect_manager(main_frame /*, &main_config*/);
static LedController controller(main_frame);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

static constexpr const char *kMonitorTag = "MONITOR";

void system_monitor_task(void *pv_parameter) {

	while (true) {
		ESP_LOGI(kMonitorTag, "================ SYSTEM MONITOR ================");

		// 1. Allgemeiner RAM (Dein Snippet)
		ESP_LOGI(kMonitorTag, "Free Heap:       %.1f KB", static_cast<float>(esp_get_free_heap_size()) / 1024.0F);
		ESP_LOGI(kMonitorTag, "Largest Block:   %.1f KB", static_cast<float>(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)) / 1024.0F);

		// 2. DMA-fähiger RAM (Wichtig für dein SPI/Display)
		ESP_LOGI(kMonitorTag, "Free DMA RAM:    %.1f KB", static_cast<float>(heap_caps_get_free_size(MALLOC_CAP_DMA)) / 1024.0F);
		ESP_LOGI(kMonitorTag, "Largest DMA:     %.1f KB", static_cast<float>(heap_caps_get_largest_free_block(MALLOC_CAP_DMA)) / 1024.0F);

		UBaseType_t stack_watermark = uxTaskGetStackHighWaterMark(nullptr);
		ESP_LOGI(kMonitorTag, "Monitor Stack:   %lu Bytes free", static_cast<uint32_t>(stack_watermark * 4));

		ESP_LOGI(kMonitorTag, "================================================");

		vTaskDelay(pdMS_TO_TICKS(60000));
	}
}
/**
 * Needs to be initialized once per app. Will allow persistent storage in the flash.
 *
 * Needed by:
 * - WiFi to store the configuration into flash
 * - Ethernet to store the configuration into flash
 */
static void init_nvs_storage() {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

/*
 * This helper function configures the websocket server.
 */
static void handle_websocket_state(httpd_handle_t handle) {
	if (handle != nullptr) {
		ESP_LOGI(kTag, "HTTPs server handle received, starting WebSocket...");
		g_ws_server.emplace(handle, main_frame);
		g_ws_server->run();
	} else {
		ESP_LOGW(kTag, "HTTPs server handle lost, stopping WebSocket...");
		if (g_ws_server.has_value()) {
			g_ws_server->stop();
			g_ws_server.reset();
		}
	}
}

extern "C" void app_main(void) {
	// Only used for pytest_boot
	ESP_LOGI(kTag, "LED Wall startup");

	init_nvs_storage();

	/*
	 * Initialize TCP/IP stack
	 *
	 * Needed by:
	 * - WiFi
	 * - Ethernet
	 */
	ESP_ERROR_CHECK(esp_netif_init());

	/*
	 * Create the default event loop that runs in the background. There can only be one default event loop
	 *
	 * Needed by:
	 * - WiFi
	 * - Ethernet
	 */
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	/*
	 * This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
	 */
	// TODO: Proper error handling, currently the application will not launch
	ESP_ERROR_CHECK(Network::init());

	/**
	 * This helper function configures the HTTPs server
	 */
	ESP_ERROR_CHECK(HttpsServer::init([&](httpd_handle_t handle) {
		handle_websocket_state(handle);
		RestServer::init(main_config, handle);
	}));

	/*
	 * This helper function starts Wi-Fi or Ethernet, as configured above.
	 */
	// TODO: Proper error handling, currently the application will not launch
	ESP_ERROR_CHECK(Network::connect());

	init_timeserver();

	vTaskDelay(1000);

	// Run Selftest for FileManager
	if (FileManager::run_selftest() != ESP_OK) {
		ESP_LOGE(kTag, "File manager self-test failed");
		return;
	}
	EffectFactory::writeDefaults("/effects/defaults");
	// FileManager::print_default_configs(); Sollte später alle defaults ausprinten

	// // Load Configuration from SD Card
	// if (main_config.deserialize() != ESP_OK) {
	// 	ESP_LOGW(kTag, "Failed to load /config.json, creating default config.");
	// 	main_config.serialize();
	// }

	// Start Effect
	// std::string current_effect_path = ColumnScanEffect::kDefaultConfigPath;
	// std::string current_effect_path = RowScanEffect::kDefaultConfigPath;

	vTaskDelay(100);
	// Effekt Test
	/*
	auto scrolling_effect = std::make_shared<ScrollingEffect>();
	scrolling_effect->set_parameter("text", "Hello World! This is a scrolling text effect demo. :)");
*/

	// TODO:
	// auto main_timeline = EffectFactory::generate_from_json(DVDEffect::kDefaultConfigPath); // Beide X Beide Y & Farbe
	// auto main_timeline = EffectFactory::generate_from_json(FireEffect::kDefaultConfigPath); // Beide X Beide Y & Farbe

	// auto main_timeline = EffectFactory::generate_from_json(EqualizerEffect::kDefaultConfigPath); // Beide X Beide Y & Farbe

	// auto main_timeline = EffectFactory::generate_from_json(ContinentEffect::kDefaultConfigPath);
	// auto main_timeline = EffectFactory::generate_from_json("/effects/timeline_continent.json");
	auto main_timeline = EffectFactory::generate_from_json("/effects/timeline_full.json");
	// auto main_timeline = EffectFactory::generate_from_json(DayNightEffect::kDefaultConfigPath);
	effect_manager.register_effect(main_timeline);
	effect_manager.set_effect(main_timeline);
	effect_manager.start();

	vTaskDelay(100);
	// Start the Led controller task
	if (controller.run() != ESP_OK) {
		ESP_LOGE(kTag, "Failed to setup the LED controller!");
	}

	// Starte Monitor Task
	xTaskCreatePinnedToCore(system_monitor_task, "sys_monitor", 4096, nullptr, 1, nullptr, tskNO_AFFINITY);

	while (true) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
