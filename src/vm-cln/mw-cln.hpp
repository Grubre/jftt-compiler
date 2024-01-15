#pragma once

#include "instruction.hpp"
#include <array>
#include <cstdint>
#include <emitter.hpp>
#include <map>
#include <cln/cln.h>
#include "mw.hpp"

ProgramState<cln::cl_I> run_machine(const std::vector<emitter::Line> &lines,
                         ReadHandler *read_handler,
                         WriteHandler<cln::cl_I> *write_handler);
