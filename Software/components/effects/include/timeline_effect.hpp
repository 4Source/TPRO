#pragma once
#include "effect.hpp"

// Jeder Effekt im vektor _subeffects hat eine cycle time
struct TimelineStep {
	std::unique_ptr<Effect> effect;
	uint32_t duration_ms;
	uint32_t start_ms;
};

/// @brief Spezieller Effect: Timeline aus n verschiedenen Sub-Effekten
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager
/// erkannt
class TimelineEffect : public Effect {
  public:
	TimelineEffect() : _path(""), _path_buffer() {}
	~TimelineEffect() override = default;
	std::unique_ptr<LedFrame> get_led_data(DateTime time) override;

	esp_err_t serialize(const char *path) override;
	esp_err_t deserialize(const char *path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(const char *path) override;
	const std::string get_filepath() override;

  private:
	std::vector<Effect *> _subeffects;
	std::vector<std::pair<std::string, std::string>> _overwrites; // Key Value pairs für überschreiben von Sub-Effekt Parametern
	std::string _path;
	std::array<char, 128> _path_buffer;
	std::vector<TimelineStep> _steps;
	uint32_t _total_duration_ms = 0;
	esp_err_t delete_subeffect(Effect *effect);
	esp_err_t set_subeffect(Effect *effect);
	std::vector<Effect *> &get_subeffects();
};
