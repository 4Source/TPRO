#pragma once

#include "effect.hpp"
#include <vector>

class ColumnScanEffect : public Effect {
  public:
	ColumnScanEffect() = default;
	~ColumnScanEffect() override = default;

	ColumnScanEffect(const ColumnScanEffect &) = delete;
	ColumnScanEffect &operator=(const ColumnScanEffect &) = delete;
	ColumnScanEffect(ColumnScanEffect &&) = delete;
	ColumnScanEffect &operator=(ColumnScanEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "column_scan";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/column_scan.json";

  private:
	std::string path{kDefaultConfigPath};
	std::array<char, 128> path_buffer;

	std::string version{"1.0"};
	std::string name{"Column scan Effect"};

	// Parameters
	RGB color_on{.red = static_cast<uint8_t>(255), .green = static_cast<uint8_t>(255), .blue = static_cast<uint8_t>(255)};
	RGB color_off{.red = static_cast<uint8_t>(0), .green = static_cast<uint8_t>(0), .blue = static_cast<uint8_t>(0)};
	uint32_t cycle_time{1000};
	uint8_t default_brightness{100};

	// Internals
	uint32_t active_row = 0;
	uint32_t active_col = 0;
};
