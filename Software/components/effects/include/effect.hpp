#pragma once
#include "datetime.hpp"
#include "effect_parser.hpp"
#include "file_manager.hpp"
#include "led_frame.hpp"
#include "param.hpp"
#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <esp_err.h>
#include <memory>
#include <string>
#include <vector>

class Effect {
  public:
	Effect() = default;
	virtual ~Effect() = default;

	Effect(const Effect &) = delete;
	Effect &operator=(const Effect &) = delete;
	Effect(Effect &&) = delete;
	Effect &operator=(Effect &&) = delete;

	virtual esp_err_t get_led_data(
		LedFrame &frame,
		DateTime::TimeComponents time_stamp) = 0; // auf heap legen - zu groß für stack Roher Pointer - borrowing der unique Pointer der Effektframes

	virtual esp_err_t serialize() = 0;
	virtual esp_err_t deserialize(std::string path) = 0;
	virtual esp_err_t set_parameter(const char *name, const char *value) = 0;

	virtual esp_err_t set_filepath(std::string path) = 0;
	virtual std::string get_filepath() = 0;

	virtual std::string get_name() = 0;

	static constexpr const char *kTag = "effect";
};
