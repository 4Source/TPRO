#include "blinking_effect.hpp"
#include "config_manager.hpp"
#include "file_manager.hpp"
#include "light_effect_manager.hpp"
#include "network.hpp"
#include "restserver.hpp"
#include "timeserver.hpp"
#include "webserver.hpp"
#include "websocket_server.hpp"
#include <esp_log.h>
#include <esp_wifi.h>
#include <nvs_flash.h>
#include <optional>

static constexpr const char *kTag = "main";

// TODO: Muss wirklich alles davon global sein?
// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
std::optional<WebsocketServer> g_ws_server;
static LedFrame main_frame;
static ConfigManager main_config;
static LightEffectManager effect_manager(main_frame /*, &main_config*/);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

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
 * This helper function configures the webserver and websocket server.
 */
static void handle_websocket_state(httpd_handle_t handle) {
	if (handle != nullptr) {
		ESP_LOGI(kTag, "Webserver handle received, starting WebSocket...");
		g_ws_server.emplace(handle, main_frame);
		g_ws_server->run();
	} else {
		ESP_LOGW(kTag, "Webserver handle lost, stopping WebSocket...");
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
	 * This helper function starts the webserver
	 */
	ESP_ERROR_CHECK(Webserver::init([&](httpd_handle_t handle) {
		handle_websocket_state(handle);
		RestServer::init(main_config, handle);
	}));

	/*
	 * This helper function starts Wi-Fi or Ethernet, as configured above.
	 */
	// TODO: Proper error handling, currently the application will not launch
	ESP_ERROR_CHECK(Network::connect());

	init_timeserver();

	// Test Blink
	BlinkingEffect blink;
	effect_manager.register_effect(&blink);
	effect_manager.set_effect(&blink);
	effect_manager.start();

	// Run Selftest for FileManager
	if (FileManager::run_selftest() != ESP_OK) {
		ESP_LOGE("main", "File manager self-test failed");
		return;
	}

	while (true) {
		// Delay to simulate load
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
