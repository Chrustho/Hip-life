#pragma once

#include <cstdint>
#include <string>

namespace hl {

// bit i set => reacts when a cell has exactly i live neighbours (i in 0..8)
struct Rule {
    uint32_t birth;
    uint32_t survive;

    bool operator==(const Rule& o) const { return birth == o.birth && survive == o.survive; }
    bool operator!=(const Rule& o) const { return !(*this == o); }
};

constexpr uint32_t rule_bit(int n) { return 1u << n; }

namespace rules {
constexpr Rule conway   { rule_bit(3),                                     rule_bit(2) | rule_bit(3) };
constexpr Rule highlife { rule_bit(3) | rule_bit(6),                       rule_bit(2) | rule_bit(3) };
constexpr Rule daynight { rule_bit(3) | rule_bit(6) | rule_bit(7) | rule_bit(8),
                          rule_bit(3) | rule_bit(4) | rule_bit(6) | rule_bit(7) | rule_bit(8) };
constexpr Rule seeds    { rule_bit(2),                                     0 };
}

inline std::string rule_to_string(const Rule& r) {
    std::string s = "B";
    for (int n = 0; n <= 8; ++n) if (r.birth & rule_bit(n)) s += char('0' + n);
    s += "/S";
    for (int n = 0; n <= 8; ++n) if (r.survive & rule_bit(n)) s += char('0' + n);
    return s;
}

// accepts "B3/S23" (and the "b3/s23" / "23/3" Golly variants would need more; only
// the B/S form is supported, which is what LifeWiki RLE emits).
inline bool rule_from_string(const std::string& s, Rule& out) {
    uint32_t* target = nullptr;
    Rule r{0, 0};
    for (char c : s) {
        if (c == 'B' || c == 'b') target = &r.birth;
        else if (c == 'S' || c == 's') target = &r.survive;
        else if (c == '/') target = nullptr;
        else if (c >= '0' && c <= '8') {
            if (!target) return false;
            *target |= rule_bit(c - '0');
        } else if (c != ' ') {
            return false;
        }
    }
    out = r;
    return true;
}

}
