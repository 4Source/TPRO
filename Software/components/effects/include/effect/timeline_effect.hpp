#pragma once
#include "effect.hpp"
#include "effect/blinking_effect.hpp"
#include <map>

// Jeder Effekt im vektor _subeffects hat eine cycle time
struct TimelineStep {
	std::shared_ptr<Effect> effect;
	uint32_t duration_ms;
	uint32_t start_ms;
	uint32_t id;
};

struct TimelineSecondaryItem {
	uint32_t id;
	uint32_t cycle_time;
	// Map für n beliebige Key-Value Paare
	std::map<std::string, std::string> overwrites;
};

struct TimelineSubeffectConfig {
	std::string type;
	std::string path;
};

/// @brief Spezieller Effect: Timeline aus n verschiedenen Sub-Effekten
/// Muss Effect implementieren: sonst nicht vom Licht-Effekt Manager erkannt
class TimelineEffect : public Effect {
  public:
	TimelineEffect() = default;
	~TimelineEffect() override = default;

	TimelineEffect(const TimelineEffect &) = delete;
	TimelineEffect &operator=(const TimelineEffect &) = delete;
	TimelineEffect(TimelineEffect &&) = delete;
	TimelineEffect &operator=(TimelineEffect &&) = delete;

	esp_err_t get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) override;

	esp_err_t serialize() override;
	esp_err_t deserialize(std::string path) override;

	esp_err_t set_parameter(const char *name, const char *value) override;

	esp_err_t set_filepath(std::string path) override;
	std::string get_filepath() override;

	std::string get_name() override;

	// Pure Getter - keine internen veränderungen!
	[[nodiscard]] const std::vector<std::shared_ptr<Effect>> &get_subeffects() const;

	static constexpr const char *kType = "timeline";
	static constexpr const char *kDefaultConfigPath = "/effects/defaults/timeline.json";

  private:
	std::vector<std::shared_ptr<Effect>> subeffects_;
	std::vector<std::pair<std::string, std::string>> overwrites_; // Key Value pairs für überschreiben von Sub-Effekt Parametern
	std::string path_{kDefaultConfigPath};
	std::array<char, 128> path_buffer_;
	std::vector<TimelineStep> steps_;
	uint32_t primary_id_{0};
	std::string version_{"1.0"};
	std::string name_{"Timeline Effect"};

	std::vector<TimelineSubeffectConfig> subeffect_configs_;
	std::vector<TimelineSecondaryItem> secondary_items_;

	uint32_t total_duration_ms_{0};
	uint32_t primary_cycle_time_{0};
	uint8_t speed_{1};

	esp_err_t delete_subeffect(Effect *effect);
	esp_err_t set_subeffect(const std::shared_ptr<Effect> &effect);
	esp_err_t initialize_subeffects(); // init subeffects aus subeffect_configs

	// json helper
	esp_err_t parse_static_fields(jparse_ctx_t *jctx);
	esp_err_t parse_dynamic_secondary(const char *raw_json);

	std::string current_active_name_{""}; // Logging
};
