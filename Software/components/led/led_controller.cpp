#include "led_controller.hpp"
#include "led_strip_encoder.hpp"
#include "task_handles.hpp"
#include <cinttypes>
#include <cstdio>
#include <esp_check.h>
#include <iostream>
#include <print>

// This is for clang tidy when not configured the files still get analyzed and than have missing defines
#ifdef __clang__
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#ifndef CONFIG_LED_COLUMNS
#define CONFIG_LED_COLUMNS 87
#endif
#ifndef CONFIG_LED_CH1_GPIO
#define CONFIG_LED_CH1_GPIO 35
#endif
#ifndef CONFIG_LED_CH1_ROWS
#define CONFIG_LED_CH1_ROWS 12
#endif
#ifndef CONFIG_LED_CH2_GPIO
#define CONFIG_LED_CH2_GPIO 36
#endif
#ifndef CONFIG_LED_CH2_ROWS
#define CONFIG_LED_CH2_ROWS 11
#endif
#ifndef CONFIG_LED_CH3_GPIO
#define CONFIG_LED_CH3_GPIO 37
#endif
#ifndef CONFIG_LED_CH3_ROWS
#define CONFIG_LED_CH3_ROWS 12
#endif
#ifndef CONFIG_LED_STRIP_RESOLUTION_HZ
#define CONFIG_LED_STRIP_RESOLUTION_HZ 10000000
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
#endif

struct MapEntry {
	uint8_t channel;
	uint32_t channel_index;
};

constexpr uint32_t compute_row_index(uint32_t frame_col, bool start_left) {
	if (start_left) {
		return frame_col;
	}
	return CONFIG_LED_COLUMNS - 1 - frame_col;
}

/**
 * Offset for mapping the row index to the channel row
 */
constexpr std::array<uint32_t, LED_TOTAL_ROWS> kRowMap = {
	// Channel 1
	5, 11, 4, 10, 3, 9, 2, 8, 1, 7, 0, 6,
	// Channel 2
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	// Channel 2
	6, 0, 7, 1, 8, 9, 2, 3, 10, 11, 4, 5};

/**
 * Function to compute the mapping table durring compiletime
 */
constexpr std::array<std::array<MapEntry, CONFIG_LED_COLUMNS>, LED_TOTAL_ROWS> compute_map_table() {
	static_assert(LED_TOTAL_ROWS > 0, "LED rows must be greater than 0");
	static_assert(CONFIG_LED_COLUMNS > 0, "LED columns must be greater than 0");

	std::array<std::array<MapEntry, CONFIG_LED_COLUMNS>, LED_TOTAL_ROWS> map;
	for (uint32_t frame_row = 0; frame_row < LED_TOTAL_ROWS; ++frame_row) {
		for (uint32_t frame_col = 0; frame_col < CONFIG_LED_COLUMNS; ++frame_col) {
			uint8_t channel = 0;
			uint32_t channel_index = 0;
			bool start_left = true;

			// index: 0	  1	  2	  3	  4   5    6   7   8   9  10  11
			// CH1: 10L, 8R, 6L, 4R, 2L, 0R, 11L, 9R, 7L, 5R, 3L, 1R
			if (frame_row < CONFIG_LED_CH1_ROWS) {
				if (frame_row == 8 || frame_row == 4 || frame_row == 0 || frame_row == 9 || frame_row == 5 || frame_row == 1) {
					start_left = false;
				}
				channel = 1;
				channel_index = kRowMap.at(frame_row) * CONFIG_LED_COLUMNS + compute_row_index(frame_col, start_left);
			}
			// index: 0	   1    2    3    4    5    6    7    8    9   10
			// CH2: 12L, 13R, 14L, 15R, 16L, 17R, 18L, 19R, 20L, 21R, 22L
			else if (frame_row < CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS) {
				if (frame_row == 13 || frame_row == 15 || frame_row == 17 || frame_row == 19 || frame_row == 21) {
					start_left = false;
				}
				channel = 2;
				channel_index = kRowMap.at(frame_row) * CONFIG_LED_COLUMNS + compute_row_index(frame_col, start_left);
			}
			// index: 0	   1    2    3    4    5    6    7    8    9   10   11
			// CH3: 24L, 26R, 29L, 30R, 33L, 34R, 23L, 25R, 27L, 28R, 31L, 32R
			else if (frame_row < LED_TOTAL_ROWS) {
				if (frame_row == 26 || frame_row == 30 || frame_row == 34 || frame_row == 25 || frame_row == 28 || frame_row == 32) {
					start_left = false;
				}
				channel = 3;
				channel_index = kRowMap.at(frame_row) * CONFIG_LED_COLUMNS + compute_row_index(frame_col, start_left);
			} else {
				__builtin_unreachable();
			}
			// NOLINTNEXTLINE[cppcoreguidelines-pro-bounds-constant-array-index]
			map[frame_row][frame_col] = {.channel = channel, .channel_index = channel_index};
		}
	}

	return map;
}

