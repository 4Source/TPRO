#pragma once

#include "effect.hpp"
#include <vector>

class EqualizerEffect : public Effect {
  public:
	EqualizerEffect() = default;
	~EqualizerEffect() override = default;

	EqualizerEffect(const EqualizerEffect &) = delete;
	EqualizerEffect &operator=(const EqualizerEffect &) = delete;
	EqualizerEffect(EqualizerEffect &&) = delete;
	EqualizerEffect &operator=(EqualizerEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "equalizer";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/equalizer.json";

  private:
	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;
	std::vector<size_t> heights_;
	std::string name_{"Equalizer Effect"};

	// TODO: uint8_t default_brightness_{100};

	Param<uint32_t> led_range_start_{0, {"number", 0, 3044, 1}};

	Param<uint32_t> led_range_end_{3044, {"number", 0, 3044, 1}};

	Param<uint8_t> speed_{1, {"number", 1, 10, 1}};

	Param<uint8_t> scale_{1, {"number", 1, 10, 1}};

	Param<uint8_t> decay_{1, {"number", 0, 10, 1}};
};
