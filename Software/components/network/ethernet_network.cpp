#include "ethernet_network.hpp"

#include <array>
#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_eth_mac_w5500.h>
#include <esp_eth_phy_w5500.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_netif_ip_addr.h>

// This is for clang tidy when not configured the files still get analyzed and than have missing defines
#ifdef __clang__
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifndef ETHERNET_SPI_MISO_GPIO
#define ETHERNET_SPI_MISO_GPIO 12
#endif
#ifndef ETHERNET_SPI_MOSI_GPIO
#define ETHERNET_SPI_MOSI_GPIO 11
#endif
#ifndef ETHERNET_SPI_SCLK_GPIO
#define ETHERNET_SPI_SCLK_GPIO 13
#endif
#ifndef ETHERNET_SPI_CS_GPIO
#define ETHERNET_SPI_CS_GPIO 14
#endif
#ifndef ETHERNET_SPI_HOST
#define ETHERNET_SPI_HOST 2
#endif
#ifndef ETHERNET_RST_GPIO
#define ETHERNET_RST_GPIO 9
#endif
#ifndef ETHERNET_INT_GPIO
#define ETHERNET_INT_GPIO 10
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif

#if (CONFIG_ETHERNET_SPI_HOST == 1)
#define ETH_SPI_HOST SPI1_HOST
#elif (CONFIG_ETHERNET_SPI_HOST == 2)
#define ETH_SPI_HOST SPI2_HOST
#elif (CONFIG_ETHERNET_SPI_HOST == 3)
#define ETH_SPI_HOST SPI3_HOST
#endif

esp_eth_handle_t EthernetNetwork::s_eth_handle = nullptr;

EventGroupHandle_t EthernetNetwork::s_eth_event_group = xEventGroupCreate();

esp_err_t EthernetNetwork::init() {
	ESP_LOGI(kTag, "Setup ethernet connection...");

	esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
	esp_netif_t *eth_netif = esp_netif_new(&netif_config);

	spi_device_interface_config_t device_config = {};
	ESP_RETURN_ON_ERROR(init_spi(&device_config), kTag, "Failed to initialize SPI bus for ethernet");

	// Init vendor specific MAC config to default
	eth_w5500_config_t ethernet_config = ETH_W5500_DEFAULT_CONFIG(ETH_SPI_HOST, &device_config);
	ethernet_config.int_gpio_num = CONFIG_ETHERNET_INT_GPIO;

	// Init common MAC and PHY configs to default
	eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
	eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
	phy_config.reset_gpio_num = CONFIG_ETHERNET_RST_GPIO;
	phy_config.phy_addr = 1;

	// Create new MAC instance
	esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&ethernet_config, &mac_config);
	if (mac == nullptr) {
		ESP_LOGE(kTag, "Create MAC instance failed");
		return ESP_FAIL;
	}

	// Create new PHY instance
	esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
	if (phy == nullptr) {
		ESP_LOGE(kTag, "Create PHY instance failed");
		mac->del(mac);
		return ESP_FAIL;
	}

	// Init Ethernet driver to default and install it
	esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
	esp_err_t err = esp_eth_driver_install(&config, &s_eth_handle);
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Ethernet driver install failed");
		mac->del(mac);
		phy->del(phy);
		return err;
	}

	// Apply local unique MAC this is necessary so there will be no conflicts with WiFi
	std::array<uint8_t, 6> local_mac;
	std::array<uint8_t, 6> base_mac;
	ESP_RETURN_ON_ERROR(esp_read_mac(base_mac.data(), ESP_MAC_ETH), kTag, "Failed to read base MAC");
	ESP_RETURN_ON_ERROR(esp_derive_local_mac(local_mac.data(), base_mac.data()), kTag, "Failed to create local MAC");
	ESP_RETURN_ON_ERROR(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, local_mac.data()), kTag, "Failed to set local MAC");

	ESP_RETURN_ON_ERROR(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(s_eth_handle)), kTag, "Failed to attach netif to ethernet handle");

	// Register event handler
	ESP_RETURN_ON_ERROR(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, nullptr), kTag,
						"Failed to register Ethernet event handler");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, nullptr), kTag,
						"Failed to register Ethernet IP event handler");

	return ESP_OK;
}

esp_err_t EthernetNetwork::connect() {
	ESP_LOGI(kTag, "Start ethernet connection...");

	ESP_RETURN_ON_ERROR(esp_eth_start(s_eth_handle), kTag, "Failed to start ethernet");

	return wait_for_connection();
}

