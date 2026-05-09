#include "websocket_server.hpp"
#include "task_handles.hpp"
#include <array>
#include <esp_check.h>
#include <esp_log.h>
#include <memory>
#include <optional>
#include <span>
#include <vector>

static constexpr const char *kTagWebSocketServer = "websocket_server";

std::array<uint8_t, (3 * CONFIG_LED_COLUMNS) * (CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS)> WebsocketServer::snapshot{};

WebsocketServer::WebsocketServer(httpd_handle_t server, LedFrame &frame) : m_server(server), r_frame(frame) {}

esp_err_t WebsocketServer::run() {
	if (m_server == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}

	// Task starten
	BaseType_t task_ret = xTaskCreate(ws_broadcast_task, "ws_broadcast_task", TaskHandle::kStackSizeWebSocket, this,
									  TaskHandle::kPrioWebsocket, // Priorität
									  &TaskHandle::x_websocket_task_handle);

	if (task_ret != pdPASS) {
		ESP_LOGE(kTagWebSocketServer, "Failed to create WebSocket broadcast task");
		return ESP_FAIL;
	}

	ESP_LOGD(kTagWebSocketServer, "WebsocketServer successfully started");
	return ESP_OK;
}

esp_err_t WebsocketServer::update_simulation() {
	if (m_server == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}

	size_t max_clients = 10;
	std::array<int, 10> client_fds{};
	if (httpd_get_client_list(m_server, &max_clients, client_fds.data()) != ESP_OK) {
		return ESP_FAIL;
	}

	if (max_clients == 0) {
		return ESP_OK;
	}

	// Snapshot der LED-Daten
	{
		LedFrame::ScopedReadLock lock(r_frame);

		// NOLINTNEXTLINE[cppcoreguidelines-pro-type-reinterpret-cast]
		const auto *data_ptr = reinterpret_cast<const uint8_t *>(r_frame.led_data.data());
		constexpr size_t data_size = sizeof(r_frame.led_data);
		std::memcpy(snapshot.data(), data_ptr, data_size);
	}

	for (size_t i = 0; i < max_clients; ++i) {
		int file_descriptor = client_fds[i];

		if (httpd_ws_get_fd_info(m_server, file_descriptor) != HTTPD_WS_CLIENT_WEBSOCKET) {
			continue;
		}

		httpd_ws_frame_t ws_pkt = {};
		ws_pkt.type = HTTPD_WS_TYPE_BINARY;
		ws_pkt.payload = snapshot.data();
		ws_pkt.len = snapshot.size();
		ws_pkt.final = true;

		esp_err_t ret = httpd_ws_send_data(m_server, file_descriptor, &ws_pkt);

		if (ret != ESP_OK) {
			ESP_LOGW(kTagWebSocketServer, "WS send failed FD %d: %s", file_descriptor, esp_err_to_name(ret));
		}
	}

	return ESP_OK;
}

void WebsocketServer::ws_broadcast_task(void *arg) {
	auto *ws_server = static_cast<WebsocketServer *>(arg);

	while (true) {
		ESP_LOGD(kTagWebSocketServer, "Waiting for new led data...");
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Sind neue Daten da wird Task aufgeweckt
		ws_server->update_simulation();
		vTaskDelay(2);
	}
}

esp_err_t WebsocketServer::ws_handler(httpd_req_t *req) {
	if (req->method == HTTP_GET) {
		ESP_LOGD(kTagWebSocketServer, "Handshake done, new WebSocket connection opened");
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
	ESP_LOGD(kTagWebSocketServer, "Stopping WebsocketServer...");

	// Broadcast-Task stoppen
	if (TaskHandle::x_websocket_task_handle != nullptr) {
		vTaskDelete(TaskHandle::x_websocket_task_handle);
		TaskHandle::x_websocket_task_handle = nullptr;
	}

	// m_server wird vom übergeordneten HTTPs server aufgeräumt,
	m_server = nullptr;

	return ESP_OK;
}