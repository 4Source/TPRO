#pragma once

#include "effect.hpp"
#include <vector>

class StaticColorEffect : public Effect {
  public:
	StaticColorEffect() = default;
	~StaticColorEffect() override = default;

	StaticColorEffect(const StaticColorEffect &) = delete;
	StaticColorEffect &operator=(const StaticColorEffect &) = delete;
	StaticColorEffect(StaticColorEffect &&) = delete;
	StaticColorEffect &operator=(StaticColorEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "static_color";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/static_color.json";

  private:
	std::string path{kDefaultConfigPath};
	// Use when desereializing is implemented
	std::array<char, 128> path_buffer;

	std::string version{"1.0"};
	std::string name{"Static color Effect"};

	// Parameters
	RGB color{.red = static_cast<uint8_t>(255), .green = static_cast<uint8_t>(255), .blue = static_cast<uint8_t>(255)};
	uint8_t default_brightness{100};
};
