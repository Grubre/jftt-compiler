#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mw.hpp"
#include "tests_shared.hpp"
#include <array>
#include <memory>

struct TestParams {
    std::string filename;
    std::deque<uint64_t> input_values;
    std::vector<uint64_t> expected_outputs;
};

TEST_CASE("Official tests") {
    std::array test_params = {
        TestParams{"/example1.imp", {5, 5}, {5, 4, 5}},
        TestParams{"/example2.imp", {0, 1}, {46368, 28657}},
        TestParams{"/example3.imp", {1}, {121393}},
        TestParams{"/example4.imp", {20, 9}, {167960}},
        TestParams{"/example5.imp",
                   {1234567890, 1234567890987654321, 987654321},
                   {674106858}},
        TestParams{"/example6.imp", {20}, {2432902008176640000, 6765}},
        TestParams{"/example7.imp", {0, 0, 0}, {31000, 40900, 2222010}},
        TestParams{"/example7.imp", {1, 0, 2}, {31001, 40900, 2222012}},
        TestParams{"/example8.imp",
                   {},
                   {
                       5,  2,  10, 4,  20, 8,  17, 16, 11, 9,  22, 18,
                       21, 13, 19, 3,  15, 6,  7,  12, 14, 1,  0,  1234567890,
                       0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                       12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
                   }},
        TestParams{"/example9.imp", {20, 9}, {167960}},
        TestParams{"/binary.imp", {5}, {1, 0, 1}},
        TestParams{"/gcd.imp", {5, 25, 50, 100}, {5}},
        TestParams{"/gcd.imp", {12, 18, 96, 36}, {6}},
    };

    for (auto &[filename, inputs, expected_outputs] : test_params) {
        SUBCASE(("Test file: " + filename).c_str()) {
            const auto filecontent =
                read_file(std::string(TESTS_DIR) + filename);

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
            auto write_handler = std::make_unique<WriteHandlerVector>();

            auto program_state =
                run_machine(lines, read_handler.get(), write_handler.get());

            CHECK(!program_state.error);

            const auto outputs = write_handler->get_outputs();

            CHECK(outputs.size() == expected_outputs.size());
            for (auto i = 0; i < outputs.size(); i++)
                CHECK(outputs[i] == expected_outputs[i]);
        }
    }
}
