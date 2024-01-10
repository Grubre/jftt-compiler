#pragma once

#include "instruction.hpp"
#include <array>
#include <cstdint>
#include <emitter.hpp>
#include <map>

struct ProgramState {
    std::array<long long, 8> r;
    std::map<long long, long long> pam;
    bool error;
};

class ReadHandler {
  public:
    virtual auto get_next_input() -> uint64_t = 0;
};

class ReadHandlerStdin : public ReadHandler {
  public:
    auto get_next_input() -> uint64_t override;
};

class ReadHandlerDeque : public ReadHandler {
  public:
    ReadHandlerDeque(std::deque<uint64_t> input_values)
        : input_values(std::move(input_values)) {}

    auto get_next_input() -> uint64_t override;

  private:
    std::deque<uint64_t> input_values;
};

class WriteHandler {
  public:
    virtual void handle_output(uint64_t output) = 0;
};

class WriteHandlerStdout : public WriteHandler {
  public:
    void handle_output(uint64_t output) override;
};

class WriteHandlerVector : public WriteHandler {
  public:
    void handle_output(uint64_t output) override;

    auto get_outputs() -> const std::vector<uint64_t> &;

  private:
    std::vector<uint64_t> outputs;
};

ProgramState run_machine(const std::vector<emitter::Line> &lines,
                         ReadHandler *read_handler,
                         WriteHandler *write_handler);
