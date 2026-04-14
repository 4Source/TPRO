#pragma once
#include <esp_err.h>

class Network {
  public:
	/**
	 * Configures the network connections Wi-Fi and/or Ethernet, as selected in menuconfig.
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
	 *
	 * - [ESP-IDF Ethernet Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/network/esp_eth.html)
	 */
	static esp_err_t init(void);

	/**
	 * Start the network connections Wi-Fi and/or Ethernet. Should be configured first with init.
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
	 *
	 * - [ESP-IDF Ethernet Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/network/esp_eth.html)
	 */
	static esp_err_t connect(void);

  private:
	static constexpr const char *kTag = "network";
};
