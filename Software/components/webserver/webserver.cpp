#include "webserver.hpp"
#include "embedded_files.hpp"
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

httpd_handle_t Webserver::g_https_server_handle = nullptr;

std::function<void(httpd_handle_t)> Webserver::g_state_callback = nullptr;

esp_err_t Webserver::init(const std::function<void(httpd_handle_t)> &callback) {
	esp_err_t ret = ESP_ERR_INVALID_STATE;
	ESP_LOGI(kTag, "Initializing webserver events...");
	g_state_callback = callback;

#if CONFIG_LISTEN_ETHERNET
	ret = ESP_OK;
	ESP_LOGI(kTag, "Webserver listening to ethernet connection...");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, nullptr), kTag,
						"Failed to register Ethernet connect handler");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, nullptr), kTag,
						"Failed to register Ethernet disconnect handler");
#endif
#if CONFIG_LISTEN_WIFI
	ret = ESP_OK;
	ESP_LOGI(kTag, "Webserver listening to wifi connection...");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, nullptr), kTag,
						"Failed to register WiFi connect handler");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, nullptr), kTag,
						"Failed to register WiFi disconnect handler");
#endif

	ESP_RETURN_ON_ERROR(esp_event_handler_register(ESP_HTTP_SERVER_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr), kTag,
						"Failed to register https server events handler");

	ESP_RETURN_ON_ERROR(esp_event_handler_register(ESP_HTTPS_SERVER_EVENT, ESP_EVENT_ANY_ID, &event_handler, nullptr), kTag,
						"Failed to register https server events handler");

	return ret;
}

void Webserver::log_active_socket_connections() {
	size_t https_fds = kMaxSockets;
	std::array<int, kMaxSockets> https_client_fds;
	httpd_get_client_list(g_https_server_handle, &https_fds, https_client_fds.data());

	if (https_fds >= kMaxSockets - 2) {
		ESP_LOGW(kTag, "HTTPs %d active socket clients", https_fds);
	} else {
		ESP_LOGI(kTag, "HTTPs %d active socket clients", https_fds);
	}
	for (size_t i = 0; i < https_fds; ++i) {
		ESP_LOGD(kTag, "\tHTTPs socket client: %d", https_client_fds.at(i));
	}
}

void Webserver::connect_handler(void *arg_0, esp_event_base_t event_base, int32_t event_id, void *arg_1) {
	ESP_LOGI(kTag, "Starting server");

	// https server
	httpd_ssl_config_t https_config = HTTPD_SSL_CONFIG_DEFAULT();
	https_config.servercert = &server_cert_pem_start[0];
	https_config.servercert_len = server_cert_pem_size;

	https_config.prvtkey_pem = &server_key_pem_start[0];
	https_config.prvtkey_len = server_key_pem_size;

	https_config.httpd.uri_match_fn = httpd_uri_match_wildcard;
	// W5500 Supports a max of 8 socket connections and 3 are for internal use therefore CONFIG_LWIP_MAX_SOCKETS should be a total of 11
	https_config.httpd.max_open_sockets = kMaxSockets;
	// Close least recently used connection: when connection requested but max_open_sockets already reached
	https_config.httpd.lru_purge_enable = true;

	https_config.httpd.max_uri_handlers = 10; // default is 8 but we have more routes to register

	if (httpd_ssl_start(&g_https_server_handle, &https_config) == ESP_OK) {
		register_https_routes(g_https_server_handle);

		if (g_state_callback) {
			g_state_callback(g_https_server_handle);
		}
	}
}

void Webserver::disconnect_handler(void *arg_0, esp_event_base_t event_base, int32_t event_id, void *arg_1) {
	// TODO: Maybe if both (ethernet/wifi) are connected the disconnect_handler should not stop the webserver
	ESP_LOGI(kTag, "Server stopping...");

	// Melldung
	if (g_state_callback) {
		g_state_callback(nullptr);
	}

	if (g_https_server_handle != nullptr) {
		httpd_ssl_stop(g_https_server_handle);
		g_https_server_handle = nullptr;
	}
}

