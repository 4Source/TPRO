#pragma once
#include "effect.hpp"
#include <memory>

class EffectFactory {
  public:
	static std::shared_ptr<Effect> generate_from_json(std::string path);

  private:
	static inline std::array<char, 32> type_buffer;
};