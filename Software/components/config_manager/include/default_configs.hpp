#include <string>

namespace DefaultConfigs {
const std::string kDefaultDayNightConfigPath = "/effects/day_night.json";
const std::string kDefaultBlinkConfigPath = "/effects/blink.json";
const std::string kDefaultTimelineConfigPath = "/effects/main.json";

const std::string kDefaultDayNightConfig = R"({
  "version": "1.0",
  "name": "Day Night",
  "effect": {
      "type": "day_night",
      "parameters": { "default_brightness": 100, "default_speed": 1000 }
  }
})";

const std::string kDefaultBlinkConfig = R"({
  "version": "1.0",
  "name": "Blink",
  "effect": {
      "type": "blink",
      "parameters": {
        "color": "#222222",
        "cycle_time": 5000,
        "led_range": [0, 3024],
        "default_brightness": 90
      }
  }
})";

const std::string kDefaultTimelineConfig = R"({
  "version": "1.0",
  "name": "Main Timeline",
  "effect": {
      "type": "timeline",
      "parameters": {
        "primary": { "id": 0, "cycle_time": 500 },
        "secondary": [ { "id": 1, "cycle_time": 500, "overwrite": { "color": "#FF0000" } } ]
      }
  },
  "subeffects": [
    { "type": "day_night", "path": "/effects/day_night.json" },
    { "type": "blink", "path": "/effects/blink.json" }
  ]
})";
} // namespace DefaultConfigs