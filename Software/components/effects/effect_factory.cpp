#include "effect_factory.hpp"
#include "effect_parser.hpp"

#if defined(UNIT_TEST)
#include "file_manager.hpp"
#include <iostream>
#endif

#include <cstring>

// Konkrete Effect hier hinzufügen!!!
#include "blinking_effect.hpp"
#include "day_night_effect.hpp"
#include "timeline_effect.hpp"

std::unique_ptr<Effect> EffectFactory::generate_from_json(const char *path) {
	_type_buffer = {}; // Buffer leeren vor Benutzung
#if defined(UNIT_TEST)
	const char *json_text = FileManager::read_file(path);
#else
	const char *json_text = nullptr;
#endif
	if (json_text == nullptr) {
		return nullptr;
	}

	jparse_ctx_t jctx;
	if (json_parse_start(&jctx, json_text, static_cast<int>(strlen(json_text))) != OS_SUCCESS) {
		return nullptr;
	}

	if (json_obj_get_object(&jctx, "effect") != OS_SUCCESS) {
		json_parse_end(&jctx);
		return nullptr;
	}

	if (json_obj_get_string(&jctx, "type", _type_buffer.data(), _type_buffer.size()) != OS_SUCCESS) {
		json_parse_end(&jctx);
		return nullptr;
	}

#if defined(UNIT_TEST)
	std::cerr << "DEBUG: Extracted type: [" << _type_buffer.data() << "]" << std::endl;
#endif

	std::unique_ptr<Effect> effect = nullptr;

	if (strcmp(_type_buffer.data(), "day_night") == 0) {
		effect = std::make_unique<DayNightEffect>();
	} else if (strcmp(_type_buffer.data(), "blinking") == 0) {
		effect = std::make_unique<BlinkingEffect>();
	} else if (strcmp(_type_buffer.data(), "timeline") == 0) {
		effect = std::make_unique<TimelineEffect>();
	} else {
		// Unbekannter Effekt-Typ
		json_parse_end(&jctx);
		return nullptr;
	}

	json_parse_end(&jctx);

	if (effect) {
		effect->deserialize(path);
	}

	return effect;
}