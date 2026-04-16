#pragma once
#include "effect.hpp"
/// @brief Spezieller Effect: Tag-Nacht Effekt
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager
/// erkannt
class DayNightEffect : public Effect {
  public:
	DayNightEffect() = default;
	~DayNightEffect() override = default;

	DayNightEffect(const DayNightEffect &) = delete;
	DayNightEffect &operator=(const DayNightEffect &) = delete;
	DayNightEffect(DayNightEffect &&) = delete;
	DayNightEffect &operator=(DayNightEffect &&) = delete;

	std::unique_ptr<LedFrame> get_led_data(DateTime time) override;

	esp_err_t serialize(const char *path) override;
	esp_err_t deserialize(const char *path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(const char *path) override;
	std::string get_filepath() override;

  private:
	static esp_err_t request_api();
	std::string _path;
	std::array<char, 128> _path_buffer;
};
