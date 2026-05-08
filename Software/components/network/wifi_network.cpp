#include "wifi_network.hpp"

#include <cstring>
#include <esp_check.h>
#include <esp_log.h>
#include <mdns.h>
#include <string>

// This is for clang tidy when not configured the files still get analyzed and than have missing defines
#ifdef __clang__
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifndef CONFIG_WIFI_SSID
#define CONFIG_WIFI_SSID "myssid"
#endif
#ifndef CONFIG_WIFI_PASSWORD
#define CONFIG_WIFI_PASSWORD "mypassword"
#endif
#ifndef CONFIG_WIFI_CONN_MAX_RETRY
#define CONFIG_WIFI_CONN_MAX_RETRY 6
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif

uint8_t WifiNetwork::s_retry_num = 0;

EventGroupHandle_t WifiNetwork::s_wifi_event_group = xEventGroupCreate();

esp_err_t WifiNetwork::init() {
	ESP_LOGD(kTag, "Setup wifi connection...");
	// Initialize wifi and start the task
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_init_config), kTag, "Failed to initialize wifi");

	// Register event handler
	ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr), kTag,
						"Failed to register WiFi event handler");

	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr), kTag,
						"Failed to register WiFi IP event handler");

	// Configure the wifi
	wifi_config_t wifi_config{};
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	std::strncpy(reinterpret_cast<char *>(wifi_config.sta.ssid), CONFIG_WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	std::strncpy(reinterpret_cast<char *>(wifi_config.sta.password), CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);
	wifi_config.sta.scan_method = WIFI_FAST_SCAN;

	ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), kTag, "Failed to set wifi to station mode ");

	ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), kTag, "Failed to set wifi configuration");

	ESP_LOGD(kTag, "Setup wifi connection finished");

	return ESP_OK;
}

esp_err_t WifiNetwork::connect() {
	// TODO: Write testcases for wifi/ethernet connection

	ESP_LOGD(kTag, "Start wifi connection...");
	// Start wifi
	ESP_RETURN_ON_ERROR(esp_wifi_start(), kTag, "Failed to start WiFi");

	esp_wifi_set_ps(WIFI_PS_NONE);
	ESP_LOGD("NETWORK", "WiFi Power Save deaktiviert");

	return wait_for_connection();
}

esp_err_t WifiNetwork::wait_for_connection() {

	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_CONNECTION_FAILED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	// xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened.
	if ((bits & WIFI_CONNECTED_BIT) != 0) {
		ESP_LOGD(kTag, "Connected to AP SSID: %s", CONFIG_WIFI_SSID);
	} else if ((bits & WIFI_CONNECTION_FAILED_BIT) != 0) {
		ESP_LOGE(kTag, "Failed to connect to SSID: %s", CONFIG_WIFI_SSID);
		return ESP_ERR_WIFI_NOT_CONNECT;
	} else {
		ESP_LOGE(kTag, "Unexpected event");
		return ESP_FAIL;
	}

	return ESP_OK;
}

void WifiNetwork::wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		handle_sta_start();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
		handle_sta_stop();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
		handle_sta_connected(static_cast<wifi_event_sta_connected_t *>(event_data));
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		handle_sta_disconnected(static_cast<wifi_event_sta_disconnected_t *>(event_data));
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		handle_got_ip(static_cast<ip_event_got_ip_t *>(event_data));
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
		handle_lost_ip();
	}
}

void WifiNetwork::handle_sta_start() {
	ESP_LOGD(kTag, "WiFi is successfully stated in station mode");
	esp_wifi_connect();
}

void WifiNetwork::handle_sta_stop() {
	ESP_LOGD(kTag, "WiFi is successfully stopped");
	xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
}

void WifiNetwork::handle_sta_connected(wifi_event_sta_connected_t *event) {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	ESP_LOGD(kTag, "WiFi is successfully connected to SSID: %s", reinterpret_cast<const char *>(event->ssid));
}

void WifiNetwork::handle_sta_disconnected(wifi_event_sta_disconnected_t *event) {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	ESP_LOGD(kTag, "WiFi is disconnected from SSID: %s (%s)", reinterpret_cast<char *>(event->ssid), wifi_reason_to_string(event->reason));
	xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

	handle_sta_reconnect(event);
}

