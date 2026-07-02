#include <cstdint>
#include <cstdio>
#include <vector>

#include "sim/simulation.hpp"

using namespace hl;

namespace {

int failures = 0;

std::vector<uint8_t> cells(ISimulation& s) {
    const uint8_t* c = s.hostCells();
    return std::vector<uint8_t>(c, c + size_t(s.width()) * s.height());
}

std::vector<uint32_t> ages(ISimulation& s) {
    const uint32_t* a = s.hostAges();
    return std::vector<uint32_t>(a, a + size_t(s.width()) * s.height());
}

void compare(Rule rule, Boundary boundary, uint32_t seed) {
    auto cpu = make_cpu_simulation();
    auto gpu = make_hip_simulation();
    const int W = 97, H = 61;
    for (ISimulation* s : {cpu.get(), gpu.get()}) {
        s->resize(W, H);
        s->setRule(rule);
        s->setBoundary(boundary);
        s->randomize(0.3f, seed);
    }
    for (int gen = 1; gen <= 50; ++gen) {
        cpu->step();
        gpu->step();
        if (cells(*cpu) != cells(*gpu)) {
            std::printf("FAIL: cells diverge at gen %d (rule %s, boundary %d)\n", gen,
                        rule_to_string(rule).c_str(), int(boundary));
            ++failures;
            return;
        }
        if (ages(*cpu) != ages(*gpu)) {
            std::printf("FAIL: ages diverge at gen %d\n", gen);
            ++failures;
            return;
        }
    }
}

}

int main() {
    if (!make_hip_simulation()) {
        std::printf("SKIP: no usable HIP device\n");
        return 0;
    }
    compare(rules::conway, Boundary::Toroidal, 1);
    compare(rules::conway, Boundary::Fixed, 2);
    compare(rules::highlife, Boundary::Toroidal, 3);
    compare(rules::seeds, Boundary::Toroidal, 4);
    if (failures == 0) std::printf("GPU matches CPU reference across all cases\n");
    return failures == 0 ? 0 : 1;
}
