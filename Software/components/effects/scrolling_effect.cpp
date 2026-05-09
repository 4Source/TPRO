#include "effect/scrolling_effect.hpp"
#include "effect_parser.hpp"
#include "file_manager.hpp"
#include <cstring>
#include <esp_log.h>

constexpr const char *kTag = "SCROLLING_EFFECT";

ScrollingEffect::ScrollingEffect() {

	if (!lv_is_initialized()) {
		lv_init();
	}

	uint32_t logical_width = physical_width_ / kScaleX;
	uint32_t logical_height = kLogicalHeight;

	disp_ = lv_display_create(static_cast<int32_t>(logical_width), static_cast<int32_t>(logical_height));
	lv_display_set_color_format(disp_, LV_COLOR_FORMAT_RGB888);

	// 3Bytes pro Pixel (RGB888)
	size_t buffer_size_bytes = logical_width * logical_height * 3;
	draw_buffer_.resize(buffer_size_bytes);
	lv_display_set_buffers(disp_, draw_buffer_.data(), nullptr, buffer_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

	// Callbacks registrieren
	lv_display_set_flush_cb(disp_, flush_cb);
	lv_display_set_user_data(disp_, this);

	// UI aufbauen
	screen_ = lv_obj_create(nullptr);
	lv_obj_set_style_bg_color(screen_, lv_color_black(), 0);
	lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE); // Verhindert Scrollbalken

	label_ = lv_label_create(screen_);

	// Nutzt jetzt die Member-Variable anstatt eines hardcodierten Strings
	lv_label_set_text(label_, text_.value.c_str());

	// Lauftext Einstellungen
	lv_label_set_long_mode(label_, LV_LABEL_LONG_CLIP);
	lv_obj_set_width(label_, 1000); // wide virtual canvas
	lv_obj_align(label_, LV_ALIGN_LEFT_MID, 0, 0);

	// Montserrat 14 in sdkconfig aktiviert
	lv_obj_set_style_text_font(label_, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(label_, lv_color_make(255, 255, 255), 0);
	lv_display_set_default(disp_);
	lv_screen_load(screen_);

	// Leeren LedFrame initialisieren
	current_frame_ = std::make_unique<LedFrame>();
}

ScrollingEffect::~ScrollingEffect() {
	if (disp_ != nullptr) {
		lv_display_delete(disp_);
	}
	if (screen_ != nullptr) {
		lv_obj_delete(screen_);
	}
}

// Flush
void ScrollingEffect::flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) { // NOLINT(readability-non-const-parameter)
	auto *effect = static_cast<ScrollingEffect *>(lv_display_get_user_data(disp));

	if ((effect == nullptr) || (effect->current_frame_ == nullptr)) {
		lv_display_flush_ready(disp);
		return;
	}

	uint32_t logical_width = (area->x2 - area->x1) + 1;

	// Wir iterieren über das LOGISCHE LVGL-Bild (Das kleine Bild)
	for (int32_t logical_y = area->y1; logical_y <= area->y2; logical_y++) {
		for (int32_t logical_x = area->x1; logical_x <= area->x2; logical_x++) {

			// Calculate the 1D index for the flat px_map array based on current x,y relative to the area
			int32_t relative_x = logical_x - area->x1;
			int32_t relative_y = logical_y - area->y1;
			uint32_t px_index = static_cast<uint32_t>((relative_y * logical_width) + relative_x) * 3;

			// RGB aus LVGL extrahieren (ohne pointer arithmetic i.e. px_index++)
			// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			uint8_t red_val = px_map[px_index + 0];
			uint8_t green_val = px_map[px_index + 1];
			uint8_t blue_val = px_map[px_index + 2];
			// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)

			RGB current_color{.red = red_val, .green = green_val, .blue = blue_val};

			// UPSCALING: Die Farbe in die tatsächlichen (physischen) LEDs schreiben
			for (int32_t delta_y = 0; delta_y < static_cast<int32_t>(kScaleY); delta_y++) {
				for (int32_t delta_x = 0; delta_x < static_cast<int32_t>(kScaleX); delta_x++) {

					int32_t phys_x = (logical_x * static_cast<int32_t>(kScaleX)) + delta_x;
					int32_t phys_y = (logical_y * static_cast<int32_t>(kScaleY)) + delta_y;

					if ((phys_y >= 0) && (phys_x >= 0) && (static_cast<size_t>(phys_y) < effect->current_frame_->led_data.size()) &&
						(static_cast<size_t>(phys_x) < effect->current_frame_->led_data[0].size())) {

						// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
						effect->current_frame_->led_data[phys_y][phys_x] = current_color;
					}
				}
			}
		}
	}

	lv_display_flush_ready(disp);
}

