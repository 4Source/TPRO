#pragma once

#include "effect.hpp"
#include <vector>

class ColorWipeEffect : public Effect {
  public:
	ColorWipeEffect() = default;
	~ColorWipeEffect() override = default;

	ColorWipeEffect(const ColorWipeEffect &) = delete;
	ColorWipeEffect &operator=(const ColorWipeEffect &) = delete;
	ColorWipeEffect(ColorWipeEffect &&) = delete;
	ColorWipeEffect &operator=(ColorWipeEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "wipe";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/wipe.json";

  private:
	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;
	std::string name_{"Color Wipe Effect"};

	Param<uint8_t> bri_default_{100, {"number", 0, 255, 1}};

	Param<uint32_t> cyc_time_{1000, {"number", 100, 10000, 100}};

	Param<uint32_t> led_start_{0, {"number", 0, 3044, 1}};

	Param<uint32_t> led_end_{3044, {"number", 0, 3044, 1}};

	Param<uint8_t> speed_{1, {"number", 1, 10, 1}};

	Param<uint8_t> scale_{1, {"number", 1, 10, 1}};

	Param<RGB> col_a_{RGB{.red = static_cast<uint8_t>(255), .green = static_cast<uint8_t>(255), .blue = static_cast<uint8_t>(255)}, {"color"}};

	Param<RGB> col_b_{RGB{.red = static_cast<uint8_t>(0), .green = static_cast<uint8_t>(0), .blue = static_cast<uint8_t>(0)}, {"color"}};
};
