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

LedFrame BlinkingEffect::get_led_data(DateTime time_stamp) {
	LedFrame frame;

	// Implements a simple blinking logic: 1 second ON, 1 second OFF
	bool is_on = (time_stamp.second % 2) == 0;
	RGB current_color = is_on ? color_on : color_off;

	for (auto &led_row : frame.led_data) {
		for (auto &led_pos : led_row) {
			led_pos = current_color;
		}
	}

	return frame;
}

esp_err_t BlinkingEffect::serialize() {
	// Not implemented for this simple test effect
	return ESP_OK;
}

esp_err_t BlinkingEffect::deserialize(const char *json_text) {
	// Not implemented for this simple test effect
	return ESP_OK;
}

std::vector<Effect *> &BlinkingEffect::get_subeffects() { return subeffects; }

esp_err_t BlinkingEffect::set_subeffect(Effect *effect) {
	if (effect != nullptr) {
		subeffects.push_back(effect);
		return ESP_OK;
	}
	return ESP_ERR_INVALID_ARG;
}

esp_err_t BlinkingEffect::set_filepath(const char *path) {
	if (path != nullptr) {
		filepath = path;
		return ESP_OK;
	}
	return ESP_ERR_INVALID_ARG;
}

const char *BlinkingEffect::get_filepath() { return filepath.c_str(); }
