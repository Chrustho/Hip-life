#include <hip/hip_runtime.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <utility>

#include "grid.hpp"
#include "hip_check.hpp"
#include "kernels.hpp"
#include "simulation.hpp"

using namespace hl;

namespace {

using Launcher = void (*)(const uint8_t*, const uint32_t*, uint8_t*, uint32_t*, int, int, Rule, Boundary);

double gpu_cells_per_sec(Launcher launch, const Grid& seed, int iters, Rule rule, Boundary boundary) {
    size_t n = size_t(seed.w) * seed.h;
    uint8_t *dcur, *dnxt;
    uint32_t *dage, *dnage;
    HIP_CHECK(hipMalloc(&dcur, n));
    HIP_CHECK(hipMalloc(&dnxt, n));
    HIP_CHECK(hipMalloc(&dage, n * sizeof(uint32_t)));
    HIP_CHECK(hipMalloc(&dnage, n * sizeof(uint32_t)));
    HIP_CHECK(hipMemcpy(dcur, seed.cells.data(), n, hipMemcpyHostToDevice));
    HIP_CHECK(hipMemcpy(dage, seed.age.data(), n * sizeof(uint32_t), hipMemcpyHostToDevice));

    launch(dcur, dage, dnxt, dnage, seed.w, seed.h, rule, boundary);
    HIP_CHECK(hipDeviceSynchronize());

    hipEvent_t start, stop;
    HIP_CHECK(hipEventCreate(&start));
    HIP_CHECK(hipEventCreate(&stop));
    HIP_CHECK(hipEventRecord(start));
    for (int i = 0; i < iters; ++i) {
        launch(dcur, dage, dnxt, dnage, seed.w, seed.h, rule, boundary);
        std::swap(dcur, dnxt);
        std::swap(dage, dnage);
    }
    HIP_CHECK(hipEventRecord(stop));
    HIP_CHECK(hipEventSynchronize(stop));
    float ms = 0;
    HIP_CHECK(hipEventElapsedTime(&ms, start, stop));

    HIP_CHECK(hipEventDestroy(start));
    HIP_CHECK(hipEventDestroy(stop));
    HIP_CHECK(hipFree(dcur));
    HIP_CHECK(hipFree(dnxt));
    HIP_CHECK(hipFree(dage));
    HIP_CHECK(hipFree(dnage));
    return double(n) * iters / (ms / 1000.0);
}

double cpu_cells_per_sec(const Grid& seed, int iters, Rule rule, Boundary boundary) {
    auto sim = make_cpu_simulation();
    sim->resize(seed.w, seed.h);
    sim->setRule(rule);
    sim->setBoundary(boundary);
    for (int y = 0; y < seed.h; ++y)
        for (int x = 0; x < seed.w; ++x)
            sim->setCell(x, y, seed.cells[size_t(y) * seed.w + x]);

    auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) sim->step();
    auto t1 = std::chrono::steady_clock::now();
    double s = std::chrono::duration<double>(t1 - t0).count();
    return double(size_t(seed.w) * seed.h) * iters / s;
}

}

int main(int argc, char** argv) {
    int w = 1024, h = 1024, iters = 200;
    if (argc >= 3) {
        w = std::atoi(argv[1]);
        h = std::atoi(argv[2]);
    }
    if (argc >= 4) iters = std::atoi(argv[3]);

    Grid seed;
    seed.resize(w, h);
    seed.randomize(0.3f, 12345);
    Rule rule = rules::conway;
    Boundary boundary = Boundary::Toroidal;

    std::printf("grid %dx%d, %d iterations, rule %s\n", w, h, iters, rule_to_string(rule).c_str());
    double naive = gpu_cells_per_sec(launch_life_naive, seed, iters, rule, boundary);
    double tiled = gpu_cells_per_sec(launch_life_tiled, seed, iters, rule, boundary);
    double cpu = cpu_cells_per_sec(seed, iters < 40 ? iters : 40, rule, boundary);

    auto row = [](const char* name, double cps) {
        std::printf("  %-8s %8.2f Mcells/s\n", name, cps / 1e6);
    };
    row("cpu", cpu);
    row("naive", naive);
    row("tiled", tiled);
    std::printf("tiled/naive speedup: %.2fx\n", tiled / naive);
    return 0;
}
