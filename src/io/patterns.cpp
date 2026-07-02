#include "patterns.hpp"

namespace hl {

namespace {

struct Src {
    const char* name;
    const char* rle;
};

const Src kSources[] = {
    {"Glider", "x = 3, y = 3, rule = B3/S23\nbob$2bo$3o!"},
    {"LWSS", "x = 5, y = 4, rule = B3/S23\nbo2bo$o4b$o3bo$4o!"},
    {"Pulsar", "x = 13, y = 13, rule = B3/S23\n2b3o3b3o$$o4bobo4bo$o4bobo4bo$o4bobo4bo$2b3o3b3o$$2b3o3b3o$o4bo"
               "bo4bo$o4bobo4bo$o4bobo4bo$$2b3o3b3o!"},
    {"Gosper gun", "x = 36, y = 9, rule = B3/S23\n24bo$22bobo$12b2o6b2o12b2o$11bo3bo4b2o12b2o$2o8bo5bo3b2o$"
                   "2o8bo3bob2o4bobo$10bo5bo7bo$11bo3bo$12b2o!"},
};

std::vector<NamedPattern> build() {
    std::vector<NamedPattern> out;
    std::string err;
    for (const Src& s : kSources) {
        auto p = parse_rle(s.rle, err);
        if (p) out.push_back({s.name, *p});
    }
    return out;
}

}

const std::vector<NamedPattern>& builtin_patterns() {
    static const std::vector<NamedPattern> patterns = build();
    return patterns;
}

}
