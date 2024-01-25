#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "lexer.hpp"
#include "mw-cln.hpp"
#include "mw.hpp"
#include "parser.hpp"
#include "tests_shared.hpp"
#include <array>
#include <memory>

template <typename T> struct TestParams {
    std::string filename;
    std::deque<uint64_t> input_values;
    std::vector<T> expected_outputs;
};

TEST_CASE_TEMPLATE("Official tests", T, uint64_t, cln::cl_I) {
    std::array test_params = {
        TestParams<T>{"/example1.imp", {5, 5}, {5, 4, 5}},
        TestParams<T>{"/example2.imp", {0, 1}, {46368, 28657}},
        TestParams<T>{"/example3.imp", {1}, {121393}},
        TestParams<T>{"/example4.imp", {20, 9}, {167960}},
        TestParams<T>{"/example5.imp", {1234567890, 1234567890987654321, 987654321}, {674106858}},
        TestParams<T>{"/example6.imp", {20}, {2432902008176640000, 6765}},
        TestParams<T>{"/example7.imp", {0, 0, 0}, {31000, 40900, 2222010}},
        TestParams<T>{"/example7.imp", {1, 0, 2}, {31001, 40900, 2222012}},
        TestParams<T>{"/example8.imp",
                      {},
                      {
                          5,  2, 10, 4,  20, 8,  17, 16,         11, 9,  22, 18, 21, 13, 19, 3,
                          15, 6, 7,  12, 14, 1,  0,  1234567890, 0,  1,  2,  3,  4,  5,  6,  7,
                          8,  9, 10, 11, 12, 13, 14, 15,         16, 17, 18, 19, 20, 21, 22,
                      }},
        TestParams<T>{"/example9.imp", {20, 9}, {167960}},
        TestParams<T>{"/binary.imp", {5}, {1, 0, 1}},
        TestParams<T>{"/gcd.imp", {5, 25, 50, 100}, {5}},
        TestParams<T>{"/gcd.imp", {12, 18, 96, 36}, {6}},
    };

    for (auto &[filename, inputs, expected_outputs] : test_params) {
        SUBCASE(("Test file: " + filename).c_str()) {
            const auto filecontent = read_file(std::string(TESTS_DIR) + filename);

            REQUIRE(filecontent.has_value());

            auto lexer = Lexer(*filecontent);

            auto tokens = std::vector<Token>{};

            for (auto &token : lexer) {
                REQUIRE(token.has_value());
                tokens.push_back(*token);
            }

            auto parser = parser::Parser(tokens);

            auto program = parser.parse_program();

            REQUIRE(program.has_value());

            auto emitter = emitter::Emitter(std::move(*program));

            emitter.emit();

            const auto lines = emitter.get_lines();

            auto read_handler = std::make_unique<ReadHandlerDeque>(inputs);
            auto write_handler = std::make_unique<WriteHandlerVector<T>>();

            auto program_state = run_machine(lines, read_handler.get(), write_handler.get());

            CHECK(!program_state.error);

            const auto outputs = write_handler->get_outputs();

            CHECK(outputs.size() == expected_outputs.size());
            for (auto i = 0u; i < outputs.size(); i++)
                CHECK(outputs[i] == expected_outputs[i]);
        }
    }
}

TEST_CASE("Slowik tests") {
    std::array test_params = {
        TestParams<cln::cl_I>{"/slowik/test0.imp",
                              {},
                              {cln::cl_I("340282367713220089251654026161790386200"),
                               cln::cl_I("340282367713220089251654026161790386200")}},
        TestParams<cln::cl_I>{"/slowik/test2a.imp", {}, {25}},
        TestParams<cln::cl_I>{"/slowik/test2b.imp", {}, {25}},
        TestParams<cln::cl_I>{"/slowik/test2c.imp", {}, {25}},
        TestParams<cln::cl_I>{"/slowik/test2d.imp", {}, {25}},
    };

    for (auto &[filename, inputs, expected_outputs] : test_params) {
        SUBCASE(("Test file: " + std::string(TESTS_DIR) + filename).c_str()) {
            const auto filecontent = read_file(std::string(TESTS_DIR) + filename);

            REQUIRE(filecontent.has_value());

            auto lexer = Lexer(*filecontent);

            auto tokens = std::vector<Token>{};

            for (auto &token : lexer) {
                if (!token) {
                    display_error(token.error());
                }
                REQUIRE(token.has_value());
                tokens.push_back(*token);
            }

            auto parser = parser::Parser(tokens);

            auto program = parser.parse_program();

            REQUIRE(program.has_value());

            auto emitter = emitter::Emitter(std::move(*program));

            emitter.emit();

            const auto lines = emitter.get_lines();

            auto read_handler = std::make_unique<ReadHandlerDeque>(inputs);
            auto write_handler = std::make_unique<WriteHandlerVector<cln::cl_I>>();

            auto program_state = run_machine(lines, read_handler.get(), write_handler.get());

            CHECK(!program_state.error);

            const auto outputs = write_handler->get_outputs();

            CHECK(outputs.size() == expected_outputs.size());
            for (auto i = 0u; i < outputs.size(); i++)
                CHECK(outputs[i] == expected_outputs[i]);
        }
    }
}
