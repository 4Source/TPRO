#include "led_frame.hpp"
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class WebsocketServer {
  public:
	/**
	 * @brief Konstruktor für den Websocket Server
	 * @param server Der ESP-IDF httpd_handle_t deines Webservers
	 * @param frame Referenz auf das zentrale LedFrame
	 */
	WebsocketServer(httpd_handle_t server, LedFrame &frame);

	/**
	 * @brief Registriert den URI-Handler und startet den Background-Task zum Senden der Daten
	 * @retval ESP_OK bei Erfolg
	 */
	esp_err_t run();
	esp_err_t stop();

#ifdef UNIT_TEST
  public:
#else
  private:
#endif
	httpd_handle_t m_server;
	LedFrame &r_frame;

	/**
	 * @brief Holt die aktuellen Clients und sendet das LED-Array als Binary
	 */
	esp_err_t update_simulation();

	// FreeRTOS Task-Funktion
	static void ws_broadcast_task(void *arg);

	// ESP-IDF HTTP Handler für den WebSocket
	static esp_err_t ws_handler(httpd_req_t *req);
};
