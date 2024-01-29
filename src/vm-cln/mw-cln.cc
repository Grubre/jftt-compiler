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

#include "common.hpp"
#include "emitter.hpp"
#include "mw-cln.hpp"
#include "mw.hpp"
#include <iostream>
#include <stack>

ProgramState<cln::cl_I> run_machine(const std::vector<instruction::Line> &lines, ReadHandler *read_handler,
                                    WriteHandler<cln::cl_I> *write_handler) {
    std::map<long long, cln::cl_I> pam;

    std::vector<cln::cl_I> outputs;

    std::array<cln::cl_I, 8> r;
    // long long tmp;
    long long lr;

    long long t, io;

    lr = 0;
    srand((unsigned int)time(NULL));
    for (int i = 0; i < 8; i++)
        r[i] = rand();
    t = 0;
    io = 0;

    while (!std::holds_alternative<instruction::Halt>(lines[lr].instruction)) // HALT
    {
        std::visit(overloaded{[&](const instruction::Read &) {
                                  r[0] = read_handler->get_next_input();
                                  io += 100;
                                  lr++;
                              },
                              [&](const instruction::Write &) {
                                  write_handler->handle_output(r[0]);
                                  io += 100;
                                  lr++;
                              },
                              [&](const instruction::Load &load) {
                                  r[0] = pam[cln::cl_I_to_long(r[(int)load.address])];
                                  t += 50;
                                  lr++;
                              },
                              [&](const instruction::Store &store) {
                                  pam[cln::cl_I_to_long(r[(int)store.address])] = r[0];
                                  t += 50;
                                  lr++;
                              },
                              [&](const instruction::Add &add) {
                                  r[0] += r[(int)add.address];
                                  t += 5;
                                  lr++;
                              },
                              [&](const instruction::Sub &sub) {
                                  r[0] -= r[0] >= r[(int)sub.address] ? r[(int)sub.address] : r[0];
                                  t += 5;
                                  lr++;
                              },
                              [&](const instruction::Get &get) {
                                  r[0] = r[(int)get.address];
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Put &put) {
                                  r[(int)put.address] = r[0];
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Rst &rst) {
                                  r[(int)rst.address] = 0;
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Inc &inc) {
                                  r[(int)inc.address]++;
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Dec &dec) {
                                  if (r[(int)dec.address] > 0)
                                      r[(int)dec.address]--;
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Shl &shl) {
                                  r[(int)shl.address] <<= 1;
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Shr &shr) {
                                  r[(int)shr.address] >>= 1;
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Jump &jump) {
                                  lr = jump.line;
                                  t += 1;
                              },
                              [&](const instruction::Jpos &jpos) {
                                  if (r[0] > 0)
                                      lr = jpos.line;
                                  else
                                      lr++;
                                  t += 1;
                              },
                              [&](const instruction::Jzero &jzero) {
                                  if (r[0] == 0)
                                      lr = jzero.line;
                                  else
                                      lr++;
                                  t += 1;
                              },
                              [&](const instruction::Strk &strk) {
                                  r[(int)strk.reg] = lr;
                                  t += 1;
                                  lr++;
                              },
                              [&](const instruction::Jumpr &jumpr) {
                                  lr = cln::cl_I_to_long(r[(int)jumpr.reg]);
                                  t += 1;
                              },
                              [&](const instruction::Halt &) {},
                              [&](const instruction::Comment &) {}},
                   lines[lr].instruction);

        if (lr < 0 || lr >= (int)lines.size()) {
    return ProgramState<cln::cl_I>{.r = r, .pam = pam, .t =t, .io=io, .error=true};
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

    return ProgramState<cln::cl_I>{.r = r, .pam = pam, .t =t, .io=io, .error=false};
}
