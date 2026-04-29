#pragma once
#include "config_observer.hpp"
#include "config_type.hpp"
#include <algorithm>
#include <array>
#include <esp_err.h>
#include <esp_log.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class ConfigManager {
  private:
	ConfigType config;
	std::unordered_map<std::string, std::vector<ConfigObserver *>> observer_list;

	// All possible Keys are listed in this array,
	static constexpr std::array<std::string, 4> kEys{"current_effect", "effects_path", "speed", "brightness"};

	// necessary for switch case
	enum KEY : std::uint8_t { CURRENT_EFFECT, EFFECTS_PATH, SPEED, BRIGHTNESS };
	// Map string keys to enum values for switch case

	std::unordered_map<std::string, KEY> stringToEnum = {
		{"current_effect", CURRENT_EFFECT}, {"effects_path", EFFECTS_PATH}, {"speed", SPEED}, {"brightness", BRIGHTNESS}};

	static constexpr const char *kTag = "config-manager";

  public:
	ConfigManager();

	/**
	 * Notify all registered observers about tha changes to there keys that changed.
	 *
	 * @note Use this when all keys are changed
	 */
	void notify_observers();

	/**
	 * Notify all registered observers about tha changes to there keys that changed.
	 *
	 * @param keys The keys that changed
	 */
	void notify_observers(std::vector<std::string> &keys);

	/**
	 * Notify all registered observers about the changes to the key
	 *
	 * @param key The key that changed
	 */
	void notify_observers(const std::string &key);

	/**
	 * Allows an observer to register for changes of a specific configuration
	 *
	 * @param key The name of the configuration the observer wants to know when the value changes
	 * @param observer The observer to notify when changes to he value of the configuration happen
	 * @retval `true` when the observer is registered successfully to the key
	 * @retval `false` when the observer allready was registered to the key
	 */
	esp_err_t add_observer(const std::string &key, ConfigObserver &observer);

	/**
	 * Allows an observer to unregister for changes of a specific configuration
	 *
	 * @param key The name of the configuration the observer wants to unregister
	 * @param observer The observer to unregister for the changes of a configuration
	 * @retval `true` when the observer is unregistered successfully from the key
	 * @retval `false` when the key has no observers
	 * @retval `false` when the observer is not registered to the key
	 */
	esp_err_t remove_observer(const std::string &key, const ConfigObserver &observer);

	//(GET)
	std::string get_config(const std::string &key) const {
		std::string result;
		if (!stringToEnum.contains(key)) {
			ESP_LOGE(kTag, "Tried to access Unknown key");
			return result;
		}

		KEY enum_key = stringToEnum.at(key);
		switch (enum_key) {
		case CURRENT_EFFECT:
			result = config.current_effect.string();
			break;
		case EFFECTS_PATH:
			result = config.effects_path.string();
			break;
		case SPEED:
			result = std::to_string(config.speed);
			break;
		case BRIGHTNESS:
			result = std::to_string(config.brightness);
			break;
		default:
			break;
		}

		return result;
	}

	//(PUT)
	esp_err_t set_config(const std::string &key, const std::string &value);

	//(POST)
	esp_err_t set_config(ConfigType new_config);

	//(DELETE)
	esp_err_t set_to_default(const std::string &key);

	esp_err_t serialize(const std::string &path) const;
	esp_err_t deserialize(const std::string &path);
};