#include "effect_factory.hpp"
#include "effect_parser.hpp"
#include "file_manager.hpp"

#if defined(UNIT_TEST)
#include <iostream>
#endif

#include <cstring>

// Konkrete Effect hier hinzufügen!!!
#include "blinking_effect.hpp"
#include "day_night_effect.hpp"
#include "timeline_effect.hpp"

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

	if (json_obj_get_object(&jctx, "effect") != OS_SUCCESS) {
		json_parse_end(&jctx);
		return nullptr;
	}

	if (json_obj_get_string(&jctx, "type", type_buffer.data(), type_buffer.size()) != OS_SUCCESS) {
		json_parse_end(&jctx);
		return nullptr;
	}

#if defined(UNIT_TEST)
	std::cerr << "DEBUG: Extracted type: [" << type_buffer.data() << "]" << std::endl;
#endif

	std::shared_ptr<Effect> effect = nullptr;

	if (strcmp(type_buffer.data(), "day_night") == 0) {
		effect = std::make_shared<DayNightEffect>();
	} else if (strcmp(type_buffer.data(), "blink") == 0) {
		effect = std::make_shared<BlinkingEffect>();
	} else if (strcmp(type_buffer.data(), "timeline") == 0) {
		effect = std::make_shared<TimelineEffect>();
	} else {
		// Unbekannter Effekt-Typ
		json_parse_end(&jctx);
		return nullptr;
	}

	json_parse_end(&jctx);

	if (effect) {
		effect->deserialize(std::move(path));
	}

	return effect;
}