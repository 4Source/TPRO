#pragma once
#include <string>
#include <vector>

class ConfigObserver {
  public:
	virtual ~ConfigObserver() = default;
	virtual void update(const std::string &key) = 0;
	virtual void update(const std::vector<std::string> &keys) = 0;
};