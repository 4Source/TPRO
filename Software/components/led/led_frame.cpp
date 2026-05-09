#include "led_frame.hpp"
#include "esp_log.h"
#include <cinttypes>
#include <cstdio>
#include <print>
#include <ranges>

// NOLINTBEGIN[cppcoreguidelines-prefer-member-initializer]
LedFrame::LedFrame() : led_data{} {
	m_resource_mutex = xSemaphoreCreateBinary();
	xSemaphoreGive(m_resource_mutex); // Initialize as "available"

	m_reader_mutex = xSemaphoreCreateBinary();
	xSemaphoreGive(m_reader_mutex); // Initialize as "available"
}
// NOLINTEND[cppcoreguidelines-prefer-member-initializer]

LedFrame::~LedFrame() {
	if (m_resource_mutex != nullptr) {
		vSemaphoreDelete(m_resource_mutex);
	}
	if (m_reader_mutex != nullptr) {
		vSemaphoreDelete(m_reader_mutex);
	}
}

void LedFrame::copy_data_from(const LedFrame &other) { this->led_data = other.led_data; }

void LedFrame::print_led_data_csv_table() {
	LedFrame::ScopedReadLock read_lock(*this);
	ESP_LOGW(kTag, "start of csv");

	std::print("row/column");
	for (size_t column = 0; column < CONFIG_LED_COLUMNS; ++column) {
		std::print(",{}", column);
	}
	std::println("");

	for (size_t row = 0; row < CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS; ++row) {
		for (size_t column = 0; column < CONFIG_LED_COLUMNS; ++column) {
			const auto &led_color = this->led_data[row][column]; // NOLINT(cppcoreguidelines-pro-bounds-constant-array-index)
			if (column == 0) {
				std::print("{}", row);
			}
			std::print(",#{:02X}{:02X}{:02X}", led_color.red, led_color.green, led_color.blue);
		}
		std::println("");
	}
	ESP_LOGW(kTag, "end of csv");
}

void LedFrame::lock_write() { xSemaphoreTake(m_resource_mutex, portMAX_DELAY); }

void LedFrame::unlock_write() { xSemaphoreGive(m_resource_mutex); }

void LedFrame::lock_read() {
	xSemaphoreTake(m_reader_mutex, portMAX_DELAY);
	m_active_readers++;

	if (m_active_readers == 1) {
		xSemaphoreTake(m_resource_mutex, portMAX_DELAY);
	}
	xSemaphoreGive(m_reader_mutex);
}

void LedFrame::unlock_read() {
	xSemaphoreTake(m_reader_mutex, portMAX_DELAY);
	m_active_readers--;
	if (m_active_readers == 0) {
		xSemaphoreGive(m_resource_mutex);
	}
	xSemaphoreGive(m_reader_mutex);
}

void LedFrame::clear_led_data() {
	// Zero out the frame at the beginning
	std::ranges::fill(this->led_data | std::views::join, RGB{.red = 0, .green = 0, .blue = 0});
}
