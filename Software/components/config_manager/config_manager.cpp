#include "config_manager.hpp"
#include "./config_observer.hpp"
#include "./config_type.hpp"
#include "cJSON.h"
#include "file_manager.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>

ConfigManager::ConfigManager() {
	this->config = ConfigType(); // default
}

void ConfigManager::notify_observers() {
	if (observer_list.empty()) {
		return;
	}

	for (const auto &[key, observers] : observer_list) {
		for (ConfigObserver *observer : observers) {
			observer->update(key);
		}
	}
}

void ConfigManager::notify_observers(const std::vector<std::string> &keys) {
	for (const std::string &key : keys) {

		auto it = observer_list.find(key);
		if (it == observer_list.end()) {
			continue;
		}

		// No copy
		const auto &key_observers = it->second;

		for (ConfigObserver *observer : key_observers) {
			observer->update(key);
		}
	}
}

void ConfigManager::notify_observers(const std::string &key) {

	auto it = observer_list.find(key);
	if (it == observer_list.end()) {
		return;
	}

	// No copy
	const auto &key_observers = it->second;

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
	ESP_LOGD(kTag, "set_config %s=%s", key.c_str(), value.c_str());
	KEY enum_key = stringToEnum.at(key);
	switch (enum_key) {
	case CURRENT_EFFECT:
		config.current_effect = std::filesystem::path(value);
		break;
	case EFFECTS_PATH:
		config.effects_path = std::filesystem::path(value);
		break;
	case ACTIVE_FROM:
		config.active_from = value;
		break;
	case ACTIVE_TO:
		config.active_to = value;
		break;
	default:
		ESP_LOGE(kTag, "Tried to access Unknown key");
		return ESP_FAIL;
	}

	notify_observers(key); // update observers
	serialize();
	return ESP_OK;
}

//(POST)
esp_err_t ConfigManager::set_config(ConfigType new_config) {
	config = std::move(new_config);
	notify_observers();
	serialize();
	return ESP_OK;
}

//(DELETE)
esp_err_t ConfigManager::set_to_default(const std::string &key) {
	if (!stringToEnum.contains(key)) {
		ESP_LOGE(kTag, "Tried to access Unknown key");
		return ESP_FAIL;
	}
	ConfigType default_config{};
	KEY enum_key = stringToEnum.at(key);
	switch (enum_key) {
	case CURRENT_EFFECT:
		config.current_effect = default_config.current_effect;
		break;
	case EFFECTS_PATH:
		config.effects_path = default_config.effects_path;
		break;
	case ACTIVE_FROM:
		config.active_from = default_config.active_from;
		break;
	case ACTIVE_TO:
		config.active_to = default_config.active_to;
		break;
	default:
		ESP_LOGE(kTag, "Tried to access Unknown key");
		return ESP_FAIL;
	}
	notify_observers();
	// Always serialize on delete
	serialize();
	return ESP_OK;
}

// -----------------
// JSON
// -----------------

esp_err_t ConfigManager::serialize() const {
	std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_CreateObject(), cJSON_Delete);

	if (root == nullptr) {
		return ESP_FAIL;
	}

	cJSON_AddStringToObject(root.get(), "current_effect", config.current_effect.string().c_str());
	cJSON_AddStringToObject(root.get(), "effects_path", config.effects_path.string().c_str());
	cJSON_AddStringToObject(root.get(), "active_from", config.active_from.c_str());
	cJSON_AddStringToObject(root.get(), "active_to", config.active_to.c_str());

	std::unique_ptr<char, decltype(&free)> json_string(cJSON_PrintUnformatted(root.get()), free);
	if (json_string == nullptr) {
		return ESP_FAIL;
	}

	return FileManager::save_file(kPath, json_string.get());
}

esp_err_t ConfigManager::deserialize() {
	auto opt_json = FileManager::read_file(kPath);
	esp_err_t err = ESP_OK;

	if (!opt_json) {
		// Read failed
		ESP_LOGW(kTag, "Read failed");
		return ESP_FAIL;
	}

	std::unique_ptr<cJSON, decltype(&cJSON_Delete)> root(cJSON_Parse(opt_json.value().c_str()), cJSON_Delete);
	if (root == nullptr) {
		return ESP_FAIL;
	}

	cJSON *item = cJSON_GetObjectItem(root.get(), "current_effect");
	if (cJSON_IsString(item) != 0 && (item->valuestring != nullptr)) {
		config.current_effect = std::filesystem::path(item->valuestring);
	} else {
		err = ESP_FAIL;
	}

	item = cJSON_GetObjectItem(root.get(), "effects_path");
	if (cJSON_IsString(item) != 0 && (item->valuestring != nullptr)) {
		config.effects_path = std::filesystem::path(item->valuestring);
	} else {
		err = ESP_FAIL;
	}

	item = cJSON_GetObjectItem(root.get(), "active_from");
	if (cJSON_IsString(item) != 0 && (item->valuestring != nullptr)) {
		config.active_from = std::string(item->valuestring);
	} else {
		err = ESP_FAIL;
	}

	item = cJSON_GetObjectItem(root.get(), "active_to");
	if (cJSON_IsString(item) != 0 && (item->valuestring != nullptr)) {
		config.active_to = std::string(item->valuestring);
	} else {
		err = ESP_FAIL;
	}

	notify_observers();
	return err;
}
