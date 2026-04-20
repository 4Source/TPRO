#include "config_manager.hpp"
#include "./config_observer.hpp"
#include "./config_type.hpp"
#include <algorithm>
#include <map>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

ConfigManager::ConfigManager() {
	this->config = ConfigType(); // default
}

void ConfigManager::notify_observers() {
	if (observer_list.empty()) {
		// exit early because no entries
		return;
	}

	std::vector<std::string> keys;
	keys.reserve(observer_list.size());

	// Collect all keys at least one observer has registered to
	for (const auto &entry : observer_list) {
		keys.push_back(entry.first);
	}

	notify_observers(keys);
}

void ConfigManager::notify_observers(std::vector<std::string> &keys) {
	std::map<ConfigObserver *, std::vector<std::string>> observer_keys;

	for (const std::string &key : keys) {
		// Check if observer list contains the key
		if (!observer_list.contains(key)) {
			continue;
		}

		// The current key to all observers that are registered for it
		std::vector<ConfigObserver *> key_observers = observer_list.find(key)->second;
		for (ConfigObserver *observer : key_observers) {
			observer_keys[observer].push_back(key);
		}
	}

	// notify all observers about there keys that changed
	for (const auto &[observer, keys] : observer_keys) {
		observer->update(keys);
	}
}

void ConfigManager::notify_observers(const std::string &key) {
	// Check if observer list contains the key
	if (!observer_list.contains(key)) {
		return;
	}

	// Notify all observers which are registered for the key
	std::vector<ConfigObserver *> key_observers = observer_list.find(key)->second;
	for (ConfigObserver *observer : key_observers) {
		observer->update(key);
	}
}

esp_err_t ConfigManager::add_observer(const std::string &key, ConfigObserver &observer) {

	// Check if observer list allready contains the key if not insert the observer
	if (!observer_list.contains(key)) {
		// No creation of empty vector needed see: https://en.cppreference.com/w/cpp/container/unordered_map/operator_at.html
		observer_list[key].push_back(&observer);
		return ESP_OK;
	}

	auto it_key_observers = std::find(observer_list[key].begin(), observer_list[key].end(), &observer);

	// Check if observer list for this key allready contains the observer
	if (it_key_observers != observer_list[key].end()) {
		// Exit early because observer allready registered
		return ESP_OK;
	}

	observer_list[key].push_back(&observer);
	return ESP_OK;
}

esp_err_t ConfigManager::remove_observer(const std::string &key, const ConfigObserver &observer) {
	// Check if observer list contains the key
	if (!observer_list.contains(key)) {
		return ESP_OK;
	}

	auto it_key_observers = std::find(observer_list[key].begin(), observer_list[key].end(), &observer);

	// Check if observer list for this key contains the observer
	if (it_key_observers == observer_list[key].end()) {
		// Exit early because observer allready registered
		return ESP_OK;
	}

	// Remove the observer from the vector
	observer_list[key].erase(it_key_observers);
	return ESP_OK;
}

//(PUT)
esp_err_t ConfigManager::set_config(const std::string &key, const std::string &value) {
	if (!stringToEnum.contains(key)) {
		ESP_LOGE(kTag, "Tried to access Unknown key");
		return ESP_FAIL;
	}
	KEY enum_key = stringToEnum.at(key);
	switch (enum_key) {
	case CURRENT_EFFECT: {
		config.current_effect = std::filesystem::path(value);
		break;
	}
	case EFFECTS_PATH: {
		config.effects_path = std::filesystem::path(value);
		break;
	}
	case SPEED: {
		// check if value is a number
		if (std::ranges::all_of(value, ::isdigit)) {
			config.speed = std::stoi(value);
		} else {
			ESP_LOGW(kTag, "Invalid value");
			return ESP_FAIL;
		}
		break;
	}
	case BRIGHTNESS: {
		// check if value is a number
		if (std::ranges::all_of(value, ::isdigit)) {
			config.brightness = std::stoi(value);
		} else {
			ESP_LOGW(kTag, "Invalid value");
			return ESP_FAIL;
		}
		break;
	}
	default: {
		ESP_LOGE(kTag, "Tried to access Unknown key");
		return ESP_FAIL;
	}
	}

	notify_observers(key); // update observers
	return ESP_OK;
}

//(POST)
esp_err_t ConfigManager::set_config(ConfigType new_config) {
	config = std::move(new_config);
	notify_observers();
	return ESP_OK;
}

//(DELETE)
esp_err_t ConfigManager::set_to_default(const std::string &key) {
	if (!stringToEnum.contains(key)) {
		ESP_LOGE(kTag, "Tried to access Unknown key");
		return ESP_FAIL;
	}
	KEY enum_key = stringToEnum.at(key);
	switch (enum_key) {
	case CURRENT_EFFECT: {
		config.current_effect = std::filesystem::path("");
		break;
	}
	case EFFECTS_PATH: {
		config.effects_path = std::filesystem::path("");
		break;
	}
	case SPEED: {
		config.speed = 0;
		break;
	}
	case BRIGHTNESS: {
		config.brightness = 0;
		break;
	}
	default: {
		ESP_LOGE(kTag, "Tried to access Unknown key");
		return ESP_FAIL;
	}
	}
	notify_observers();
	return ESP_OK;
}