/**
 * Compile time computed mapping table for mapping the positions of the led rows and columns to the channels and the order of the physical LEDs
 */
constexpr std::array<std::array<MapEntry, CONFIG_LED_COLUMNS>, LED_TOTAL_ROWS> kMapTable = compute_map_table();

void LedController::print_map_table_csv_flat() {
	ESP_LOGW(kTag, "start of csv");
	std::println("row,column,channel,channel_index");

	for (size_t row = 0; row < LED_TOTAL_ROWS; ++row) {
		for (size_t column = 0; column < CONFIG_LED_COLUMNS; ++column) {
			const auto &entry = kMapTable.at(row).at(column);
			// NOLINTNEXTLINE
			printf("%zu,%zu,%u,%" PRIu32 "\n", row, column, entry.channel, entry.channel_index);
		}
	}
	ESP_LOGW(kTag, "end of csv");
}

void LedController::print_map_table_csv_table() {
	ESP_LOGW(kTag, "start of csv");

	std::print("channel,row/column");
	for (size_t column = 0; column < CONFIG_LED_COLUMNS; ++column) {
		std::print(",{}", column);
	}
	std::println("");

	for (size_t row = 0; row < LED_TOTAL_ROWS; ++row) {
		for (size_t column = 0; column < CONFIG_LED_COLUMNS; ++column) {
			const auto &entry = kMapTable.at(row).at(column);
			if (column == 0) {
				std::print("{},{}", entry.channel, row);
			}
			printf(",%" PRIu32, entry.channel_index); // NOLINT(cppcoreguidelines-pro-type-vararg,modernize-use-std-print)
		}
		std::println("");
	}
	ESP_LOGW(kTag, "end of csv");
}

void LedController::print_led_buffer_csv_table() {
	LockGuard lock(xSemaphore);
	ESP_LOGW(kTag, "start of csv");

	std::print("row/column");
	for (size_t column = 0; column < CONFIG_LED_COLUMNS; ++column) {
		std::print(",{}", column);
	}
	std::println("");

	for (size_t row = 0; row < LED_TOTAL_ROWS; ++row) {
		for (size_t column = 0; column < CONFIG_LED_COLUMNS; ++column) {
			MapEntry entry = kMapTable.at(row).at(column);

			uint8_t green = 0;
			uint8_t red = 0;
			uint8_t blue = 0;

			switch (entry.channel) {
			case 1:
				green = led_ch1_buffer.at((entry.channel_index * 3) + 0);
				red = led_ch1_buffer.at((entry.channel_index * 3) + 1);
				blue = led_ch1_buffer.at((entry.channel_index * 3) + 2);
				break;
			case 2:
				green = led_ch2_buffer.at((entry.channel_index * 3) + 0);
				red = led_ch2_buffer.at((entry.channel_index * 3) + 1);
				blue = led_ch2_buffer.at((entry.channel_index * 3) + 2);
				break;
			case 3:
				green = led_ch3_buffer.at((entry.channel_index * 3) + 0);
				red = led_ch3_buffer.at((entry.channel_index * 3) + 1);
				blue = led_ch3_buffer.at((entry.channel_index * 3) + 2);
				break;

			default:
				ESP_LOGE(kTag, "Unknown channel: %d", entry.channel);
				break;
			}
			if (column == 0) {
				std::print("{}", row);
			}
			std::print(",#{:02X}{:02X}{:02X}", red, green, blue);
		}
		std::println("");
	}
	ESP_LOGW(kTag, "end of csv");
}

