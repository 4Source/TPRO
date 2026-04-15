#pragma once

#include "effect.hpp"
#include <vector>

class BlinkingEffect : public Effect {
  public:
	BlinkingEffect();
	~BlinkingEffect() override = default;

	LedFrame get_led_data(DateTime time_stamp) override;

	esp_err_t serialize(const char *path) override;
	esp_err_t deserialize(const char *path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(const char *path) override;
	const std::string get_filepath() override;

  private:
	std::string _path;
	// Use when desereializing is implemented
	// std::array<char, 128> _path_buffer;
	RGB color_on;
	RGB color_off;
};
