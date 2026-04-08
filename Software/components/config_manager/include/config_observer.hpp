#pragma once
#include <string>

class ConfigObserver {
  public:
	virtual ~ConfigObserver() = default;
	virtual void update(const std::string &key) = 0;
};