void Webserver::event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (event_base == ESP_HTTPS_SERVER_EVENT) {
		switch (event_id) {
		case HTTPS_SERVER_EVENT_ERROR:
			handle_https_server_error(static_cast<esp_https_server_last_error_t *>(event_data));
			break;
		case HTTPS_SERVER_EVENT_START:
			handle_https_server_start();
			break;
		case HTTPS_SERVER_EVENT_ON_CONNECTED:
			handle_https_server_connected();
			break;
		case HTTPS_SERVER_EVENT_ON_DATA:
			handle_https_server_data(static_cast<int *>(event_data));
			break;
		case HTTPS_SERVER_EVENT_SENT_DATA:
			handle_https_server_sent_data();
			break;
		case HTTPS_SERVER_EVENT_DISCONNECTED:
			handle_https_server_disconnected();
			break;
		case HTTPS_SERVER_EVENT_STOP:
			handle_https_server_stop();
			break;

		default:
			break;
		}
	} else if (event_base == ESP_HTTP_SERVER_EVENT) {
		switch (event_id) {

		case HTTP_SERVER_EVENT_ERROR:
			handle_http_server_error(static_cast<httpd_err_code_t *>(event_data));
			break;
		case HTTP_SERVER_EVENT_START:
			handle_http_server_start();
			break;
		case HTTP_SERVER_EVENT_ON_CONNECTED:
			handle_http_server_connected(static_cast<int *>(event_data));
			break;
		case HTTP_SERVER_EVENT_ON_HEADER:
			handle_http_server_header(static_cast<int *>(event_data));
			break;
		case HTTP_SERVER_EVENT_HEADERS_SENT:
			handle_http_server_headers_sent(static_cast<int *>(event_data));
			break;
		case HTTP_SERVER_EVENT_ON_DATA:
			handle_http_server_data(static_cast<esp_http_server_event_data *>(event_data));
			break;
		case HTTP_SERVER_EVENT_SENT_DATA:
			handle_http_server_sent_data(static_cast<esp_http_server_event_data *>(event_data));
			break;
		case HTTP_SERVER_EVENT_DISCONNECTED:
			handle_http_server_disconnected(static_cast<int *>(event_data));
			break;
		case HTTP_SERVER_EVENT_STOP:
			handle_http_server_stop();
			break;

		default:
			break;
		}
	}
}

void Webserver::handle_https_server_error(esp_https_server_last_error_t *event) {
	ESP_LOGE(kTag, "Error event triggered: last_error = %s, last_tls_err = %d, tls_flag = %d", esp_err_to_name(event->last_error),
			 event->esp_tls_error_code, event->esp_tls_flags);
	Webserver::log_active_socket_connections();
}

void Webserver::handle_https_server_start() {
	ESP_LOGI(kTag, "HTTPs server start");
	Webserver::log_active_socket_connections();
}

void Webserver::handle_https_server_connected() {
	ESP_LOGI(kTag, "HTTPs server connected");
	Webserver::log_active_socket_connections();
}

void Webserver::handle_https_server_data(int *event) { ESP_LOGD(kTag, "HTTPs server data %d", event); }

void Webserver::handle_https_server_sent_data() { ESP_LOGD(kTag, "HTTPs server sent data"); }

void Webserver::handle_https_server_disconnected() {
	ESP_LOGI(kTag, "HTTPs server disconnected");
	Webserver::log_active_socket_connections();
}

void Webserver::handle_https_server_stop() {
	ESP_LOGI(kTag, "HTTPs server stop");
	Webserver::log_active_socket_connections();
}

void Webserver::handle_http_server_error(httpd_err_code_t *event) {
	ESP_LOGE(kTag, "Error event triggered: %d", event);
	Webserver::log_active_socket_connections();
}

void Webserver::handle_http_server_start() {
	ESP_LOGI(kTag, "HTTP server start");
	Webserver::log_active_socket_connections();
}

void Webserver::handle_http_server_connected(int *event) {
	ESP_LOGI(kTag, "HTTP server connected %d", event);
	Webserver::log_active_socket_connections();
}

void Webserver::handle_http_server_header(int *event) { ESP_LOGD(kTag, "HTTP server header %d", event); }

void Webserver::handle_http_server_headers_sent(int *event) { ESP_LOGD(kTag, "HTTP server headers sent %d", event); }

void Webserver::handle_http_server_data(esp_http_server_event_data *event) {
	ESP_LOGD(kTag, "HTTP server data, fd=%d len=%d", event->fd, event->data_len);
}

void Webserver::handle_http_server_sent_data(esp_http_server_event_data *event) {
	ESP_LOGD(kTag, "HTTP server sent data, fd=%d len=%d", event->fd, event->data_len);
}

void Webserver::handle_http_server_disconnected(int *event) {
	ESP_LOGI(kTag, "HTTP server disconnected %d", event);
	Webserver::log_active_socket_connections();
}

void Webserver::handle_http_server_stop() {
	ESP_LOGI(kTag, "HTTP server stop");
	Webserver::log_active_socket_connections();
}
