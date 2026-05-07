#pragma once

#include "effect.hpp"
#include <vector>

class DebugEffect : public Effect {
  public:
	DebugEffect() = default;
	~DebugEffect() override = default;

	DebugEffect(const DebugEffect &) = delete;
	DebugEffect &operator=(const DebugEffect &) = delete;
	DebugEffect(DebugEffect &&) = delete;
	DebugEffect &operator=(DebugEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "debug";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/debug.json";

  private:
	std::string path{kDefaultConfigPath};
	std::array<char, 128> path_buffer;

	std::string version{"1.0"};
	std::string name{"Debug Effect"};
};