LedController::LedController(LedFrame &frame) : r_frame{frame}, xSemaphore{xSemaphoreCreateMutex()} {}

esp_err_t LedController::run() {
	if (led_ch1 != nullptr) {
		ESP_LOGW(kTag, "LED channel 1 already in use");
		return ESP_FAIL;
	}

	if (led_ch2 != nullptr) {
		ESP_LOGW(kTag, "LED channel 2 already in use");
		return ESP_FAIL;
	}

	if (led_ch3 != nullptr) {
		ESP_LOGW(kTag, "LED channel 3 already in use");
		return ESP_FAIL;
	}

	// Initialize the common configuration
	rmt_tx_channel_config_t channel_config = {};
	channel_config.clk_src = RMT_CLK_SRC_DEFAULT;
	channel_config.mem_block_symbols = CONFIG_SOC_RMT_MEM_WORDS_PER_CHANNEL;
	channel_config.resolution_hz = CONFIG_LED_STRIP_RESOLUTION_HZ; // 10MHz
	channel_config.trans_queue_depth = 4;
	channel_config.flags.invert_out = false;
	channel_config.flags.with_dma = false;

	channel_config.gpio_num = (gpio_num_t)CONFIG_LED_CH1_GPIO;
	ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_config, &led_ch1), kTag, "Failed to initialize RMT for LED channel 1");

	// TEST: Is it still present if only channel 2 is written?
	channel_config.gpio_num = (gpio_num_t)CONFIG_LED_CH2_GPIO;
	ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_config, &led_ch2), kTag, "Failed to initialize RMT for LED channel 2");

	channel_config.gpio_num = (gpio_num_t)CONFIG_LED_CH3_GPIO;
	ESP_RETURN_ON_ERROR(rmt_new_tx_channel(&channel_config, &led_ch3), kTag, "Failed to initialize RMT for LED channel 3");

	rmt_simple_encoder_config_t encoder_config = {};
	encoder_config.callback = led_strip_encoder;
	ESP_RETURN_ON_ERROR(rmt_new_simple_encoder(&encoder_config, &led_encoder_ch1), kTag, "Failed to set encoder for LED channel 1");
	ESP_RETURN_ON_ERROR(rmt_new_simple_encoder(&encoder_config, &led_encoder_ch2), kTag, "Failed to set encoder for LED channel 2");
	ESP_RETURN_ON_ERROR(rmt_new_simple_encoder(&encoder_config, &led_encoder_ch3), kTag, "Failed to set encoder for LED channel 3");

	BaseType_t task_ret =
		xTaskCreatePinnedToCore(led_send_task, "led_send_task", TaskHandle::kStackSizeLedController, this, TaskHandle::kPrioLedController,
								&TaskHandle::x_led_controller_task_handle, TaskHandle::kStackCoreLedController);

	if (task_ret != pdPASS) {
		ESP_LOGE(kTag, "Failed to create LedController task");
		return ESP_FAIL;
	}

	ESP_LOGD(kTag, "LedController task successfully created");

	return ESP_OK;
}

esp_err_t LedController::stop() {
	ESP_LOGD(kTag, "Stopping LedController...");

	// Deinitialize LED channels
	if (led_ch1 != nullptr) {
		ESP_RETURN_ON_ERROR(rmt_del_channel(led_ch1), kTag, "Failed to deinitialize RMT for LED channel 1");
		led_ch1 = nullptr;
	}

	if (led_ch2 != nullptr) {
		ESP_RETURN_ON_ERROR(rmt_del_channel(led_ch2), kTag, "Failed to deinitialize RMT for LED channel 2");
		led_ch2 = nullptr;
	}

	if (led_ch3 != nullptr) {
		ESP_RETURN_ON_ERROR(rmt_del_channel(led_ch3), kTag, "Failed to deinitialize RMT for LED channel 3");
		led_ch3 = nullptr;
	}

	if (TaskHandle::x_led_controller_task_handle != nullptr) {
		vTaskDelete(TaskHandle::x_led_controller_task_handle);
		TaskHandle::x_led_controller_task_handle = nullptr;
	}

	return ESP_OK;
}

