#pragma once
// TODO #include "FileManager.hpp"
#include "effect_parser.hpp"
#include "led_frame.hpp"
#include <cstdint>
#include <esp_err.h>
#include <memory>
#include <string>
#include <vector>
// Timestamp
struct DateTime {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
};

class Effect {
  public:
	virtual ~Effect() = default;

	virtual LedFrame get_led_data(DateTime time_stamp) = 0;

	virtual esp_err_t serialize(const char *path) = 0;
	virtual esp_err_t deserialize(const char *path) = 0;
	virtual esp_err_t set_parameter(const char *name, const char *value) = 0;

	virtual esp_err_t set_filepath(const char *path) = 0;
	virtual const std::string get_filepath() = 0;
};
