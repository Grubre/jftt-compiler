#pragma once
#include "parser.hpp"
#include <algorithm>

class Emitter {
  public:
    Emitter() = delete;
    Emitter(parser::Program &&program) : program(std::move(program)){};

  private:
    parser::Program program;
};
