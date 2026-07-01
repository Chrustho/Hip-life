#pragma once

#include <cstdint>

#include "boundary.hpp"
#include "rule.hpp"

#ifdef __HIPCC__
#define HL_HD __host__ __device__
#else
#define HL_HD
#endif

namespace hl {

// modulo that stays non-negative for negative operands; the naive x % n would
// pull wrapped neighbours off a torus edge to the wrong row.
HL_HD inline int wrap(int v, int n) { return ((v % n) + n) % n; }

HL_HD inline uint8_t sample(const uint8_t* cells, int w, int h, int x, int y, Boundary b) {
    if (b == Boundary::Toroidal) {
        x = wrap(x, w);
        y = wrap(y, h);
    } else if (x < 0 || x >= w || y < 0 || y >= h) {
        return 0;
    }
    return cells[y * w + x];
}

HL_HD inline int count_neighbours(const uint8_t* cells, int w, int h, int x, int y, Boundary b) {
    int n = 0;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy) n += sample(cells, w, h, x + dx, y + dy, b);
    return n;
}

HL_HD inline uint8_t apply_rule(Rule r, uint8_t alive, int neighbours) {
    uint32_t bit = 1u << neighbours;
    return alive ? ((r.survive & bit) != 0) : ((r.birth & bit) != 0);
}

HL_HD inline uint8_t next_state(const uint8_t* cells, int w, int h, int x, int y, Rule r, Boundary b) {
    return apply_rule(r, cells[y * w + x], count_neighbours(cells, w, h, x, y, b));
}

HL_HD inline uint32_t next_age(uint32_t age, uint8_t was_alive, uint8_t now_alive) {
    if (!now_alive) return 0;
    return was_alive ? age + 1 : 1;
}

}
