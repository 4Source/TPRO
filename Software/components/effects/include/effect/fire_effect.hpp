#pragma once

#include "effect.hpp"
#include <vector>

class FireEffect : public Effect {
  public:
	FireEffect() = default;
	~FireEffect() override = default;

	FireEffect(const FireEffect &) = delete;
	FireEffect &operator=(const FireEffect &) = delete;
	FireEffect(FireEffect &&) = delete;
	FireEffect &operator=(FireEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "fire";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/fire.json";

  private:
	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;
	static constexpr size_t WIDTH = 87;
	static constexpr size_t HEIGHT = 35;
	std::array<std::array<int, WIDTH>, HEIGHT> heat_{};
	std::string name_{"Fire Effect"};

	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};

	Param<uint32_t> cycle_time_{90, {"number", 1, 60000, 1}};

	Param<uint32_t> led_range_start_{0, {"number", 0, 3044, 1}};

	Param<uint32_t> led_range_end_{3044, {"number", 0, 3044, 1}};

	Param<uint8_t> speed_{1, {"number", 1, 10, 1}};

	Param<uint8_t> scale_{1, {"number", 1, 10, 1}};

	Param<uint8_t> cooling_{55, {"number", 0, 255, 1}};

	Param<uint8_t> sparking_{120, {"number", 0, 255, 1}};
};
