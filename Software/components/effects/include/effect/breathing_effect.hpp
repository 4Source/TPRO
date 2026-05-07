#pragma once

#include "effect.hpp"
#include <vector>

class BreathingEffect : public Effect {
  public:
	BreathingEffect() = default;
	~BreathingEffect() override = default;

	BreathingEffect(const BreathingEffect &) = delete;
	BreathingEffect &operator=(const BreathingEffect &) = delete;
	BreathingEffect(BreathingEffect &&) = delete;
	BreathingEffect &operator=(BreathingEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "breathing";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/breathing.json";

  private:
	std::string path_{kDefaultConfigPath};
	// Use when desereializing is implemented
	std::array<char, 128> path_buffer_;
	std::string name_{"Breathing Effect"};

	Param<uint32_t> cycle_time_{5000, {"number", 100, 10000, 100}};

	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};

	Param<uint8_t> speed_{1, {"number", 1, 10, 1}};

	Param<uint8_t> scale_{1, {"number", 1, 10, 1}};

	Param<RGB> color_{RGB{.red = static_cast<uint8_t>(255), .green = static_cast<uint8_t>(255), .blue = static_cast<uint8_t>(255)}, {"color"}};
};
