#include "effect/static_color_effect.hpp"
#include <esp_err.h>
#include <esp_log.h>

esp_err_t StaticColorEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}

	for (auto &led_row : frame.led_data) {
		for (auto &led_cell : led_row) {
			led_cell = color;
		}
	}

	return ESP_OK;
}

esp_err_t StaticColorEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	this->path = path;
	return ESP_OK;
}

std::string StaticColorEffect::get_filepath() { return this->path; }

std::string StaticColorEffect::get_name() { return this->name; }

esp_err_t StaticColorEffect::set_parameter(const char *name, const char *value) {
	// TODO: implemented for this effect
	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t StaticColorEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	if (!opt_json) {
		ESP_LOGW(kTag, "Read failed");
		return ESP_FAIL;
	}
	this->path = path;

	EffectParser::cJSON_ptr root(cJSON_Parse(opt_json->c_str()), cJSON_Delete);
	if (!root) { return ESP_FAIL; }

	if (auto *item = cJSON_GetObjectItem(root.get(), "name"); cJSON_IsString(item))
		this->name = item->valuestring;

	if (auto *item = cJSON_GetObjectItem(root.get(), "version"); cJSON_IsString(item))
		this->version = item->valuestring;

	auto *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (!params) { return ESP_OK; }

	if (auto *arr = cJSON_GetObjectItem(params, "color"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 3) {
		this->color.red   = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		this->color.green = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
		this->color.blue  = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 2)->valuedouble);
	}

	if (auto *item = cJSON_GetObjectItem(params, "default_brightness"); cJSON_IsNumber(item))
		this->default_brightness = static_cast<uint8_t>(item->valuedouble);

	return ESP_OK;
}

esp_err_t StaticColorEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", this->version.c_str());
	cJSON_AddStringToObject(root.get(), "name", this->name.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);
	cJSON *params = cJSON_AddObjectToObject(root.get(), "parameters");

	// Build color array
	cJSON *color_arr = cJSON_AddArrayToObject(params, "color");
	cJSON_AddItemToArray(color_arr, cJSON_CreateNumber(this->color.red));
	cJSON_AddItemToArray(color_arr, cJSON_CreateNumber(this->color.green));
	cJSON_AddItemToArray(color_arr, cJSON_CreateNumber(this->color.blue));

	cJSON_AddNumberToObject(params, "default_brightness", this->default_brightness);

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(this->path, json_string.get());
}
