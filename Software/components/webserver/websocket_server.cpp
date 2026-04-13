#include "websocket_server.hpp"
#include "task_handles.hpp"
#include <array>
#include <esp_check.h>
#include <esp_log.h>
#include <optional>
#include <span>

static constexpr const char *kTagWebSocketServer = "websocket_server";

WebsocketServer::WebsocketServer(httpd_handle_t server, LedFrame &frame) : m_server(server), r_frame(frame) {}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
esp_err_t WebsocketServer::run() {
	if (m_server == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}

	// 1. WebSocket URI registrieren
	httpd_uri_t ws_uri = {};
	ws_uri.uri = "/ws";
	ws_uri.method = HTTP_GET;
	ws_uri.handler = ws_handler;
	ws_uri.user_ctx = this; // Instanz als context
	ws_uri.is_websocket = true;

	ESP_RETURN_ON_ERROR(httpd_register_uri_handler(m_server, &ws_uri), kTagWebSocketServer, "Failed to register WebSocket URI");

	// Task starten
	BaseType_t task_ret = xTaskCreate(ws_broadcast_task, "ws_broadcast_task", TaskHandle::kStackSizeWebSocket, this,
									  TaskHandle::kPrioWebsocket, // Priorität
									  &TaskHandle::x_websocket_task_handle);

	if (task_ret != pdPASS) {
		ESP_LOGE(kTagWebSocketServer, "Failed to create WebSocket broadcast task");
		return ESP_FAIL;
	}

	ESP_LOGI(kTagWebSocketServer, "WebsocketServer successfully started");
	return ESP_OK;
}

esp_err_t WebsocketServer::update_simulation() {
	if (m_server == nullptr) {
		return ESP_ERR_INVALID_STATE; // Klarer Fehler: Server läuft nicht
	}

	size_t max_clients = 10;
	std::array<int, 10> client_fds;
	int *client_fds_ptr = client_fds.data();

	// Fehler beim Abrufen der Client-Liste direkt weitergeben
	esp_err_t err = httpd_get_client_list(m_server, &max_clients, client_fds_ptr);
	if (err != ESP_OK) {
		return err;
	}

	if (max_clients == 0) {
		return ESP_OK; // Kein Fehler, aber auch nichts zu tun
	}

	// Lock holen und Daten senden
	{
		LedFrame::ScopedReadLock read_lock(r_frame);
		auto byte_span = std::as_writable_bytes(std::span{r_frame.led_data});
		httpd_ws_frame_t ws_pkt = {};
		ws_pkt.type = HTTPD_WS_TYPE_BINARY;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		ws_pkt.payload = reinterpret_cast<uint8_t *>(byte_span.data());
		ws_pkt.len = byte_span.size();
		ws_pkt.final = true;

		esp_err_t send_err = ESP_OK;

		std::span<const int, 10> all_clients{client_fds};
		auto active_clients = all_clients.first(max_clients);

		for (const int socket_fd : active_clients) {
			if (httpd_ws_get_fd_info(m_server, socket_fd) == HTTPD_WS_CLIENT_WEBSOCKET) {
				esp_err_t ret = httpd_ws_send_frame_async(m_server, socket_fd, &ws_pkt);

				if (ret != ESP_OK) {
					// Wir loggen den Fehler, machen aber mit den anderen Clients weiter
					ESP_LOGW(kTagWebSocketServer, "Failed to send to client FD %d: %s", socket_fd, esp_err_to_name(ret));
					send_err = ret;
				}
			}
		}

		// Gibt entweder ESP_OK oder den letzten aufgetretenen Sende-Fehler zurück
		return send_err;
	}
}

void WebsocketServer::ws_broadcast_task(void *arg) {
	auto *ws_server = static_cast<WebsocketServer *>(arg);

	while (true) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Sind neue Daten da wird Task aufgeweckt
		ws_server->update_simulation();
	}
}

esp_err_t WebsocketServer::ws_handler(httpd_req_t *req) {
	if (req->method == HTTP_GET) {
		ESP_LOGI(kTagWebSocketServer, "Handshake done, new WebSocket connection opened");
		return ESP_OK;
	}

	// Hier könnten Befehle von der Simulationsseite ZUM ESP empfangen werden
	httpd_ws_frame_t ws_pkt = {};
	esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
	if (ret != ESP_OK) {
		return ret;
	}
	// Optional: Logik zum Empfangen von Daten hinzufügen

	return ESP_OK;
}

esp_err_t WebsocketServer::stop() {
	ESP_LOGI(kTagWebSocketServer, "Stopping WebsocketServer...");

	// Broadcast-Task stoppen
	if (TaskHandle::x_websocket_task_handle != nullptr) {
		vTaskDelete(TaskHandle::x_websocket_task_handle);
		TaskHandle::x_websocket_task_handle = nullptr;
	}

	// m_server wird vom übergeordneten Webserver aufgeräumt,
	m_server = nullptr;

	return ESP_OK;
}