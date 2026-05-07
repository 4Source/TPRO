#pragma once

#include "effect.hpp"
#include <vector>

class DVDEffect : public Effect {
  public:
	DVDEffect() = default;
	~DVDEffect() override = default;

	DVDEffect(const DVDEffect &) = delete;
	DVDEffect &operator=(const DVDEffect &) = delete;
	DVDEffect(DVDEffect &&) = delete;
	DVDEffect &operator=(DVDEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "dvd";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/dvd.json";

  private:
	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;
	static constexpr int WIDTH = 86, HEIGHT = 34;

	std::string name_{"DVD Effect"};

	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};

	Param<uint8_t> speed_{1, {"number", 1, 10, 1}};

	Param<uint32_t> vel_x_{1, {"number", 0, 20, 1}};

	Param<uint32_t> vel_y_{1, {"number", 0, 20, 1}};

	Param<uint32_t> pos_x_{0, {"number", 0, 100, 1}};

	Param<uint32_t> pos_y_{0, {"number", 0, 100, 1}};

	Param<RGB> col_rgb_{RGB{.red = static_cast<uint8_t>(255), .green = static_cast<uint8_t>(0), .blue = static_cast<uint8_t>(0)}, {"color"}};
};
