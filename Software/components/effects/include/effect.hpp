#pragma once
// TODO #include "FileManager.hpp"
#include "datetime.hpp"
#include "effect_parser.hpp"
#include "led_frame.hpp"
#include <cstdint>
#include <esp_err.h>
#include <memory>
#include <string>
#include <vector>

class Effect {
  public:
	virtual ~Effect() = default;

	virtual std::unique_ptr<LedFrame> get_led_data(DateTime time_stamp) = 0; // auf heap legen - zu groß für stack

	virtual esp_err_t serialize(const char *path) = 0;
	virtual esp_err_t deserialize(const char *path) = 0;
	virtual esp_err_t set_parameter(const char *name, const char *value) = 0;

	virtual esp_err_t set_filepath(const char *path) = 0;
	virtual const std::string get_filepath() = 0;
};
