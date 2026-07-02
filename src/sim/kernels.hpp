#pragma once

#include <hip/hip_runtime.h>

#include "sim_core.hpp"

namespace hl {
namespace {

constexpr int kTile = 16;

__global__ void k_life_naive(const uint8_t* cur, const uint32_t* age, uint8_t* nxt, uint32_t* nage,
                             int w, int h, Rule rule, Boundary boundary) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x >= w || y >= h) return;
    int i = y * w + x;
    uint8_t alive = next_state(cur, w, h, x, y, rule, boundary);
    nxt[i] = alive;
    nage[i] = next_age(age[i], cur[i], alive);
}

inline dim3 grid_for(int w, int h) {
    return dim3((w + kTile - 1) / kTile, (h + kTile - 1) / kTile);
}

void launch_life_naive(const uint8_t* cur, const uint32_t* age, uint8_t* nxt, uint32_t* nage, int w,
                       int h, Rule rule, Boundary boundary) {
    k_life_naive<<<grid_for(w, h), dim3(kTile, kTile)>>>(cur, age, nxt, nage, w, h, rule, boundary);
}

}
}
