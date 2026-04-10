#include "config_manager.hpp"
#include "./config_observer.hpp"
#include "./config_type.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

ConfigManager::ConfigManager() {
	this->config = ConfigType(); // default
}

void ConfigManager::notify_observers(const std::string &key) {

	auto iterator = observerlist.find(key);
	for (ConfigObserver *observer : iterator->second) {
		observer->update(key);
	}
}

bool ConfigManager::add_observer(const std::string &key, ConfigObserver &observer) {
	if (observerlist.contains(key)) { // ACHTUNG: Hier wurd contains anstatt find verwendent um clang-tidy zufrieden zu stellen
		observerlist[key].push_back(&observer);
		return true;
	}
	return false;
}

bool ConfigManager::remove_observer(const std::string &key, const ConfigObserver &observer) {
	auto iterator = observerlist.find(key); // returns iterator, iterator->first is key, iterator->second is vector
	if (iterator == observerlist.end()) {
		return false; // key not found
	}
	auto vec_spot_to_remove = std::find(iterator->second.begin(), iterator->second.end(), &observer); // find the observer
	if (vec_spot_to_remove != iterator->second.end()) {
		iterator->second.erase(vec_spot_to_remove); // remove the observer from the vector
	}
	return true;
}

//(GET)
auto ConfigManager::get_config(const std::string &key) const {
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
	notify_observers("current_effect");
	notify_observers("effects_path");
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
	notify_observers(key);
}
