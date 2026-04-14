#include "webserver.hpp"
#include "routes.hpp"
#include <cstring>
#include <esp_check.h>
#include <esp_eth.h>
#include <esp_log.h>
#include <esp_wifi.h>

// This is for clang tidy when not configured the files still get analyzed and than have missing defines
#ifdef __clang__
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifndef CONFIG_LISTEN_ETHERNET
#define CONFIG_LISTEN_ETHERNET y
#endif
#ifndef CONFIG_LISTEN_WIFI
#define CONFIG_LISTEN_WIFI y
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif

static constexpr const char *kTagWebServer = "webserver";

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::function<void(httpd_handle_t)> g_state_callback;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
httpd_handle_t g_webserver = nullptr;

/**
 * Event handler for got IP events. When a got IP event is received this handler will start the webserver
 *
 * This function is registered to the `default event loop` and is called whenever an event is posted to the specified event bases.
 *
 * @param arg User-provided arguments passed during handler registration. Can be `nullptr` if not used.
 * @param event_base Event base identifying the event source.
 * @param event_id Event identifier specific to the event base.
 * @param event_data Pointer to event specific data structure. The actual type depends on the `event_base` and `event_id`.
 *
 * For more details, see:
 *
 * - [ESP-IDF Event Loop Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/system/esp_event.html)
 */
static void connect_handler(void *arg_0, esp_event_base_t event_base, int32_t event_id, void *arg_1) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.uri_match_fn = httpd_uri_match_wildcard;

	if (httpd_start(&g_webserver, &config) == ESP_OK) {
		ESP_LOGI(kTagWebServer, "Server started");
		register_routes(g_webserver);

		if (g_state_callback) {
			g_state_callback(g_webserver);
		}
	}
}

/**
 * Event handler for disconnected events. When a disconnected event is received this handler will stop+ the webserver
 *
 * This function is registered to the `default event loop` and is called whenever an event is posted to the specified event bases.
 *
 * @param arg User-provided arguments passed during handler registration. Can be `nullptr` if not used.
 * @param event_base Event base identifying the event source.
 * @param event_id Event identifier specific to the event base.
 * @param event_data Pointer to event specific data structure. The actual type depends on the `event_base` and `event_id`.
 *
 * For more details, see:
 *
 * - [ESP-IDF Event Loop Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/system/esp_event.html)
 */

static void disconnect_handler(void *arg_0, esp_event_base_t event_base, int32_t event_id, void *arg_1) {
	// TODO: Maybe if both (ethernet/wifi) are connected the disconnect_handler should not stop the webserver
	ESP_LOGI(kTagWebServer, "Server stopping...");

	// Melldung
	if (g_state_callback) {
		g_state_callback(nullptr);
	}

	if (g_webserver != nullptr) {
		httpd_stop(g_webserver);
		g_webserver = nullptr;
	}
}

esp_err_t init_webserver(const std::function<void(httpd_handle_t)> &callback) {
	esp_err_t ret = ESP_ERR_INVALID_STATE;
	ESP_LOGI(kTagWebServer, "Initializing webserver events...");
	g_state_callback = callback;

#if CONFIG_LISTEN_ETHERNET
	ret = ESP_OK;
	ESP_LOGI(kTagWebServer, "Webserver listening to ethernet connection...");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, nullptr), kTagWebServer,
						"Failed to register Ethernet connect handler");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, nullptr), kTagWebServer,
						"Failed to register Ethernet disconnect handler");
#endif
#if CONFIG_LISTEN_WIFI
	ret = ESP_OK;
	ESP_LOGI(kTagWebServer, "Webserver listening to wifi connection...");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, nullptr), kTagWebServer,
						"Failed to register WiFi connect handler");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, nullptr), kTagWebServer,
						"Failed to register WiFi disconnect handler");
#endif

	return ret;
}
