#pragma once

#include "effect.hpp"
#include <vector>

class BlinkingEffect : public Effect {
  public:
	BlinkingEffect() = default;
	~BlinkingEffect() override = default;

	BlinkingEffect(const BlinkingEffect &) = delete;
	BlinkingEffect &operator=(const BlinkingEffect &) = delete;
	BlinkingEffect(BlinkingEffect &&) = delete;
	BlinkingEffect &operator=(BlinkingEffect &&) = delete;

	std::unique_ptr<LedFrame> get_led_data(DateTime time_stamp) override;

	esp_err_t serialize(std::string path) override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

  private:
	std::string path_;
	// Use when desereializing is implemented
	std::array<char, 128> path_buffer_;

	u_int8_t default_brightness_{100};
	u_int32_t cycle_time_{1000};
	std::string name_{"Blinking Effect"};
	u_int32_t led_range_start_{0};
	u_int32_t led_range_end_{1000};

	RGB color_on_{.red = static_cast<u_int8_t>(255), .green = static_cast<u_int8_t>(255), .blue = static_cast<u_int8_t>(255)};
	RGB color_off_{.red = static_cast<u_int8_t>(0), .green = static_cast<u_int8_t>(0), .blue = static_cast<u_int8_t>(0)};
};
