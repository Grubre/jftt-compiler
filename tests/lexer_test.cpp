#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "lexer.hpp"
#include "tests_shared.hpp"
#include <array>

TEST_CASE("Provided examples tests") {
    std::string filename;
    constexpr std::array filenames = {
        "example1.imp", "example2.imp", "example3.imp",
        "example4.imp", "example5.imp", "example6.imp",
        "example7.imp", "example8.imp", "example9.imp",
    };

    DOCTEST_VALUE_PARAMETERIZED_DATA(filename, filenames);

    auto filepath = std::string(TESTS_DIR) + "/" + filename;

    auto filecontent = read_file(filepath);

    if (!filecontent) {
        FAIL("File " << std::quoted(filepath) << " not found.");
    }

    CHECK(filecontent->size() > 0);

    auto lexer = Lexer(*filecontent);

    for (auto &token : lexer) {
        CHECK(token.has_value());
        if (!token) {
            display_error(token.error());
        }
    }
}
