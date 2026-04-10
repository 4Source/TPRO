#pragma once
#include "effect.hpp"
#include "effect_parser.hpp"
/// @brief Spezieller Effect: Tag-Nacht Effekt
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager
/// erkannt
class DayNightEffect : public Effect {
  public:
	LedFrame get_led_data(DateTime time) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(const char *json_text) override;

	esp_err_t set_subeffect(Effect *effect) override;
	std::vector<Effect *> &get_subeffects() override;

	esp_err_t set_filepath(const char *path) override;
	const char *get_filepath() override;

  private:
	static esp_err_t request_api();
	std::vector<Effect *> _subeffects;
	char *_path;
};
