#include "webserver.hpp"
#include "routes.hpp"
#include <esp_check.h>
#include <esp_eth.h>
#include <esp_log.h>
#include <esp_wifi.h>

static const char *TAG = "webserver";

httpd_handle_t g_webserver = nullptr;

/**
 * Starts the webserver and registers the routes.
 *
 * @retval - `ESP_OK`: Succeed
 * @retval - `ESP_ERR_INVALID_ARG`: Null argument(s)
 * @retval - `ESP_ERR_HTTPD_ALLOC_MEM`: Failed to allocate memory for instance
 * @retval - `ESP_ERR_HTTPD_TASK`: Failed to launch server task
 * @retval - `ESP_ERR_HTTPD_HANDLERS_FULL`: No slots left for new handler
 * @retval - `ESP_ERR_HTTPD_HANDLER_EXISTS`: Handler with same URI and method already registered
 *
 * For more details, see:
 *
 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
 */
esp_err_t start_webserver(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	// Allows wild card matching for routes
	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

	ESP_RETURN_ON_ERROR(httpd_start(&g_webserver, &config), TAG, "Failed to start http server");

	ESP_RETURN_ON_ERROR(register_routes(g_webserver), TAG, "Failed to register routes");

	return ESP_OK;
}

/**
 * Stops the webserver
 *
 * @retval - `ESP_OK`: Succeed
 * @retval - `ESP_ERR_INVALID_ARG`: Handle argument is Null
 *
 * For more details, see:
 *
 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
 */
esp_err_t stop_webserver(void) {
	// There is no running webserver instance
	if (g_webserver == nullptr) {
		return ESP_OK;
	}

	ESP_LOGI(TAG, "Stopping webserver...");

	ESP_RETURN_ON_ERROR(httpd_stop(g_webserver), TAG, "Failed to stop http server");
	g_webserver = nullptr;
	return ESP_OK;
}

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
static void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (start_webserver() != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start webserver");
	} else {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "Webserver available at: http://" IPSTR, IP2STR(&event->ip_info.ip));
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
static void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ESP_ERROR_CHECK_WITHOUT_ABORT(stop_webserver());
}

esp_err_t init_webserver(void) {
	esp_err_t ret = ESP_ERR_INVALID_STATE;
	ESP_LOGI(TAG, "Initialize webserver...");

#if CONFIG_LISTEN_ETHERNET
	ret = ESP_OK;
	ESP_LOGI(TAG, "Webserver listening to ethernet connection...");
	ESP_LOGW(TAG, "Ethernet is not implemented yet.");
#endif
#if CONFIG_LISTEN_WIFI
	ret = ESP_OK;
	ESP_LOGI(TAG, "Webserver listening to wifi connection...");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, nullptr), TAG,
						"Failed to register WiFi connect handler");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, nullptr), TAG,
						"Failed to register WiFi disconnect handler");
#endif

	return ret;
}
