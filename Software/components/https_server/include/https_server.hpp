#pragma once

#include <esp_err.h>
#include <esp_https_server.h>
#include <functional>

class HttpsServer {
  public:
	/**
	 * Registers event handlers to network connections WiFi and/or Ethernet which start the HTTPs server if successfully created a connection.
	 *
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_INVALID_STATE`: Neither WiFi nor Ethernet are enabled in menuconfig.
	 * @retval - `ESP_ERR_NO_MEM`: Cannot allocate memory for the handler
	 * @retval - `ESP_ERR_INVALID_ARG`: Invalid combination of event base and event ID
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF HTTP Server Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_http_server.html)
	 *
	 * - [ESP-IDF HTTPs Server
	 * Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/protocols/esp_https_server.html)
	 *
	 * - [ESP-IDF WiFi Driver
	 * Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/wifi-driver/station-scenarios.html)
	 *
	 * - [ESP-IDF Ethernet Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/network/esp_eth.html)
	 */
	static esp_err_t init(const std::function<void(httpd_handle_t)> &state_callback);

	/**
	 * For debug purposes logs the active socket connections
	 */
	static void log_active_socket_connections();

	/**
	 * Converts the `httpd_method_t` to the corrsponding string
	 *
	 * @param method The method to convert
	 * @returns The string representation
	 */
	static constexpr const char *http_method_to_str(httpd_method_t method) {
		switch (method) {
		case HTTP_GET:
			return "GET";
		case HTTP_POST:
			return "POST";
		case HTTP_PUT:
			return "PUT";
		case HTTP_DELETE:
			return "DELETE";
		case HTTP_PATCH:
			return "PATCH";
		case HTTP_HEAD:
			return "HEAD";
		case HTTP_OPTIONS:
			return "OPTIONS";
		default:
			return "UNKNOWN";
		}
	}

  private:
	/**
	 * Event handler for got IP events. When a got IP event is received this handler will start the HTTPs server
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
	static void connect_handler(void *arg_0, esp_event_base_t event_base, int32_t event_id, void *arg_1);

	/**
	 * Event handler for disconnected events. When a disconnected event is received this handler will stop the HTTPs server
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
	static void disconnect_handler(void *arg_0, esp_event_base_t event_base, int32_t event_id, void *arg_1);

	/**
	 * Event handler for https server events.
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
	static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

	/**
	 * Triggered when an active HTTPS client connection fails due to a TLS error, socket error, timeout, or HTTP processing failure. It indicates a
	 * per-connection failure in the secure communication or request handling path, not a server-wide issue.
	 */
	static void handle_https_server_error(esp_https_server_last_error_t *event);
	/**
	 * Triggered when HTTPS Server is started
	 */
	static void handle_https_server_start();
	/**
	 * Triggered once the HTTPS Server has been connected to the client
	 */
	static void handle_https_server_connected();
	/**
	 * Triggered when receiving data from the client
	 */
	static void handle_https_server_data(int *event);
	/**
	 * Triggered when an ESP HTTPS server sends data to the client
	 */
	static void handle_https_server_sent_data();
	/**
	 * Triggered when the connection has been disconnected
	 */
	static void handle_https_server_disconnected();
	/**
	 * Triggered when HTTPS Server is stopped
	 */
	static void handle_https_server_stop();

	/**
	 * Triggered when there are any errors during execution
	 */
	static void handle_http_server_error(httpd_err_code_t *event);
	/**
	 * Triggered when HTTP Server is started
	 */
	static void handle_http_server_start();
	/**
	 * Triggered once the HTTP Server has been connected to the client, no data exchange has been performed
	 */
	static void handle_http_server_connected(int *event);
	/**
	 * Triggered when receiving each header sent from the client
	 */
	static void handle_http_server_header(int *event);
	/**
	 * Triggered after sending all the headers to the client
	 */
	static void handle_http_server_headers_sent(int *event);
	/**
	 * Triggered when receiving data from the client
	 */
	static void handle_http_server_data(esp_http_server_event_data *event);
	/**
	 * Triggered when an ESP HTTP server session is finished
	 */
	static void handle_http_server_sent_data(esp_http_server_event_data *event);
	/**
	 * Triggered when the connection has been disconnected
	 */
	static void handle_http_server_disconnected(int *event);
	/**
	 * Triggered when HTTP Server is stopped
	 */
	static void handle_http_server_stop();

	static constexpr const char *kTag = "https-server";
	static std::function<void(httpd_handle_t)> g_state_callback;
	static httpd_handle_t g_https_server_handle;
	static constexpr size_t kMaxSockets = CONFIG_LWIP_MAX_SOCKETS - 3;
};
