#pragma once

#include "effect.hpp"
#include <array>
#include <vector>

class BlinkingEffect : public Effect {
  public:
	BlinkingEffect() = default;
	~BlinkingEffect() override = default;

	BlinkingEffect(const BlinkingEffect &) = delete;
	BlinkingEffect &operator=(const BlinkingEffect &) = delete;
	BlinkingEffect(BlinkingEffect &&) = delete;
	BlinkingEffect &operator=(BlinkingEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "blink";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/blink.json";

  private:
	std::string path{kDefaultConfigPath};
	// Use when desereializing is implemented
	std::array<char, 128> path_buffer_;
	std::string name_{"Blinking Effect"};
	Param<uint32_t> cycle_time_{1000, {"number", 100, 10000, 100}};

	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};

	Param<std::array<uint32_t, 2>> led_range_{{0, 3044}, {"range", 0, 3044, 1}};

	Param<RGB> color_on_{RGB{255, 255, 255}, {"color"}};

	Param<RGB> color_off_{RGB{0, 0, 0}, {"color"}};
};
