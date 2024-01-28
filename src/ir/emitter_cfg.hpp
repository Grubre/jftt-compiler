#pragma once

#include "cfg_builder.hpp"
#include <cstdint>
#include <string>

namespace lir {
class EmitterCfg {
  public:
    EmitterCfg() = default;
    EmitterCfg(Cfg &&cfg) : cfg(std::move(cfg)) {}

  private:
    Cfg cfg;
};
} // namespace lir
