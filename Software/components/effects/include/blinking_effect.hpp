#pragma once

#include "effect.hpp"
#include <string>
#include <vector>

class BlinkingEffect : public Effect {
  public:
	BlinkingEffect();
	~BlinkingEffect() override = default;

	LedFrame get_led_data(DateTime time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(const char *json_text) override;

	std::vector<Effect *> &get_subeffects() override;
	esp_err_t set_subeffect(Effect *effect) override;

	esp_err_t set_filepath(const char *path) override;
	const char *get_filepath() override;

  private:
	std::vector<Effect *> subeffects;
	std::string filepath;

	RGB color_on;
	RGB color_off;
};