esp_err_t ScrollingEffect::get_led_data(LedFrame &frame, DateTime::TimeComponents time_stamp) {
	if (frame.led_data.empty() || frame.led_data[0].empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	const uint32_t current_ms = (static_cast<uint32_t>(time_stamp.second) * 1000U) + time_stamp.millisecond;

	if (last_scroll_ms_ == 0U) {
		last_scroll_ms_ = current_ms;
	}

	uint32_t delta_ms = 0;

	if (current_ms >= last_scroll_ms_) {
		delta_ms = current_ms - last_scroll_ms_;
	} else {
		// minute wrap fallback (your RTC-like source)
		delta_ms = (60000U + current_ms - last_scroll_ms_);
	}

	last_scroll_ms_ = current_ms;

	const float speed = scroll_speed_.value; // pixels per second

	// accumulate smooth motion
	scroll_offset_ += static_cast<int32_t>((static_cast<float>(delta_ms) * speed) / 35.0F);

	const int32_t max_scroll = lv_obj_get_width(label_);

	if (max_scroll > 0) {
		if (scroll_offset_ >= max_scroll) {
			scroll_offset_ -= max_scroll;
		} else if (scroll_offset_ < 0) {
			scroll_offset_ += max_scroll;
		}
	}

	lv_obj_set_x(label_, -scroll_offset_);
	lv_refr_now(nullptr);

	frame.led_data = current_frame_->led_data;
	return ESP_OK;
}

esp_err_t ScrollingEffect::set_parameter(const char *name, const char *value) {
	if (name == nullptr || value == nullptr) {
		return ESP_ERR_INVALID_ARG;
	}
	if (strcmp(name, "text") == 0) {
		text_.value = value;
		lv_label_set_text(label_, text_.value.c_str());
		return ESP_OK;
	}
	if (strcmp(name, "scroll_speed") == 0) {
		char *end_ptr = nullptr;
		float new_speed = std::strtof(value, &end_ptr);
		if (end_ptr != value) {
			new_speed = std::max(new_speed, 0.1F);
			new_speed = std::min(new_speed, 10.0F);
			scroll_speed_.value = new_speed;
			return ESP_OK;
		}
		return ESP_FAIL;
	}
	return ESP_ERR_NOT_FOUND;
}

esp_err_t ScrollingEffect::set_filepath(std::string path) {
	if (path.empty()) {
		return ESP_ERR_INVALID_ARG;
	}
	path_ = std::move(path);
	return ESP_OK;
}

std::string ScrollingEffect::get_filepath() { return path_; }

std::string ScrollingEffect::get_name() { return this->name_; }

// -----------------
// JSON
// -----------------
esp_err_t ScrollingEffect::deserialize(std::string path) {
	auto opt_json = FileManager::read_file(path);
	if (!opt_json) {
		return ESP_FAIL;
	}
	this->path_ = path;

	EffectParser::cJSON_ptr root(cJSON_Parse(opt_json->c_str()), cJSON_Delete);
	if (!root) {
		return ESP_FAIL;
	}

	auto *params = cJSON_GetObjectItem(root.get(), "parameters");
	if (!params) {
		return ESP_OK;
	}

	if (auto *item = cJSON_GetObjectItem(params, "text"); cJSON_IsString(item)) {
		text_.value = item->valuestring;
		if (label_ != nullptr) {
			lv_label_set_text(label_, text_.value.c_str());
		}
	}

	if (auto *item = cJSON_GetObjectItem(params, "scroll_speed"); cJSON_IsNumber(item))
		scroll_speed_.value = static_cast<float>(item->valuedouble);

	return ESP_OK;
}

esp_err_t ScrollingEffect::serialize() {
	EffectParser::cJSON_ptr root(cJSON_CreateObject(), cJSON_Delete);

	cJSON_AddStringToObject(root.get(), "type", kType);

	auto *params = cJSON_AddObjectToObject(root.get(), "parameters");

	cJSON_AddStringToObject(params, "text", text_.value.c_str());
	cJSON_AddNumberToObject(params, "scroll_speed", scroll_speed_.value);

	// ---- SCHEMA ----
	auto *schema = cJSON_AddObjectToObject(root.get(), "schema");

	auto add_schema = [&](const char *key, const ParameterSchema &schema_param) {
		auto *object = cJSON_AddObjectToObject(schema, key);
		cJSON_AddStringToObject(object, "type", schema_param.type.c_str());

		if (schema_param.type == "number" || schema_param.type == "range") {
			cJSON_AddNumberToObject(object, "min", schema_param.min);
			cJSON_AddNumberToObject(object, "max", schema_param.max);
			cJSON_AddNumberToObject(object, "step", schema_param.step);
		}
	};

	add_schema("text", text_.schema);
	add_schema("scroll_speed", scroll_speed_.schema);

	EffectParser::cJSON_str_ptr json(cJSON_PrintUnformatted(root.get()), free);

	return FileManager::save_file(this->path_, json.get());
}