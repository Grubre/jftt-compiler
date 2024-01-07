#pragma once
#include "doctest/doctest.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

inline auto read_file(const std::string &filename)
    -> std::optional<std::string> {
    auto file = std::ifstream(filename);

    if (!file) {
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

#define DOCTEST_VALUE_PARAMETERIZED_DATA(data, data_container)                 \
    static size_t _doctest_subcase_idx = 0;                                    \
    std::for_each(                                                             \
        data_container.begin(), data_container.end(), [&](const auto &in) {    \
            DOCTEST_SUBCASE((std::string(#data_container "[") +                \
                             std::to_string(_doctest_subcase_idx++) + "]")     \
                                .c_str()) {                                    \
                data = in;                                                     \
            }                                                                  \
        });                                                                    \
    _doctest_subcase_idx = 0

inline auto read_test_files() -> std::string {
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

    REQUIRE(filecontent->size() > 0);

    return *filecontent;
}
