#pragma once
#include "effect.hpp"

#pragma pack(push, 1) // Verhindert leeres Padding durch den Compiler
struct DayNightData {
	uint8_t x_pos;
	uint8_t y_pos;
	int16_t sunrise_minute;
	int16_t sunset_minute;
	float lat;
	float lon;
};
#pragma pack(pop)
/// @brief Spezieller Effect: Tag-Nacht Effekt
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager
/// erkannt
class DayNightEffect : public Effect {
  public:
	DayNightEffect();
	~DayNightEffect() override = default;

	DayNightEffect(const DayNightEffect &) = delete;
	DayNightEffect &operator=(const DayNightEffect &) = delete;
	DayNightEffect(DayNightEffect &&) = delete;
	DayNightEffect &operator=(DayNightEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "day_night";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/day_night.json";

  private:
	static esp_err_t request_api();

	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;

	std::string version{"1.0"};
	std::string name_{"Day/Night Effect"};
	Param<uint32_t> default_speed_{1, {"number", 1, 10, 1}};
	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};

	std::shared_ptr<std::vector<DayNightData>> _led_data;
	esp_err_t load_day_from_file(const std::string &path);

	esp_err_t load_leds_streaming(const std::string &path);
	
	int8_t last_loaded_day_ = -1; // Für ist neuer Tag?
	uint16_t call_counter_ = 0;

	int timeStringToMinutes(const char *iso_string);
};
