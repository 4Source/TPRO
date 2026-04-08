#pragma once
#include "./config_observer.hpp"
#include "./config_type.hpp"
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>


struct ConfigManager {
	ConfigType config;
	std::unordered_map<std::string, std::vector<ConfigObserver *>> observerlist;

	ConfigManager();

	void notify_observers(const std::string &key);

	bool add_observer(const std::string &key, ConfigObserver &observer);

	bool remove_observer(const std::string &key, const ConfigObserver &observer);

	//(GET)
	auto get_config(const std::string &key);

	//(PUT)
	void set_config(const std::string &key, const std::string &value);

	//(POST)
	void set_config(ConfigType new_config);

	//(DELETE)
	void set_to_default(const std::string &key);
};