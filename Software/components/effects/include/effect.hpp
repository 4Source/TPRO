#pragma once
#include "datetime.hpp"
#include "effect_parser.hpp"
#include "file_manager.hpp"
#include "led_frame.hpp"
#include <cstdint>
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

	virtual std::unique_ptr<LedFrame> get_led_data(DateTime time_stamp) = 0; // auf heap legen - zu groß für stack

	virtual esp_err_t serialize(std::string path) = 0;
	virtual esp_err_t deserialize(std::string path) = 0;
	virtual esp_err_t set_parameter(const char *name, const char *value) = 0;

	virtual esp_err_t set_filepath(std::string path) = 0;
	virtual std::string get_filepath() = 0;
};
