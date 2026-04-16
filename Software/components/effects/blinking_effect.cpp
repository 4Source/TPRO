#include "blinking_effect.hpp"
#include <esp_err.h>

BlinkingEffect::BlinkingEffect() {
	color_on = {
		.red = 255,
		.green = 255,
		.blue = 255,
	}; // white
	color_off = {
		.red = 0,
		.green = 0,
		.blue = 0,
	}; // black
}

std::unique_ptr<LedFrame> BlinkingEffect::get_led_data(DateTime time_stamp) {
	auto frame = std::make_unique<LedFrame>();

	// Implements a simple blinking logic: 1 second ON, 1 second OFF
	bool is_on = (time_stamp.second % 2) == 0;
	RGB current_color = is_on ? color_on : color_off;

	for (auto &led_row : frame->led_data) {
		for (auto &led_pos : led_row) {
			led_pos = current_color;
		}
	}

	return frame;
}

esp_err_t BlinkingEffect::serialize(const char *path) {
	// Not implemented for this simple test effect
	return ESP_OK;
}

esp_err_t BlinkingEffect::deserialize(const char *path) {
	// Not implemented for this simple test effect
	return ESP_OK;
}

esp_err_t BlinkingEffect::set_filepath(const char *path) {
	if (path != nullptr) {
		_path = const_cast<char *>(path);
		return ESP_OK;
	}
	return ESP_ERR_INVALID_ARG;
}

std::string BlinkingEffect::get_filepath() { return _path; }

esp_err_t BlinkingEffect::set_parameter(const char *name, const char *value) {
	// Not implemented for this simple test effect
	return ESP_OK;
}
