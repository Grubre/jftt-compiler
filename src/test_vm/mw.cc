/*
 * Kod interpretera maszyny rejestrowej do projektu z JFTT2023
 *
 * Autor: Maciek Gębala
 * http://ki.pwr.edu.pl/gebala/
 * 2023-11-15
 * (wersja long long)
 */
#include <iostream>
#include <locale>

#include <map>
#include <utility>
#include <vector>

#include <cstdlib> // rand()
#include <ctime>

#include "emitter.hpp"
#include "mw.hpp"
#include <stack>

ProgramState run_machine(const std::vector<emitter::Line> &lines,
                         std::deque<uint64_t> input_values) {
    std::map<long long, long long> pam;

    std::vector<uint64_t> outputs;

    std::array<long long, 8> r;
    long long tmp;
    long long lr;

    long long t, io;

    lr = 0;
    srand(time(NULL));
    for (int i = 0; i < 8; i++)
        r[i] = rand();
    t = 0;
    io = 0;

    while (
        !std::holds_alternative<emitter::Halt>(lines[lr].instruction)) // HALT
    {
        // std::cout << to_string(lines[lr].instruction) << std::endl;
        std::visit(overloaded{[&](const emitter::Read &) {
                                  std::cout << "popping "
                                            << input_values.front()
                                            << std::endl;
                                  r[0] = input_values.front();
                                  input_values.pop_front();
                                  io += 100;
                                  lr++;
                              },
                              [&](const emitter::Write &) {
                                  outputs.push_back(r[0]);
                                  io += 100;
                                  lr++;
                              },
                              [&](const emitter::Load &load) {
                                  r[0] = pam[r[(int)load.address]];
                                  t += 50;
                                  lr++;
                              },
                              [&](const emitter::Store &store) {
                                  pam[r[(int)store.address]] = r[0];
                                  t += 50;
                                  lr++;
                              },
                              [&](const emitter::Add &add) {
                                  r[0] += r[(int)add.address];
                                  t += 5;
                                  lr++;
                              },
                              [&](const emitter::Sub &sub) {
                                  r[0] -= r[0] >= r[(int)sub.address]
                                              ? r[(int)sub.address]
                                              : r[0];
                                  t += 5;
                                  lr++;
                              },
                              [&](const emitter::Get &get) {
                                  r[0] = r[(int)get.address];
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Put &put) {
                                  r[(int)put.address] = r[0];
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Rst &rst) {
                                  r[(int)rst.address] = 0;
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Inc &inc) {
                                  r[(int)inc.address]++;
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Dec &dec) {
                                  if (r[(int)dec.address] > 0)
                                      r[(int)dec.address]--;
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Shl &shl) {
                                  r[(int)shl.address] <<= 1;
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Shr &shr) {
                                  r[(int)shr.address] >>= 1;
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Jump &jump) {
                                  lr = jump.line;
                                  t += 1;
                              },
                              [&](const emitter::Jpos &jpos) {
                                  if (r[0] > 0)
                                      lr = jpos.line;
                                  else
                                      lr++;
                                  t += 1;
                              },
                              [&](const emitter::Jzero &jzero) {
                                  if (r[0] == 0)
                                      lr = jzero.line;
                                  else
                                      lr++;
                                  t += 1;
                              },
                              [&](const emitter::Strk &strk) {
                                  r[(int)strk.reg] = lr;
                                  t += 1;
                                  lr++;
                              },
                              [&](const emitter::Jumpr &jumpr) {
                                  lr = r[(int)jumpr.reg];
                                  t += 1;
                              },
                              [&](const emitter::Halt &) {},
                              [&](const emitter::Comment &comment) {}},
                   lines[lr].instruction);

        if (lr < 0 || lr >= (int)lines.size()) {
            return ProgramState{outputs, r, pam, true};
            // cerr << cRed << "Błąd: Wywołanie nieistniejącej instrukcji nr "
            // << lr << "." << cReset << endl;
            // exit(-1);
        }

        // std::cout << "Registers:\n";
        // for (int i = 0; i < 8; i++)
        //     std::cout << "r" << i << " = " << r[i] << std::endl;
        // std::cout << "Memory:\n";
        // for (auto &p : pam)
        //     std::cout << "p[" << p.first << "] = " << p.second << std::endl;
    }

    return ProgramState{outputs, r, pam, false};
}
