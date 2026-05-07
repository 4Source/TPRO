#include "effect/debug_effect.hpp"
#include <esp_err.h>
#include <esp_log.h>

esp_err_t DebugEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	for (uint32_t frame_row = 0; frame_row < frame.led_data.size(); ++frame_row) {
		auto &led_row = frame.led_data.at(frame_row);
		for (uint32_t frame_col = 0; frame_col < led_row.size(); ++frame_col) {
			uint8_t channel = 0;
			if (frame_row < CONFIG_LED_CH1_ROWS) {
				channel = 1;
			} else if (frame_row < CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS) {
				channel = 2;
			} else if (frame_row < CONFIG_LED_CH1_ROWS + CONFIG_LED_CH2_ROWS + CONFIG_LED_CH3_ROWS) {
				channel = 3;
			}

			led_row.at(frame_col) = RGB{.red = channel, .green = static_cast<uint8_t>(frame_row), .blue = static_cast<uint8_t>(frame_col)};
		}
	}

	return ESP_OK;
}

esp_err_t DebugEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	this->path = path;
	return ESP_OK;
}

std::string DebugEffect::get_filepath() { return path; }

std::string DebugEffect::get_name() { return this->name; }

esp_err_t DebugEffect::set_parameter(const char *name, const char *value) { return ESP_OK; }

// -----------------
// JSON
// -----------------
esp_err_t DebugEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);

	if (!opt_json) {
		// Read failed
		ESP_LOGW(kTag, "Read failed");
		return ESP_FAIL;
	}

	this->path = path;

	return EffectParser::parse_with_defaults(opt_json.value().c_str(), path_buffer.data(), path_buffer.size(), [this](jparse_ctx_t *jctx) {
		// version
		std::array<char, 32> version_buf{};
		if (json_obj_get_string(jctx, "version", version_buf.data(), version_buf.size()) == 0) {
			this->version = version_buf.data();
		}

		// name
		std::array<char, 32> name_buf{};
		if (json_obj_get_string(jctx, "name", name_buf.data(), name_buf.size()) == 0) {
			this->name = name_buf.data();
		}

		// type
		std::array<char, 32> type_buf{};
		if (json_obj_get_string(jctx, "type", type_buf.data(), type_buf.size()) == 0) {
			if (type_buf.data() != kType) {
				ESP_LOGW(Effect::kTag, "Type miss match durring effect parsing! expected: %s received: %s", kType, type_buf);
				return ESP_FAIL;
			}
		}

		return ESP_OK;
	});
}

esp_err_t DebugEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", this->version.c_str());
	cJSON_AddStringToObject(root.get(), "name", this->name.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);
	cJSON_AddObjectToObject(root.get(), "parameters");

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(this->path, json_string.get());
}
