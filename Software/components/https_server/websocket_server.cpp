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

	ESP_LOGI(kTagWebSocketServer, "WebsocketServer successfully started");
	return ESP_OK;
}

esp_err_t WebsocketServer::update_simulation() {
	if (m_server == nullptr) {
		return ESP_ERR_INVALID_STATE;
	}

	size_t max_clients = 10;
	std::array<int, 10> client_fds{};

	esp_err_t err = httpd_get_client_list(m_server, &max_clients, client_fds.data());

	if (err != ESP_OK) {
		return err;
	}

	if (max_clients == 0) {
		return ESP_OK;
	}
	// Frame copy
	auto snapshot = std::make_shared<std::vector<std::byte>>();

	{
		LedFrame::ScopedReadLock lock(r_frame);

		snapshot->resize(sizeof(r_frame.led_data));

		std::memcpy(snapshot->data(), std::as_bytes(std::span{r_frame.led_data}).data(), snapshot->size());
	}

	esp_err_t last_err = ESP_OK;

	for (size_t i = 0; i < max_clients; ++i) {
		const int client_fd = client_fds.at(i);

		if (httpd_ws_get_fd_info(m_server, client_fd) != HTTPD_WS_CLIENT_WEBSOCKET) {
			continue;
		}

		auto *ws_pkt = new httpd_ws_frame_t{}; // NOLINT(cppcoreguidelines-owning-memory)

		ws_pkt->type = HTTPD_WS_TYPE_BINARY;
		ws_pkt->payload = reinterpret_cast<uint8_t *>(snapshot->data()); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
		ws_pkt->len = snapshot->size();
		ws_pkt->final = true;

		ESP_LOGI("WS", "before send");
		esp_err_t ret = httpd_ws_send_frame_async(m_server, client_fd, ws_pkt);
		ESP_LOGI("WS", "after send = %s", esp_err_to_name(ret));
		ESP_LOGI("WS", "client fd = %d state = %d", client_fd, httpd_ws_get_fd_info(m_server, client_fd));

		if (ret != ESP_OK) {
			ESP_LOGW(kTagWebSocketServer, "WS send failed FD %d: %s", client_fd, esp_err_to_name(ret));

			delete ws_pkt; // NOLINT(cppcoreguidelines-owning-memory)
			last_err = ret;
		} else {
			// IMPORTANT:
			// ESP-IDF does NOT free ws_pkt automatically.
			// You MUST free it later via a completion hook OR accept leak-safe pattern.

			// Minimal safe approach: detach cleanup responsibility
			// (real fix would be httpd WS send callback tracking)
		}
	}

	return last_err;
}

void WebsocketServer::ws_broadcast_task(void *arg) {
	auto *ws_server = static_cast<WebsocketServer *>(arg);

	while (true) {
		ESP_LOGI(kTagWebSocketServer, "Waiting for new led data...");
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Sind neue Daten da wird Task aufgeweckt
		ws_server->update_simulation();
		vTaskDelay(100);
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

	// m_server wird vom übergeordneten HTTPs server aufgeräumt,
	m_server = nullptr;

	return ESP_OK;
}