void LedController::led_send_task(void *arg) {
	auto *controller = static_cast<LedController *>(arg);

	while (true) {
		// Wait until got notified by light effect manager for next frame
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Process current frame
		controller->update();
	}
}

esp_err_t LedController::update() {
	LockGuard lock(xSemaphore);
	// Gether the Lock and get the Data
	{
		LedFrame::ScopedReadLock read_lock(r_frame);

		for (uint32_t frame_row = 0; frame_row < r_frame.led_data.size(); ++frame_row) {
			auto &led_row = r_frame.led_data.at(frame_row);
			for (uint32_t frame_col = 0; frame_col < led_row.size(); ++frame_col) {
				auto &led_color = led_row.at(frame_col);
				MapEntry entry = kMapTable.at(frame_row).at(frame_col);
				uint8_t channel = entry.channel;
				uint32_t channel_index = entry.channel_index;

				switch (channel) {
				case 1:
					led_ch1_buffer.at((channel_index * 3) + 0) = led_color.green;
					led_ch1_buffer.at((channel_index * 3) + 1) = led_color.red;
					led_ch1_buffer.at((channel_index * 3) + 2) = led_color.blue;
					break;
				case 2:
					led_ch2_buffer.at((channel_index * 3) + 0) = led_color.green;
					led_ch2_buffer.at((channel_index * 3) + 1) = led_color.red;
					led_ch2_buffer.at((channel_index * 3) + 2) = led_color.blue;
					break;
				case 3:
					led_ch3_buffer.at((channel_index * 3) + 0) = led_color.green;
					led_ch3_buffer.at((channel_index * 3) + 1) = led_color.red;
					led_ch3_buffer.at((channel_index * 3) + 2) = led_color.blue;
					break;

				default:
					ESP_LOGE(kTag, "Unknown channel: %d", channel);
					break;
				}
			}
		}
	}

	// Logs for debugging which are printing the values of the channels
	// ESP_LOG_BUFFER_HEX("dump_ch1", &led_ch1_buffer, sizeof(led_ch1_buffer));
	// ESP_LOG_BUFFER_HEX("dump_ch2", &led_ch2_buffer, sizeof(led_ch2_buffer));
	// ESP_LOG_BUFFER_HEX("dump_ch3", &led_ch3_buffer, sizeof(led_ch3_buffer));

	// Send data to LEDs
	if (led_ch1 == nullptr || led_ch2 == nullptr || led_ch3 == nullptr) {
		ESP_LOGE(kTag, "Invalid RMT state");
		return ESP_FAIL;
	}

	ESP_LOGD(kTag, "Enable LED channels");
	ESP_ERROR_CHECK(rmt_enable(led_ch1));
	ESP_ERROR_CHECK(rmt_enable(led_ch2));
	ESP_ERROR_CHECK(rmt_enable(led_ch3));

	rmt_transmit_config_t tx_config = {};
	tx_config.loop_count = 0;
	tx_config.flags.eot_level = 0;

	// Flush RGB values to LEDs
	ESP_ERROR_CHECK(rmt_transmit(led_ch1, led_encoder_ch1, led_ch1_buffer.data(), led_ch1_buffer.size(), &tx_config));
	ESP_ERROR_CHECK(rmt_transmit(led_ch2, led_encoder_ch2, led_ch2_buffer.data(), led_ch2_buffer.size(), &tx_config));
	ESP_ERROR_CHECK(rmt_transmit(led_ch3, led_encoder_ch3, led_ch3_buffer.data(), led_ch3_buffer.size(), &tx_config));
	ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_ch1, portMAX_DELAY));
	ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_ch2, portMAX_DELAY));
	ESP_ERROR_CHECK(rmt_tx_wait_all_done(led_ch3, portMAX_DELAY));

	ESP_LOGD(kTag, "Disable LED channels");
	ESP_ERROR_CHECK(rmt_disable(led_ch3));
	ESP_ERROR_CHECK(rmt_disable(led_ch2));
	ESP_ERROR_CHECK(rmt_disable(led_ch1));

	return ESP_OK;
}
