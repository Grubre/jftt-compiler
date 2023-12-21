#pragma once
#include "doctest/doctest.h"
#include <fstream>
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
