#pragma once
#include <string>
#include <vector>

class ConfigObserver {
  public:
	ConfigObserver() = default;
	virtual ~ConfigObserver() = default;

	ConfigObserver(const ConfigObserver &) = delete;
	ConfigObserver &operator=(const ConfigObserver &) = delete;
	ConfigObserver(ConfigObserver &&) = delete;
	ConfigObserver &operator=(ConfigObserver &&) = delete;

	virtual void update(const std::string &key) = 0;
	virtual void update(const std::vector<std::string> &keys) = 0;
};