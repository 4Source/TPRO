#pragma once
#include "led_frame.hpp"
#include <cstdint>
#include <esp_err.h>
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

  virtual esp_err_t serialize() = 0;
  virtual esp_err_t deserialize(const char *json_text) = 0;

  // Pointer auf das Interface
  virtual std::vector<Effect *> &get_subeffects() = 0;
  virtual esp_err_t set_subeffect(Effect *effect) = 0;

  virtual esp_err_t set_filepath(const char *path) = 0;
  virtual const char *get_filepath() = 0;
};