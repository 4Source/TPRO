#pragma once
#include <esp_err.h>
#include <esp_netif_types.h>
#include <esp_wifi.h>

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_CONNECTION_FAILED_BIT BIT1

class WifiNetwork {
  public:
	/**
	 * Configures the WiFi network connection, as selected in menuconfig.
	 *
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_NO_MEM`: Cannot allocate memory
	 * @retval - `ESP_ERR_INVALID_ARG`: invalid argument
	 * @retval - `ESP_ERR_WIFI_NOT_INIT`: WiFi is not initialized by esp_wifi_init
	 * @retval - `ESP_ERR_WIFI_IF`: invalid interface
	 * @retval - `ESP_ERR_WIFI_MODE`: invalid mode
	 * @retval - `ESP_ERR_WIFI_PASSWORD`: invalid password
	 * @retval - `ESP_ERR_WIFI_NVS`: WiFi internal NVS error
	 * @retval - `ESP_ERR_WIFI_STATE`: WiFi still connecting when invoke esp_wifi_set_config
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF WiFi Driver
	 * Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/wifi-driver/station-scenarios.html)
	 */
	static esp_err_t init();

	/**
	 * Start the WiFi network connection. Should be configured first with init.
	 *
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_WIFI_NOT_CONNECT`: Wifi failed to connect to AP
	 * @retval - `ESP_ERR_WIFI_NOT_INIT`: WiFi is not initialized by esp_wifi_init
	 * @retval- `ESP_ERR_INVALID_ARG`: It doesn't normally happen, the function called inside the API was passed invalid argument, user should check
	 * if the WiFi related config is correct
	 * @retval - `ESP_ERR_NO_MEM`: out of memory
	 * @retval - `ESP_ERR_WIFI_CONN`: WiFi internal error, station or soft-AP control block wrong
	 * @retval - `ESP_FAIL`: other WiFi internal errors
	 *
	 * @note The separation of configuration and start connecting allows for additional external modifications.
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF WiFi Driver
	 * Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/wifi-driver/station-scenarios.html)
	 */
	static esp_err_t connect();

  private:
	/*
	 * Waiting until either:
	 * - the connection is established (WIFI_CONNECTED_BIT) or
	 * - connection failed because the maximum number of retries is reached (WIFI_CONNECTION_FAILED_BIT).
	 */
	static esp_err_t wait_for_connection();

	/**
	 * Event handler for WiFi and IP events.
	 *
	 * This function is registered to the `default event loop` and is called whenever an event is posted to the specified event bases related to WiFi.
	 *
	 * @param arg User-provided arguments passed during handler registration. Can be `nullptr` if not used.
	 * @param event_base Event base identifying the event source.
	 * @param event_id Event identifier specific to the event base.
	 * @param event_data Pointer to event specific data structure. The actual type depends on the `event_base` and `event_id`.
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF WiFi Driver
	 * Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/wifi-driver/station-scenarios.html)
	 *
	 * - [ESP-IDF Event Loop Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/system/esp_event.html)
	 */
	static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

	/**
	 * Triggered when the WiFi is successfully started in station mode. The task will initialize the LwIP network interface (netif) and tries to
	 * connect to the AP.
	 */
	static void handle_sta_start();

	/**
	 * Triggered when the WiFi is successfully stoped and was in station mode. The task will release the IP address, stop the DHCP client, remove
	 * TCP/UDP related connections, and clear the LwIP station.
	 */
	static void handle_sta_stop();

	/**
	 * Triggered when the station successfully connects to the AP. Starts the DHCP client and begins DHCP process of getting the IP address.
	 */
	static void handle_sta_connected(wifi_event_sta_connected_t *event);

	/**
	 * Triggered:
	 * - when WiFi is is successfully disconnected or stopped when already connected to AP,
	 * - when the WiFi diver failed to connect to the AP, or
	 * - when WiFi connection is disrupted.
	 * The task shuts down the stations LwIp netif and clear the UDP/TCP connections.
	 */
	static void handle_sta_disconnected(wifi_event_sta_disconnected_t *event);

	/**
	 * Triggered:
	 * - when the WiFi diver failed to connect to the AP, or
	 * - when WiFi connection is disrupted.
	 */
	static void handle_sta_reconnect(wifi_event_sta_disconnected_t *event);

	/**
	 * Triggered when the DHCP client successfully gets the IPv4 address from the DHCP server, or when the IPv4 address is changed. Everything is
	 * ready and the application can begin its tasks.
	 */
	static void handle_got_ip(ip_event_got_ip_t *event);
	/**
	 * Triggered when the IPv4 address becomes invalid. This does NOT happen immediately after a disconnect.
	 *
	 * For debug purposes.
	 */
	static void handle_lost_ip();

	static constexpr const char *kTag = "wifi-network";
	static EventGroupHandle_t s_wifi_event_group;
	static uint8_t s_retry_num;
};

/**
 * Converts the `wifi_err_reason_t` (IEEE 802.11) to the corrsponding string
 *
 * @param reason The reason to convert
 * @returns The string representation
 */
constexpr const char *wifi_reason_to_string(wifi_err_reason_t reason);

/**
 * Converts the `wifi_err_reason_t` (IEEE 802.11) to the corrsponding string
 *
 * @param reason The reason to convert
 * @returns The string representation
 */
constexpr const char *wifi_reason_to_string(uint8_t reason);