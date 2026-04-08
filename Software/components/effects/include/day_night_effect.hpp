#pragma once
#include "effect_parser.hpp"
#include "effect.hpp"
/// @brief Spezieller Effect: Tag-Nacht Effekt
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager
/// erkannt
class DayNightEffect : public Effect {
public:
  LedFrame get_led_data(DateTime t);

  esp_err_t serialize();
  esp_err_t deserialize(const char *json_text) override;

  esp_err_t set_subeffect(Effect *effect);
  std::vector<Effect *> &get_subeffects();

  esp_err_t set_filepath(const char *path);
  const char *get_filepath();

private:
  esp_err_t request_api();
  std::vector<Effect *> _subeffects;
  char *_path;
};
