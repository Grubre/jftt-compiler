#pragma once

#include "instruction.hpp"
#include <array>
#include <cstdint>
#include <deque>
#include <iostream>
#include <map>
#include <vector>

template <typename T> struct ProgramState {
    std::array<T, 8> r;
    std::map<long long, T> pam;
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
    ReadHandlerDeque(std::deque<uint64_t> input_values) : input_values(std::move(input_values)) {}

    auto get_next_input() -> uint64_t override;

  private:
    std::deque<uint64_t> input_values;
};

template <typename T> class WriteHandler {
  public:
    virtual void handle_output(T output) = 0;
};

template <typename T> class WriteHandlerStdout : public WriteHandler<T> {
  public:
    void handle_output(T output) override { std::cout << "> " << output << std::endl; }
};

template <typename T> class WriteHandlerVector : public WriteHandler<T> {
  public:
    void handle_output(T output) override { outputs.push_back(output); }

    auto get_outputs() -> const std::vector<T> & { return outputs; }

  private:
    std::vector<T> outputs;
};

ProgramState<long long> run_machine(const std::vector<instruction::Line> &lines, ReadHandler *read_handler,
                                    WriteHandler<uint64_t> *write_handler);
