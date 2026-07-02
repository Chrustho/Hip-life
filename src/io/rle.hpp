#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "sim/rule.hpp"

namespace hl {

struct Pattern {
    int w = 0;
    int h = 0;
    bool has_rule = false;
    Rule rule{0, 0};
    std::vector<uint8_t> cells;

    uint8_t at(int x, int y) const { return cells[size_t(y) * w + x]; }
};

std::optional<Pattern> parse_rle(const std::string& text, std::string& err);
std::string write_rle(const Pattern& p, const std::string& name = "");

std::optional<Pattern> load_rle_file(const std::string& path, std::string& err);
bool save_rle_file(const std::string& path, const Pattern& p, const std::string& name, std::string& err);

}
