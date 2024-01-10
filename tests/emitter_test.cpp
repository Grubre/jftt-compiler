#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "mw.hpp"
#include "tests_shared.hpp"
#include <array>

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
        TestParams{"/example7.imp", {0, 0, 0}, {31000, 40900, 2222010}},
        TestParams{"/example7.imp", {1, 0, 2}, {31001, 40900, 2222012}},
    };

    for (auto &[filename, inputs, outputs] : test_params) {
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

            auto program_state = run_machine(lines, inputs);

            CHECK(program_state.outputs.size() == outputs.size());
            for (auto i = 0; i < outputs.size(); i++)
                CHECK(program_state.outputs[i] == outputs[i]);
        }
    }
}
