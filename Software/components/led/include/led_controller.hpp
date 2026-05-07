#include "led_frame.hpp"
#include <array>
#include <driver/rmt_tx.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LED_TOTAL_ROWS CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS

class LedController {
  public:
	/**
	 * @param frame Reference to the central LedFrame
	 */
	LedController(LedFrame &frame);

	/**
	 * Starts the background task which is responsible for sending the LED values and initaliases the LED strips
	 * @retval `ESP_OK`: On Success
	 *
	 * For more details, see:
	 *
	 * - [ESP-IDF RMT Documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-reference/peripherals/rmt.html)
	 *
	 * - [ESP-IDF RMT Reference Manual](https://documentation.espressif.com/esp32-s3_technical_reference_manual_en.pdf#page=1415)
	 */
	esp_err_t run();

	/**
	 * Stops the background task which is responsible for sending the LED values and uninitaliases the LED strips
	 * @retval `ESP_OK`: On Success
	 */
	esp_err_t stop();

	static void print_map_table_csv_flat();
	static void print_map_table_csv_table();
	void print_led_buffer_csv_table();

  private:
	/**
	 * The background task which handles sending the LED values out.
	 * @param arg Pointer to the instance of this class
	 */
	static void led_send_task(void *arg);

	/**
	 * Sends the LEDs values out
	 */
	esp_err_t update();

	static constexpr const char *kTag = "led-controller";

	class LockGuard {
	  public:
		explicit LockGuard(SemaphoreHandle_t m) : mutex(m) { xSemaphoreTake(mutex, portMAX_DELAY); }
		~LockGuard() { xSemaphoreGive(mutex); }

	  private:
		SemaphoreHandle_t mutex;
	};

	LedFrame &r_frame; // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members)
	SemaphoreHandle_t xSemaphore;
	rmt_channel_handle_t led_ch1 = nullptr;
	rmt_channel_handle_t led_ch2 = nullptr;
	rmt_channel_handle_t led_ch3 = nullptr;
	std::array<uint8_t, 3 * CONFIG_LED_CH1_ROWS * CONFIG_LED_COLUMNS> led_ch1_buffer;
	std::array<uint8_t, 3 * CONFIG_LED_CH2_ROWS * CONFIG_LED_COLUMNS> led_ch2_buffer;
	std::array<uint8_t, 3 * CONFIG_LED_CH3_ROWS * CONFIG_LED_COLUMNS> led_ch3_buffer;
	rmt_encoder_handle_t led_encoder_ch1 = nullptr;
	rmt_encoder_handle_t led_encoder_ch2 = nullptr;
	rmt_encoder_handle_t led_encoder_ch3 = nullptr;
};
