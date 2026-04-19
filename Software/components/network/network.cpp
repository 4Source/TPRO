#include "network.hpp"

#include "ethernet_network.hpp"
#include "wifi_network.hpp"
#include <esp_log.h>

// This is for clang tidy when not configured the files still get analyzed and than have missing defines
#ifdef __clang__
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifndef CONFIG_CONNECT_ETHERNET
#define CONFIG_CONNECT_ETHERNET y
#endif
#ifndef CONFIG_CONNECT_WIFI
#define CONFIG_CONNECT_WIFI y
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif

esp_err_t Network::init() {
	esp_err_t err = ESP_ERR_INVALID_STATE;

#if CONFIG_CONNECT_ETHERNET
	ESP_LOGD(kTag, "Initialize ethernet");
	err = EthernetNetwork::init();
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to initialize ethernet");
		return err;
	}
#endif

#if CONFIG_CONNECT_WIFI
	ESP_LOGD(kTag, "Initialize wifi");
	err = WifiNetwork::init();
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to initialize wifi");
		return err;
	}
#endif

	return err;
}

esp_err_t Network::connect() {
	esp_err_t err = ESP_ERR_INVALID_STATE;

#if CONFIG_CONNECT_ETHERNET
	ESP_LOGD(kTag, "Establish connection with ethernet");
	err = EthernetNetwork::connect();
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to initialize ethernet");
		return err;
	}
#endif

#if CONFIG_CONNECT_WIFI
	ESP_LOGD(kTag, "Establish connection with wifi");
	err = WifiNetwork::connect();
	if (err != ESP_OK) {
		ESP_LOGE(kTag, "Failed to initialize wifi");
		return err;
	}
#endif

	return ESP_OK;
}