void WifiNetwork::handle_sta_reconnect(wifi_event_sta_disconnected_t *event) {
	// TODO: Only try to reconnect when connection lost or not able to connect on first try
	// TODO: Allow fallback implementation where esp works as AP

	// Retry if max wifi retry configuration is not reached
	if (s_retry_num < CONFIG_WIFI_CONN_MAX_RETRY) {
		esp_wifi_connect();
		s_retry_num++;
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		ESP_LOGD(kTag, "Retry connecting to %s...", reinterpret_cast<char *>(event->ssid));
	} else {
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		ESP_LOGE(kTag, "Too many retries for trying to connect to SSID: %s", reinterpret_cast<char *>(event->ssid));
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTION_FAILED_BIT);
	}
}

void WifiNetwork::handle_got_ip(ip_event_got_ip_t *event) {
	ESP_LOGD(kTag, "WiFi successfully got IPv4 address: " IPSTR, IP2STR(&event->ip_info.ip));
	s_retry_num = 0;
	mdns_init();
	mdns_hostname_set(CONFIG_LWIP_LOCAL_HOSTNAME);
	mdns_service_add(CONFIG_LWIP_LOCAL_HOSTNAME, "_http", "_tcp", 80, nullptr, 0);
	mdns_service_add(CONFIG_LWIP_LOCAL_HOSTNAME, "_https", "_tcp", 443, nullptr, 0);
	ESP_LOGD(kTag, "WiFi successfully got mDNS address: %s.local", CONFIG_LWIP_LOCAL_HOSTNAME);
	xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
}

void WifiNetwork::handle_lost_ip() {
	ESP_LOGW(kTag, "WiFi lost IPv4 address");
	mdns_free();
	ESP_LOGD(kTag, "MDNS service stopped");
}

