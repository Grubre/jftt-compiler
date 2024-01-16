#pragma once

#include "instruction.hpp"
#include "mw.hpp"
#include <array>
#include <cln/cln.h>
#include <cstdint>
#include <emitter.hpp>
#include <map>

ProgramState<cln::cl_I> run_machine(const std::vector<instruction::Line> &lines,
                                    ReadHandler *read_handler,
                                    WriteHandler<cln::cl_I> *write_handler);
