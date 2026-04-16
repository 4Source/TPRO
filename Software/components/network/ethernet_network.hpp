#pragma once
#include <driver/spi_common.h>
#include <esp_err.h>
#include <esp_eth.h>
#include <esp_netif_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/stream_buffer.h>

#define ETH_CONNECTED_BIT BIT0
#define ETH_CONNECTION_FAILED_BIT BIT1

class EthernetNetwork {
  public:
	/**
	 * Configures the ethernet network connection, as selected in menuconfig.
	 *
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_NO_MEM`: No memory to install this service
	 * @retval - `ESP_ERR_NOT_FOUND`: No free interrupt found with the specified flags
	 * @retval - `ESP_ERR_NOT_FOUND`: if there is no available DMA channel
	 * @retval - `ESP_ERR_INVALID_ARG`: GPIO error
	 * @retval - `ESP_ERR_INVALID_ARG`: if configuration is invalid
	 * @retval - `ESP_ERR_INVALID_ARG`: install esp_eth driver failed because of some invalid argument
	 * @retval - `ESP_ERR_INVALID_ARG`: process io command failed because of some invalid argument
	 * @retval - `ESP_ERR_INVALID_ARG`: Invalid combination of event base and event ID
	 * @retval - `ESP_ERR_NOT_SUPPORTED`: requested feature is not supported
	 * @retval - `ESP_ERR_ESP_NETIF_DRIVER_ATTACH_FAILED`: if driver's pot_attach callback failed
	 * @retval - `ESP_FAIL`: install esp_eth driver failed because some other error occurred
	 * @retval - `ESP_FAIL`: process io command failed because some other error occurred
	 * @retval - `ESP_FAIL`: Could not create MAC/PHY instance
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF Ethernet Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_eth.html)
	 */
	static esp_err_t init();

	/**
	 * Start the ethernet network connection. Should be configured first with init.
	 *
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_INVALID_ARG`: start esp_eth driver failed because of some invalid argument
	 * @retval - `ESP_ERR_INVALID_STATE`: start esp_eth driver failed because driver has started already
	 * @retval - `ESP_FAIL`: start esp_eth driver failed because some other error occurred
	 *
	 * @note The separation of configuration and start connecting allows for additional external modifications.
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF Ethernet Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_eth.html)
	 */
	static esp_err_t connect();

  private:
	/*
	 * Waiting until either:
	 * - the connection is established (ETH_CONNECTED_BIT) or
	 * - connection failed because took to long to establish a connection (ETH_CONNECTION_FAILED_BIT).
	 */
	static esp_err_t wait_for_connection();

	/**
	 * Initializes the SPI bus for communication with the W5500 ethernet chip
	 *
	 * @param device_config [OUT] The device configuration
	 *
	 * @retval - `ESP_OK`: Succeed
	 * @retval - `ESP_ERR_NO_MEM`: No memory to install this service
	 * @retval - `ESP_ERR_NOT_FOUND`: No free interrupt found with the specified flags
	 * @retval - `ESP_ERR_INVALID_ARG`: GPIO error
	 * @retval - `ESP_ERR_INVALID_ARG`: if configuration is invalid
	 * @retval - `ESP_ERR_NOT_FOUND`: if there is no available DMA channel
	 * @retval - `ESP_ERR_NO_MEM`: if out of memory
	 *
	 * 	 * For more details, see:
	 *
	 * - [ESP-IDF Ethernet Driver
	 * Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_eth.html#spi-ethernet-module)
	 */
	static esp_err_t init_spi(spi_device_interface_config_t *device_config);

	/**
	 * Event handler for Ethernet and IP events.
	 *
	 * This function is registered to the `default event loop` and is called whenever an event is posted to the specified event bases related to
	 * Ethernet.
	 *
	 * @param arg User-provided arguments passed during handler registration. Can be `nullptr` if not used.
	 * @param event_base Event base identifying the event source.
	 * @param event_id Event identifier specific to the event base.
	 * @param event_data Pointer to event specific data structure. The actual type depends on the `event_base` and `event_id`.
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF Ethernet Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/network/esp_eth.html)
	 *
	 * - [ESP-IDF Event Loop Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/system/esp_event.html)
	 */
	static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

	/**
	 * Triggered when the Ethernet driver is initialized and started. The interface is up at driver level but no link or IP is guaranteed yet.
	 */
	static void handle_eth_start();
	/**
	 * Triggered when the Ethernet driver is stopped. the interface is fully down and any link or IP information is no longer valid.
	 */
	static void handle_eth_stop();
	/**
	 * Triggered when the PHY reports a valid physical link (e.g., cable connected and auto-negotiation completed); Layer 1 is up but no IP address is
	 * assigned yet.
	 */
	static void handle_eth_connected(esp_eth_handle_t event);
	/**
	 * Triggered when the PHY loses the physical link (e.g., cable unplugged or link failure); communication is no longer possible and the IP will
	 * typically be lost shortly after.
	 */
	static void handle_eth_disconnected(esp_eth_handle_t event);
	/**
	 * Triggered when the TCP/IP stack assigns an IP address to the Ethernet interface, typically after a successful DHCP lease or immediately if a
	 * static IP is configured; the interface is fully operational for network communication.
	 */
	static void handle_got_ip(ip_event_got_ip_t *event);
	/**
	 * Triggered when the Ethernet interface loses its IP address, typically due to link loss, DHCP lease expiration, or interface shutdown; the
	 * interface can no longer be used for IP communication.
	 */
	static void handle_lost_ip();

	static constexpr const char *kTag = "ethernet-network";
	static EventGroupHandle_t s_eth_event_group;
	static esp_eth_handle_t s_eth_handle;
};