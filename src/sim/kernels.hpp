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

__global__ void k_life_tiled(const uint8_t* cur, const uint32_t* age, uint8_t* nxt, uint32_t* nage,
                             int w, int h, Rule rule, Boundary boundary) {
    __shared__ uint8_t tile[kTile + 2][kTile + 2];
    int tx = threadIdx.x, ty = threadIdx.y;
    int gx = blockIdx.x * kTile + tx, gy = blockIdx.y * kTile + ty;

    tile[ty + 1][tx + 1] = sample(cur, w, h, gx, gy, boundary);
    if (tx == 0) tile[ty + 1][0] = sample(cur, w, h, gx - 1, gy, boundary);
    if (tx == kTile - 1) tile[ty + 1][kTile + 1] = sample(cur, w, h, gx + 1, gy, boundary);
    if (ty == 0) tile[0][tx + 1] = sample(cur, w, h, gx, gy - 1, boundary);
    if (ty == kTile - 1) tile[kTile + 1][tx + 1] = sample(cur, w, h, gx, gy + 1, boundary);
    if (tx == 0 && ty == 0) tile[0][0] = sample(cur, w, h, gx - 1, gy - 1, boundary);
    if (tx == kTile - 1 && ty == 0) tile[0][kTile + 1] = sample(cur, w, h, gx + 1, gy - 1, boundary);
    if (tx == 0 && ty == kTile - 1) tile[kTile + 1][0] = sample(cur, w, h, gx - 1, gy + 1, boundary);
    if (tx == kTile - 1 && ty == kTile - 1)
        tile[kTile + 1][kTile + 1] = sample(cur, w, h, gx + 1, gy + 1, boundary);
    __syncthreads();

    if (gx >= w || gy >= h) return;
    int n = 0;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if (dx || dy) n += tile[ty + 1 + dy][tx + 1 + dx];
    uint8_t self = tile[ty + 1][tx + 1];
    uint8_t alive = apply_rule(rule, self, n);
    int i = gy * w + gx;
    nxt[i] = alive;
    nage[i] = next_age(age[i], self, alive);
}

inline dim3 grid_for(int w, int h) {
    return dim3((w + kTile - 1) / kTile, (h + kTile - 1) / kTile);
}

void launch_life_naive(const uint8_t* cur, const uint32_t* age, uint8_t* nxt, uint32_t* nage, int w,
                       int h, Rule rule, Boundary boundary) {
    k_life_naive<<<grid_for(w, h), dim3(kTile, kTile)>>>(cur, age, nxt, nage, w, h, rule, boundary);
}

void launch_life_tiled(const uint8_t* cur, const uint32_t* age, uint8_t* nxt, uint32_t* nage, int w,
                       int h, Rule rule, Boundary boundary) {
    k_life_tiled<<<grid_for(w, h), dim3(kTile, kTile)>>>(cur, age, nxt, nage, w, h, rule, boundary);
}

}
}
