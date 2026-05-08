#include "effect_factory.hpp"
#include "effect_parser.hpp"
#include "file_manager.hpp"
#include <esp_log.h>
#include <format>

#if defined(UNIT_TEST)
#include <iostream>
#endif

#include <cstring>

// Konkrete Effect hier hinzufügen!!!
#include "effect/blinking_effect.hpp"
#include "effect/breathing_effect.hpp"
#include "effect/color_wipe_effect.hpp"
#include "effect/column_scan_effect.hpp"
#include "effect/continent_effect.hpp"
#include "effect/day_night_effect.hpp"
#include "effect/debug_effect.hpp"
#include "effect/dvd_effect.hpp"
#include "effect/equalizer_effect.hpp"
#include "effect/fast_day_night_effect.hpp"
#include "effect/fire_effect.hpp"
#include "effect/matrix_effect.hpp"
#include "effect/plasma_effect.hpp"
#include "effect/rainbow_wave_effect.hpp"
#include "effect/row_scan_effect.hpp"
#include "effect/scrolling_effect.hpp"
#include "effect/static_color_effect.hpp"
#include "effect/timeline_effect.hpp"

// Registry
const std::unordered_map<std::string, EffectCreator> EffectFactory::registry = {
	{BlinkingEffect::kType, []() { return std::make_shared<BlinkingEffect>(); }},
	{BreathingEffect::kType, []() { return std::make_shared<BreathingEffect>(); }},
	{ColorWipeEffect::kType, []() { return std::make_shared<ColorWipeEffect>(); }},
	{ColumnScanEffect::kType, []() { return std::make_shared<ColumnScanEffect>(); }},
	{ContinentEffect::kType, []() { return std::make_shared<ContinentEffect>(); }},
	{DayNightEffect::kType, []() { return std::make_shared<DayNightEffect>(); }},
	{DebugEffect::kType, []() { return std::make_shared<DebugEffect>(); }},
	{DVDEffect::kType, []() { return std::make_shared<DVDEffect>(); }},
	{EqualizerEffect::kType, []() { return std::make_shared<EqualizerEffect>(); }},
	{FastDayNightEffect::kType, []() { return std::make_shared<FastDayNightEffect>(); }},
	{FireEffect::kType, []() { return std::make_shared<FireEffect>(); }},
	{MatrixEffect::kType, []() { return std::make_shared<MatrixEffect>(); }},
	{PlasmaEffect::kType, []() { return std::make_shared<PlasmaEffect>(); }},
	{RainbowWaveEffect::kType, []() { return std::make_shared<RainbowWaveEffect>(); }},
	{RowScanEffect::kType, []() { return std::make_shared<RowScanEffect>(); }},
	{ScrollingEffect::kType, []() { return std::make_shared<ScrollingEffect>(); }},
	{StaticColorEffect::kType, []() { return std::make_shared<StaticColorEffect>(); }},
	{TimelineEffect::kType, []() { return std::make_shared<TimelineEffect>(); }},
};

std::shared_ptr<Effect> EffectFactory::generate_from_json(std::string path) {
	type_buffer = {}; // Buffer leeren vor Benutzung
	auto opt_json = FileManager::read_file(path);

	if (!opt_json.has_value()) {
		return nullptr;
	}

	const char *json_text = opt_json.value().c_str();

	if (json_text == nullptr) {
		return nullptr;
	}

	jparse_ctx_t jctx;
	if (json_parse_start(&jctx, json_text, static_cast<int>(strlen(json_text))) != OS_SUCCESS) {
		return nullptr;
	}

	if (json_obj_get_string(&jctx, "type", type_buffer.data(), type_buffer.size()) != OS_SUCCESS) {
		json_parse_end(&jctx);
		return nullptr;
	}

#if defined(UNIT_TEST)
	std::cerr << "DEBUG: Extracted type: [" << type_buffer.data() << "]" << std::endl;
#endif

	auto iterator = registry.find(type_buffer.data());
	if (iterator == registry.end()) {
		json_parse_end(&jctx);
		return nullptr;
	}

	std::shared_ptr<Effect> effect = iterator->second();

	json_parse_end(&jctx);

	if (effect) {
		if (effect->deserialize(std::move(path)) != ESP_OK) {
			ESP_LOGE(kTag, "Failed to deserialize the effect");
		}
	}

	return effect;
}

esp_err_t EffectFactory::writeDefaults(const std::string &directory) {
	esp_err_t result = ESP_OK;

	for (const auto &[name, creator] : registry) {
		std::string path = std::format("{}/{}.json", directory, name);
		ESP_LOGD(kTag, "Check if effect config is available: %s", path.c_str());

		if (FileManager::is_file(path) == ESP_OK) {
			continue;
		}

		auto effect = creator();
		ESP_LOGD(kTag, "Created effect: %s", name.c_str());

		if (!effect) {
			result = ESP_FAIL;
			continue;
		}

		esp_err_t err = effect->serialize();
		if (err != ESP_OK) {
			result = err;
		}
	}

	return result;
}