esp_err_t EthernetNetwork::wait_for_connection() {
	EventBits_t bits = xEventGroupWaitBits(s_eth_event_group, ETH_CONNECTED_BIT | ETH_CONNECTION_FAILED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	// TODO: implement a timeout after connection when the connection should fail
	// xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened.
	if ((bits & ETH_CONNECTED_BIT) != 0) {
		ESP_LOGI(kTag, "Connected to ethernet");
	} else if ((bits & ETH_CONNECTION_FAILED_BIT) != 0) {
		ESP_LOGE(kTag, "Failed to connect ethernet");
		return ESP_FAIL;
	} else {
		ESP_LOGE(kTag, "Unexpected event");
		return ESP_FAIL;
	}

	return ESP_OK;
}

esp_err_t EthernetNetwork::init_spi(spi_device_interface_config_t *device_config) {
	// Init SPI bus
	esp_err_t err = gpio_install_isr_service(0);
	if (err == ESP_ERR_INVALID_STATE) {
		// ISR handler has been already installed so no issues
		ESP_LOGD(kTag, "GPIO ISR handler has been already installed");
		err = ESP_OK;
	}
	ESP_RETURN_ON_ERROR(err, kTag, "GPIO ISR handler install failed");

	spi_bus_config_t bus_config{};
	// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access) Necessary for the idf internal union
	bus_config.miso_io_num = CONFIG_ETHERNET_SPI_MISO_GPIO;
	bus_config.mosi_io_num = CONFIG_ETHERNET_SPI_MOSI_GPIO;
	bus_config.sclk_io_num = CONFIG_ETHERNET_SPI_SCLK_GPIO;
	bus_config.quadwp_io_num = -1;
	bus_config.quadhd_io_num = -1;
	// NOLINTEND(cppcoreguidelines-pro-type-union-access)

	err = spi_bus_initialize(ETH_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO);
	if (err == ESP_ERR_INVALID_STATE) {
		// SPI host has been already initialized so no issues
		ESP_LOGD(kTag, "SPI host #%d has been already initialized", CONFIG_ETHERNET_SPI_HOST);
		err = ESP_OK;
	}
	ESP_RETURN_ON_ERROR(err, kTag, "SPI host #%d init failed", CONFIG_ETHERNET_SPI_HOST);

	// SPI device config
	device_config->command_bits = 16;
	device_config->address_bits = 8;
	device_config->mode = 0;
	device_config->clock_speed_hz = 20 * 1000 * 1000; // 20 MHz
	device_config->spics_io_num = CONFIG_ETHERNET_SPI_CS_GPIO;
	device_config->queue_size = 20;

	return ESP_OK;
}

void EthernetNetwork::eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_START) {
		handle_eth_start();
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_STOP) {
		handle_eth_stop();
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_CONNECTED) {
		handle_eth_connected(*static_cast<esp_eth_handle_t *>(event_data));
	} else if (event_base == ETH_EVENT && event_id == ETHERNET_EVENT_DISCONNECTED) {
		handle_eth_disconnected(*static_cast<esp_eth_handle_t *>(event_data));
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
		handle_got_ip(static_cast<ip_event_got_ip_t *>(event_data));
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_LOST_IP) {
		handle_lost_ip();
	}
}

void EthernetNetwork::handle_eth_start() { ESP_LOGD(kTag, "Ethernet is successfully stated"); }

void EthernetNetwork::handle_eth_stop() {
	ESP_LOGD(kTag, "Ethernet is successfully stopped");
	xEventGroupClearBits(s_eth_event_group, ETH_CONNECTED_BIT);
}

void EthernetNetwork::handle_eth_connected(esp_eth_handle_t event) {
	std::array<uint8_t, 6> mac_addr{0};
	esp_eth_ioctl(event, ETH_CMD_G_MAC_ADDR, mac_addr.data());
	ESP_LOGI(kTag, "Ethernet is successfully connected as MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac_addr.at(0), mac_addr.at(1), mac_addr.at(2),
			 mac_addr.at(3), mac_addr.at(4), mac_addr.at(5));
}

void EthernetNetwork::handle_eth_disconnected(esp_eth_handle_t event) {
	std::array<uint8_t, 6> mac_addr{0};
	esp_eth_ioctl(event, ETH_CMD_G_MAC_ADDR, mac_addr.data());
	ESP_LOGI(kTag, "Ethernet is disconnected as MAC: %02x:%02x:%02x:%02x:%02x:%02x", mac_addr.at(0), mac_addr.at(1), mac_addr.at(2), mac_addr.at(3),
			 mac_addr.at(4), mac_addr.at(5));

	xEventGroupClearBits(s_eth_event_group, ETH_CONNECTED_BIT);
}

void EthernetNetwork::handle_got_ip(ip_event_got_ip_t *event) {
	ESP_LOGI(kTag, "Ethernet successfully got IPv4 address: " IPSTR, IP2STR(&event->ip_info.ip));
	xEventGroupSetBits(s_eth_event_group, ETH_CONNECTED_BIT);
}

void EthernetNetwork::handle_lost_ip() { ESP_LOGW(kTag, "Ethernet lost IPv4 address"); }
