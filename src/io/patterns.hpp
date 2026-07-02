#pragma once

#include <string>
#include <vector>

#include "rle.hpp"

namespace hl {

struct NamedPattern {
    std::string name;
    Pattern pattern;
};

const std::vector<NamedPattern>& builtin_patterns();

}
