#include "config_manager.hpp"
#include "./config_observer.hpp"
#include "./config_type.hpp"
#include <algorithm>
#include <map>
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

bool ConfigManager::add_observer(const std::string &key, ConfigObserver &observer) {

	// Check if observer list allready contains the key if not insert the observer
	if (!observer_list.contains(key)) {
		// No creation of empty vector needed see: https://en.cppreference.com/w/cpp/container/unordered_map/operator_at.html
		observer_list[key].push_back(&observer);
		return true;
	}

	auto it_key_observers = std::find(observer_list[key].begin(), observer_list[key].end(), &observer);

	// Check if observer list for this key allready contains the observer
	if (it_key_observers != observer_list[key].end()) {
		// Exit early because observer allready registered
		return false;
	}

	observer_list[key].push_back(&observer);
	return true;
}

bool ConfigManager::remove_observer(const std::string &key, const ConfigObserver &observer) {
	// Check if observer list contains the key
	if (!observer_list.contains(key)) {
		return false;
	}

	auto it_key_observers = std::find(observer_list[key].begin(), observer_list[key].end(), &observer);

	// Check if observer list for this key contains the observer
	if (it_key_observers == observer_list[key].end()) {
		// Exit early because observer allready registered
		return false;
	}

	// Remove the observer from the vector
	observer_list[key].erase(it_key_observers);
	return true;
}

//(PUT)
void ConfigManager::set_config(const std::string &key, const std::string &value) {
	if (key == "current_effect") {
		config.current_effect = std::filesystem::path(value);
	} else if (key == "effects_path") {
		config.effects_path = std::filesystem::path(value);
	} else {
		// key doesn't exist, maybe return bool?
	}
	notify_observers(key); // update observers
}

//(POST)
void ConfigManager::set_config(ConfigType new_config) {
	config = std::move(new_config);
	notify_observers();
}

//(DELETE)
void ConfigManager::set_to_default(const std::string &key) {
	if (key == "current_effect") {
		config.current_effect = std::filesystem::path("");
	} else if (key == "effects_path") {
		config.effects_path = std::filesystem::path("");
	} else {
		// key doesn't exist, maybe return bool?
	}
	notify_observers();
}
