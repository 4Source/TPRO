#include "effect/row_scan_effect.hpp"
#include <esp_err.h>
#include <esp_log.h>

esp_err_t RowScanEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	frame.clear_led_data();

	for (uint32_t frame_row = 0; frame_row < frame.led_data.size(); ++frame_row) {
		auto &led_row = frame.led_data.at(frame_row);
		for (uint32_t frame_col = 0; frame_col < led_row.size(); ++frame_col) {
			auto &led_color = led_row.at(frame_col);
			if (active_row == frame_row && active_col == frame_col) {
				led_color = color_on;
			} else {
				led_color = color_off;
			}
		}
	}

	// TODO: Increase precision of time_stamp
	if (time_stamp.millisecond % cycle_time == 0) {
		active_col++;
		// End of row set to start of next row
		if (active_col >= frame.led_data.at(active_row).size()) {
			active_col = 0;
			active_row++;
		}
		// Last row set to first row
		if (active_row >= frame.led_data.size()) {
			active_row = 0;
		}
	}

	return ESP_OK;
}

esp_err_t RowScanEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	this->path = path;
	return ESP_OK;
}

std::string RowScanEffect::get_filepath() { return this->path; }

std::string RowScanEffect::get_name() { return this->name; }

esp_err_t RowScanEffect::set_parameter(const char *name, const char *value) {
	// TODO: implemented for this effect
	return ESP_OK;
}

// -----------------
// JSON
// -----------------
esp_err_t RowScanEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	if (!opt_json) {
		ESP_LOGW(kTag, "Read failed");
		return ESP_FAIL;
	}
	this->path = path;

	EffectParser::cJSON_ptr root(cJSON_Parse(opt_json->c_str()), cJSON_Delete);
	if (!root) {
		return ESP_FAIL;
	}

	if (auto *item = cJSON_GetObjectItem(root.get(), "name"); cJSON_IsString(item))
		this->name = item->valuestring;

	if (auto *item = cJSON_GetObjectItem(root.get(), "version"); cJSON_IsString(item))
		this->version = item->valuestring;

	auto *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (!params) {
		return ESP_OK;
	}

	if (auto *arr = cJSON_GetObjectItem(params, "color_on"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 3) {
		this->color_on.red = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		this->color_on.green = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
		this->color_on.blue = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 2)->valuedouble);
	}

	if (auto *arr = cJSON_GetObjectItem(params, "color_off"); cJSON_IsArray(arr) && cJSON_GetArraySize(arr) >= 3) {
		this->color_off.red = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 0)->valuedouble);
		this->color_off.green = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 1)->valuedouble);
		this->color_off.blue = static_cast<uint8_t>(cJSON_GetArrayItem(arr, 2)->valuedouble);
	}

	if (auto *item = cJSON_GetObjectItem(params, "cycle_time"); cJSON_IsNumber(item))
		this->cycle_time = static_cast<uint32_t>(item->valuedouble);

	if (auto *item = cJSON_GetObjectItem(params, "default_brightness"); cJSON_IsNumber(item))
		this->default_brightness = static_cast<uint8_t>(item->valuedouble);

	return ESP_OK;
}

esp_err_t RowScanEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "version", this->version.c_str());
	cJSON_AddStringToObject(root.get(), "name", this->name.c_str());
	cJSON_AddStringToObject(root.get(), "type", kType);
	cJSON *params = cJSON_AddObjectToObject(root.get(), "parameters");

	// Build _color_on array
	cJSON *color_on_arr = cJSON_AddArrayToObject(params, "color_on");
	cJSON_AddItemToArray(color_on_arr, cJSON_CreateNumber(this->color_on.red));
	cJSON_AddItemToArray(color_on_arr, cJSON_CreateNumber(this->color_on.green));
	cJSON_AddItemToArray(color_on_arr, cJSON_CreateNumber(this->color_on.blue));

	// Build _color_off array
	cJSON *color_off_arr = cJSON_AddArrayToObject(params, "color_off");
	cJSON_AddItemToArray(color_off_arr, cJSON_CreateNumber(this->color_off.red));
	cJSON_AddItemToArray(color_off_arr, cJSON_CreateNumber(this->color_off.green));
	cJSON_AddItemToArray(color_off_arr, cJSON_CreateNumber(this->color_off.blue));

	cJSON_AddNumberToObject(params, "cycle_time", this->cycle_time);
	cJSON_AddNumberToObject(params, "default_brightness", this->default_brightness);

	EffectParser::cJSON_str_ptr json_string(cJSON_PrintUnformatted(root.get()), free);
	return FileManager::save_file(this->path, json_string.get());
}
