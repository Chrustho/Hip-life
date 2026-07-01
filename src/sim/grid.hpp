#pragma once

#include <algorithm>
#include <cstdint>
#include <random>
#include <vector>

namespace hl {

struct Grid {
    int w = 0;
    int h = 0;
    std::vector<uint8_t> cells;
    std::vector<uint32_t> age;

    void resize(int width, int height) {
        w = width;
        h = height;
        cells.assign(size_t(w) * h, 0);
        age.assign(size_t(w) * h, 0);
    }

    void clear() {
        std::fill(cells.begin(), cells.end(), uint8_t(0));
        std::fill(age.begin(), age.end(), uint32_t(0));
    }

    void randomize(float density, uint32_t seed) {
        std::mt19937 rng(seed);
        std::bernoulli_distribution alive(density);
        for (size_t i = 0; i < cells.size(); ++i) {
            uint8_t a = alive(rng);
            cells[i] = a;
            age[i] = a;
        }
    }

    void set(int x, int y, bool alive) {
        if (x < 0 || x >= w || y < 0 || y >= h) return;
        size_t i = size_t(y) * w + x;
        cells[i] = alive ? 1 : 0;
        age[i] = alive ? 1 : 0;
    }

    uint64_t population() const {
        uint64_t p = 0;
        for (uint8_t c : cells) p += c;
        return p;
    }
};

}
