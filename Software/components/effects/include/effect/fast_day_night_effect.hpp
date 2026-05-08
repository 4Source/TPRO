#pragma once
#include "day_night_types.hpp"
#include "effect.hpp"

/// @brief Spezieller Effect: Schneller/Beschleunigter Tag-Nacht Effekt
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager
/// erkannt
class FastDayNightEffect : public Effect {
  public:
	FastDayNightEffect();
	~FastDayNightEffect() override = default;

	FastDayNightEffect(const FastDayNightEffect &) = delete;
	FastDayNightEffect &operator=(const FastDayNightEffect &) = delete;
	FastDayNightEffect(FastDayNightEffect &&) = delete;
	FastDayNightEffect &operator=(FastDayNightEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	static constexpr const char *kType = "fast_day_night";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/fast_day_night.json";

  private:
	static esp_err_t request_api();

	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;

	std::string version{"1.0"};
	std::string name_{"Day/Night Effect"};
	Param<uint32_t> default_speed_{1, {"number", 1, 10, 1}};
	Param<uint8_t> default_brightness_{100, {"number", 0, 255, 1}};
	// Parameter: speedup_ bestimmt die Simulationsgeschwindigkeit
	// 1 = Echtzeit, 1440 = 1 Tag pro Minute
	Param<uint32_t> speedup_{3600, {"number", 1, 10000, 1}};

	float accumulated_minutes_ = 0.0f;
	uint32_t last_millis_ = 0;
	uint16_t call_counter_ = 0;

	int32_t last_sim_day_ = -1;
	DateTime::TimeComponents start_date_{};

	int timeStringToMinutes(const char *iso_string);
};
