#pragma once

#include "effect.hpp"
#include <vector>

class RainbowWaveEffect : public Effect {
  public:
	RainbowWaveEffect() = default;
	~RainbowWaveEffect() override = default;

	RainbowWaveEffect(const RainbowWaveEffect &) = delete;
	RainbowWaveEffect &operator=(const RainbowWaveEffect &) = delete;
	RainbowWaveEffect(RainbowWaveEffect &&) = delete;
	RainbowWaveEffect &operator=(RainbowWaveEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	[[nodiscard]] std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "rainbow";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/rainbow.json";

  private:
	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;
	std::string name_{"Rainbow Wave Effect"};
	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};
	Param<uint32_t> cycle_time_{1000, {"number", 1, 600000, 1}};
	Param<uint32_t> led_range_start_{0, {"number", 0, 3044, 1}};
	Param<uint32_t> led_range_end_{3044, {"number", 0, 3044, 1}};
	Param<uint8_t> speed_{1, {"number", 1, 10, 1}};
	Param<uint8_t> scale_{1, {"number", 1, 10, 1}};
};
