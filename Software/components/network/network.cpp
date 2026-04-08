#include "network.hpp"
#include <esp_check.h>
#include <esp_eth.h>
#include <esp_log.h>
#include <esp_wifi.h>

static const char *TAG = "network";

/*
 * FreeRTOS event group to signal when we are connected to WiFi
 */
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_CONNECTION_FAILED_BIT BIT1

static int s_retry_num = 0;

/**
 * Event handler for WiFi and IP events.
 *
 * This function is registered to the `default event loop` and is called whenever an event is posted to the specified event bases.
 *
 * @param arg User-provided arguments passed during handler registration. Can be `nullptr` if not used.
 * @param event_base Event base identifying the event source.
 * @param event_id Event identifier specific to the event base.
 * @param event_data Pointer to event specific data structure. The actual type depends on the `event_base` and `event_id`.
 *
 * For more details, see:
 *
 * - [ESP-IDF WiFi Driver Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/wifi-driver/station-scenarios.html)
 *
 * - [ESP-IDF Event Loop Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/system/esp_event.html)
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	/**
	 * Triggered when the WiFi is successfully started in station mode. The task will initialize the LwIP network interface (netif) and tries to
	 * connect to the AP.
	 */
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		ESP_LOGD(TAG, "WiFi is successfully stated in station mode");

		// Try to connect to the configured AP
		esp_wifi_connect();
	}
	/**
	 * Triggered when the WiFi is successfully stoped and was in station mode. The task will release the IP address, stop the DHCP client, remove
	 * TCP/UDP related connections, and clear the LwIP station.
	 */
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP) {
		ESP_LOGD(TAG, "WiFi is successfully stoped");
		xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
	/**
	 * Triggered when the station successfully connects to the AP. Starts the DHCP client and begins DHCP process of getting the IP address.
	 */
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
		wifi_event_sta_connected_t *event = (wifi_event_sta_connected_t *)event_data;
		ESP_LOGI(TAG, "WiFi is successfully connected to SSID: %s", (char *)event->ssid);
	}
	/**
	 * Triggered:
	 * - when WiFi is is successfully disconnected or stopped when already connected to AP,
	 * - when the WiFi diver failed to connect to the AP, or
	 * - when WiFi connection is disrupted.
	 * The task shuts down the stations LwIp netif and clear the UDP/TCP connections.
	 */
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
		ESP_LOGI(TAG, "WiFi is disconnected from SSID: %s (%s)", (char *)event->ssid, wifi_reason_to_string(event->reason));
		xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

		// TODO: Only try to reconnect when connection lost or not able to connect on first try
		// TODO: Allow fallback implementation where esp works as AP

		// Retry if max wifi retry configuration is not reached
		if (s_retry_num < CONFIG_WIFI_CONN_MAX_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "Retry connecting to %s...", (char *)event->ssid);
		}
		// Cancel wifi connection because of too many tries to connect to AP
		else {
			ESP_LOGE(TAG, "Too many retries for trying to connect to SSID: %s", (char *)event->ssid);
			xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTION_FAILED_BIT);
		}
	}
	/**
	 * Triggered when the DHCP client successfully gets the IPv4 address from the DHCP server, or when the IPv4 address is changed. Everything is
	 * ready and the application can begin its tasks.
	 */
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG, "Successfully got IPv4 address: " IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
	/**
	 * Triggered when the IPv4 address becomes invalid. This does NOT happen immediately after a disconnect.
	 *
	 * For debug purposes.
	 */
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP) {
		ESP_LOGW(TAG, "Lost IPv4 address");
	}
}

esp_err_t init_network(void) {
#if CONFIG_CONNECT_ETHERNET
	ESP_LOGI(TAG, "Setup ethernet connection...");
	ESP_LOGW(TAG, "Ethernet is not implemented yet.");
#endif
#if CONFIG_CONNECT_WIFI
	s_wifi_event_group = xEventGroupCreate();

	ESP_LOGI(TAG, "Setup wifi connection...");

	// Initialize wifi and start the task
	wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_RETURN_ON_ERROR(esp_wifi_init(&wifi_cfg), TAG, "Failed to initialize wifi");

	// Register event handler
	ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr), TAG,
						"Failed to register WiFi event handler");
	ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, nullptr), TAG,
						"Failed to register WiFi IP event handler");

	// Configure the wifi
	wifi_config_t wifi_config{};
	strncpy((char *)wifi_config.sta.ssid, CONFIG_WIFI_SSID, sizeof(wifi_config.sta.ssid));
	strncpy((char *)wifi_config.sta.password, CONFIG_WIFI_PASSWORD, sizeof(wifi_config.sta.password));
	wifi_config.sta.scan_method = WIFI_FAST_SCAN;

	ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed to set wifi to station mode ");
	ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed to set wifi configuration");

	ESP_LOGI(TAG, "Setup wifi connection finished");
#endif
	return ESP_OK;
}

esp_err_t connect_network(void) {
#if CONFIG_CONNECT_ETHERNET
	ESP_LOGI(TAG, "Start ethernet connection...");
	ESP_LOGW(TAG, "Ethernet is not implemented yet.");
#endif
#if CONFIG_CONNECT_WIFI
	s_wifi_event_group = xEventGroupCreate();

	ESP_LOGI(TAG, "Start wifi connection...");

	// Start wifi
	ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed to start WiFi");

	/*
	 * Waiting until either:
	 * - the connection is established (WIFI_CONNECTED_BIT) or
	 * - connection failed because the maximum number of retries is reached (WIFI_CONNECTION_FAILED_BIT).
	 */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_CONNECTION_FAILED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	// xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually happened.
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "Connected to AP SSID: %s", CONFIG_WIFI_SSID);
	} else if (bits & WIFI_CONNECTION_FAILED_BIT) {
		ESP_RETURN_ON_ERROR(ESP_ERR_WIFI_NOT_CONNECT, TAG, "Failed to connect to SSID: %s", CONFIG_WIFI_SSID);
	} else {
		ESP_RETURN_ON_ERROR(ESP_FAIL, TAG, "Unexpected event");
	}
#endif
	return ESP_OK;
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