constexpr const char *wifi_reason_to_string(wifi_err_reason_t reason) {
	switch (reason) {
	case WIFI_REASON_UNSPECIFIED:
		return "UNSPECIFIED";
	case WIFI_REASON_AUTH_EXPIRE:
		return "AUTH_EXPIRE";
	case WIFI_REASON_AUTH_LEAVE:
		return "AUTH_LEAVE";
	case WIFI_REASON_DISASSOC_DUE_TO_INACTIVITY:
		return "DISASSOC_DUE_TO_INACTIVITY";
	case WIFI_REASON_ASSOC_TOOMANY:
		return "ASSOC_TOOMANY";
	case WIFI_REASON_CLASS2_FRAME_FROM_NONAUTH_STA:
		return "CLASS2_FRAME_FROM_NONAUTH_STA";
	case WIFI_REASON_CLASS3_FRAME_FROM_NONASSOC_STA:
		return "CLASS3_FRAME_FROM_NONASSOC_STA";
	case WIFI_REASON_ASSOC_LEAVE:
		return "ASSOC_LEAVE";
	case WIFI_REASON_ASSOC_NOT_AUTHED:
		return "ASSOC_NOT_AUTHED";
	case WIFI_REASON_DISASSOC_PWRCAP_BAD:
		return "DISASSOC_PWRCAP_BAD";
	case WIFI_REASON_DISASSOC_SUPCHAN_BAD:
		return "DISASSOC_SUPCHAN_BAD";
	case WIFI_REASON_BSS_TRANSITION_DISASSOC:
		return "BSS_TRANSITION_DISASSOC";
	case WIFI_REASON_IE_INVALID:
		return "IE_INVALID";
	case WIFI_REASON_MIC_FAILURE:
		return "MIC_FAILURE";
	case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
		return "4WAY_HANDSHAKE_TIMEOUT";
	case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:
		return "GROUP_KEY_UPDATE_TIMEOUT";
	case WIFI_REASON_IE_IN_4WAY_DIFFERS:
		return "IE_IN_4WAY_DIFFERS";
	case WIFI_REASON_GROUP_CIPHER_INVALID:
		return "GROUP_CIPHER_INVALID";
	case WIFI_REASON_PAIRWISE_CIPHER_INVALID:
		return "PAIRWISE_CIPHER_INVALID";
	case WIFI_REASON_AKMP_INVALID:
		return "AKMP_INVALID";
	case WIFI_REASON_UNSUPP_RSN_IE_VERSION:
		return "UNSUPP_RSN_IE_VERSION";
	case WIFI_REASON_INVALID_RSN_IE_CAP:
		return "INVALID_RSN_IE_CAP";
	case WIFI_REASON_802_1X_AUTH_FAILED:
		return "802_1X_AUTH_FAILED";
	case WIFI_REASON_CIPHER_SUITE_REJECTED:
		return "CIPHER_SUITE_REJECTED";
	case WIFI_REASON_TDLS_PEER_UNREACHABLE:
		return "TDLS_PEER_UNREACHABLE";
	case WIFI_REASON_TDLS_UNSPECIFIED:
		return "TDLS_UNSPECIFIED";
	case WIFI_REASON_SSP_REQUESTED_DISASSOC:
		return "SSP_REQUESTED_DISASSOC";
	case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT:
		return "NO_SSP_ROAMING_AGREEMENT";
	case WIFI_REASON_BAD_CIPHER_OR_AKM:
		return "BAD_CIPHER_OR_AKM";
	case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION:
		return "NOT_AUTHORIZED_THIS_LOCATION";
	case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS:
		return "SERVICE_CHANGE_PERCLUDES_TS";
	case WIFI_REASON_UNSPECIFIED_QOS:
		return "UNSPECIFIED_QOS";
	case WIFI_REASON_NOT_ENOUGH_BANDWIDTH:
		return "NOT_ENOUGH_BANDWIDTH";
	case WIFI_REASON_MISSING_ACKS:
		return "MISSING_ACKS";
	case WIFI_REASON_EXCEEDED_TXOP:
		return "EXCEEDED_TXOP";
	case WIFI_REASON_STA_LEAVING:
		return "STA_LEAVING";
	case WIFI_REASON_END_BA:
		return "END_BA";
	case WIFI_REASON_UNKNOWN_BA:
		return "UNKNOWN_BA";
	case WIFI_REASON_TIMEOUT:
		return "TIMEOUT";
	case WIFI_REASON_PEER_INITIATED:
		return "PEER_INITIATED";
	case WIFI_REASON_AP_INITIATED:
		return "AP_INITIATED";
	case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT:
		return "INVALID_FT_ACTION_FRAME_COUNT";
	case WIFI_REASON_INVALID_PMKID:
		return "INVALID_PMKID";
	case WIFI_REASON_INVALID_MDE:
		return "INVALID_MDE";
	case WIFI_REASON_INVALID_FTE:
		return "INVALID_FTE";
	case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED:
		return "TRANSMISSION_LINK_ESTABLISH_FAILED";
	case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED:
		return "ALTERATIVE_CHANNEL_OCCUPIED";
	case WIFI_REASON_BEACON_TIMEOUT:
		return "BEACON_TIMEOUT";
	case WIFI_REASON_NO_AP_FOUND:
		return "NO_AP_FOUND";
	case WIFI_REASON_AUTH_FAIL:
		return "AUTH_FAIL";
	case WIFI_REASON_ASSOC_FAIL:
		return "ASSOC_FAIL";
	case WIFI_REASON_HANDSHAKE_TIMEOUT:
		return "HANDSHAKE_TIMEOUT";
	case WIFI_REASON_CONNECTION_FAIL:
		return "CONNECTION_FAIL";
	case WIFI_REASON_AP_TSF_RESET:
		return "AP_TSF_RESET";
	case WIFI_REASON_ROAMING:
		return "ROAMING";
	case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:
		return "ASSOC_COMEBACK_TIME_TOO_LONG";
	case WIFI_REASON_SA_QUERY_TIMEOUT:
		return "SA_QUERY_TIMEOUT";
	case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
		return "NO_AP_FOUND_W_COMPATIBLE_SECURITY";
	case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:
		return "NO_AP_FOUND_IN_AUTHMODE_THRESHOLD";
	case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:
		return "NO_AP_FOUND_IN_RSSI_THRESHOLD";
	default:
		return "UNKNOWN";
	}
}

constexpr const char *wifi_reason_to_string(uint8_t reason) { return wifi_reason_to_string((wifi_err_reason_t)reason); }