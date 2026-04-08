/* LED Strip Example
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#define STRIP_GPIO_PIN GPIO_NUM_15
#define STRIP_LED_COUNT 1
#define BLINK_DELAY_MS 500

static const char *TAG = "LED_BASE";
static led_strip_handle_t led_strip;

static void configure_led(void) {
  // Setup strip with RMT
  // Info: Alles explizit angeben wegen -Werror=missing-field-initializers
  led_strip_config_t strip_config = {.strip_gpio_num = 18,
									 .max_leds = 60,
									 .led_model = LED_MODEL_WS2812,
									 .color_component_format =
										 LED_STRIP_COLOR_COMPONENT_FMT_GRB,
									 .flags = {
										 .invert_out = false,
									 }};

  led_strip_rmt_config_t rmt_config = {.clk_src = RMT_CLK_SRC_DEFAULT,
									   .resolution_hz = 10 * 1000 * 1000,
									   .mem_block_symbols = 64,
									   .flags = {
										   .with_dma = false,
									   }};
  ESP_ERROR_CHECK(
	  led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
}
extern "C" void app_main(void) {

  bool state = false;
  configure_led();
  while (1) {
	if (state) {
	  // LED 0 auf weiß
	  led_strip_set_pixel(led_strip, 0, 255, 255, 255);
	  led_strip_refresh(led_strip);
	} else {
	  led_strip_clear(led_strip);
	}

	ESP_LOGI(TAG, "LED %s", state ? "ON" : "OFF");
	state = !state;

	vTaskDelay(pdMS_TO_TICKS(BLINK_DELAY_MS));
  }
}
*/
#include "network.hpp"
#include "webserver.hpp"
#include <esp_wifi.h>
#include <nvs_flash.h>

extern "C" void app_main(void) {
	esp_err_t ret;

	printf("LED Wall startup\n");
	/**
	 * Needs to be initialized once per app. Will allow persistent storage in the flash.
	 *
	 * Needed by:
	 * - WiFi to store the configuration into flash
	 */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	/*
	 * Initialize TCP/IP stack
	 *
	 * Needed by:
	 * - WiFi
	 */
	ESP_ERROR_CHECK(esp_netif_init());

	/*
	 * Create the default event loop that runs in the background. There can only be one default event loop
	 *
	 * Needed by:
	 * - WiFi
	 */
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	/*
	 * This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
	 */
	// TODO: Proper error handling, currently the application will not launch
	ESP_ERROR_CHECK(init_network());

	/*
	 * This helper function configures the webserver.
	 */
	// TODO: Proper error handling, currently the application will not launch
	ESP_ERROR_CHECK(init_webserver());

	/*
	 * This helper function starts Wi-Fi or Ethernet, as configured above.
	 */
	// TODO: Proper error handling, currently the application will not launch
	ESP_ERROR_CHECK(connect_network());

	while (true) {
		// Delay to simulate load
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
