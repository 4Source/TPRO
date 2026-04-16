#pragma once
#include "effect.hpp"
#include <memory>

class EffectFactory {
  public:
	static std::unique_ptr<Effect> generate_from_json(const char *path);

  private:
	static inline std::array<char, 32> type_buffer;
};