#pragma once

#include "esp_err.h"
#include <cstddef>
#include <cstdint>

// 1. KORREKTUR: httpd_req_t ist ein STRUCT, kein Pointer!
typedef struct httpd_req {
	int method;
} httpd_req_t;

// Server-Handle bleibt ein Pointer
typedef void *httpd_handle_t;

// --- Konstanten ---
#define HTTP_GET 1
#define HTTPD_WS_TYPE_BINARY 0x02
#define HTTPD_WS_CLIENT_WEBSOCKET 1

// --- Structs ---
typedef struct {
	uint8_t *payload;
	size_t len;
	uint8_t type;
	bool final;
} httpd_ws_frame_t;

typedef struct {
	const char *uri;
	int method;
	// 2. KORREKTUR: Handler erwartet einen Pointer (*req) auf das Struct
	esp_err_t (*handler)(httpd_req_t *req);
	void *user_ctx;
	bool is_websocket;
} httpd_uri_t;

// --- Funktions-Mocks ---
inline esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri) { return ESP_OK; }

// 3. KORREKTUR: Erwartet einen Pointer (*req)
inline esp_err_t httpd_ws_recv_frame(httpd_req_t *req, httpd_ws_frame_t *pkt, size_t max_len) { return ESP_OK; }

inline esp_err_t httpd_get_client_list(httpd_handle_t s, size_t *n, int *fds) {
	*n = 1;
	fds[0] = 42;
	return ESP_OK;
}

inline int httpd_ws_get_fd_info(httpd_handle_t s, int fd) { return HTTPD_WS_CLIENT_WEBSOCKET; }

// --- Globale Tracking-Variablen für den Test ---
extern "C" {
extern size_t last_send_len;
extern uint8_t *last_send_payload;
}

inline esp_err_t httpd_ws_send_frame_async(httpd_handle_t s, int fd, httpd_ws_frame_t *pkt) {
	if (pkt) {
		last_send_len = pkt->len;
		last_send_payload = pkt->payload;
	}
	return ESP_OK;
}