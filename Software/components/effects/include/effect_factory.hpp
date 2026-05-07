#pragma once
#include "effect.hpp"
#include <array>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

class Effect;
using EffectCreator = std::function<std::shared_ptr<Effect>()>;

class EffectFactory {
  public:
	static std::shared_ptr<Effect> generate_from_json(std::string path);
	static esp_err_t writeDefaults(const std::string &directory);

  private:
	static constexpr const char *kTag = "factory";
	static const std::unordered_map<std::string, EffectCreator> registry;
	static inline std::array<char, 32> type_buffer;
};