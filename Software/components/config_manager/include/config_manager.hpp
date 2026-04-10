#pragma once
#include "config_observer.hpp"
#include "config_type.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

class ConfigManager {
  private:
	ConfigType config;
	std::unordered_map<std::string, std::vector<ConfigObserver *>> observer_list;

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
	bool add_observer(const std::string &key, ConfigObserver &observer);

	/**
	 * Allows an observer to unregister for changes of a specific configuration
	 *
	 * @param key The name of the configuration the observer wants to unregister
	 * @param observer The observer to unregister for the changes of a configuration
	 * @retval `true` when the observer is unregistered successfully from the key
	 * @retval `false` when the key has no observers
	 * @retval `false` when the observer is not registered to the key
	 */
	bool remove_observer(const std::string &key, const ConfigObserver &observer);

	//(GET)
	auto get_config(const std::string &key) const {
		// maybe change to a map if amount of keys becomes too long
		if (key == "current_effect") {
			return config.current_effect;
		}
		if (key == "effects_path") {
			return config.effects_path;
		}
		return std::filesystem::path("");
	}

	//(PUT)
	void set_config(const std::string &key, const std::string &value);

	//(POST)
	void set_config(ConfigType new_config);

	//(DELETE)
	void set_to_default(const std::string &key);
};