#include <cstdint>
#include <cstdio>
#include <vector>

#include "sim/sim_core.hpp"
#include "sim/simulation.hpp"

using namespace hl;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

std::vector<uint8_t> snapshot(ISimulation& s) {
    const uint8_t* c = s.hostCells();
    return std::vector<uint8_t>(c, c + size_t(s.width()) * s.height());
}

void place(ISimulation& s, int ox, int oy, std::initializer_list<std::pair<int, int>> pts) {
    for (auto& p : pts) s.setCell(ox + p.first, oy + p.second, true);
}

void test_block_static() {
    auto s = make_cpu_simulation();
    s->resize(10, 10);
    s->setBoundary(Boundary::Fixed);
    place(*s, 3, 3, {{0, 0}, {1, 0}, {0, 1}, {1, 1}});
    auto init = snapshot(*s);
    for (int i = 0; i < 8; ++i) s->step();
    check(snapshot(*s) == init, "block stays static");
    check(s->population() == 4, "block population is 4");
}

void test_blinker_period_2() {
    auto s = make_cpu_simulation();
    s->resize(10, 10);
    s->setBoundary(Boundary::Fixed);
    place(*s, 3, 5, {{0, 0}, {1, 0}, {2, 0}});
    auto init = snapshot(*s);
    s->step();
    check(snapshot(*s) != init, "blinker changes after 1 step");
    check(s->population() == 3, "blinker keeps 3 cells");
    s->step();
    check(snapshot(*s) == init, "blinker returns after 2 steps");
}

void test_glider_translation() {
    auto s = make_cpu_simulation();
    const int W = 24, H = 24;
    s->resize(W, H);
    s->setBoundary(Boundary::Toroidal);
    place(*s, 5, 5, {{1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2}});
    auto init = snapshot(*s);
    for (int i = 0; i < 4; ++i) s->step();
    const uint8_t* now = s->hostCells();
    bool shifted = true;
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            uint8_t expected = init[wrap(y - 1, H) * W + wrap(x - 1, W)];
            if (now[y * W + x] != expected) shifted = false;
        }
    check(shifted, "glider translates by (1,1) every 4 generations");
    check(s->population() == 5, "glider keeps 5 cells");
}

}

int main() {
    test_block_static();
    test_blinker_period_2();
    test_glider_translation();
    if (failures == 0) std::printf("all invariants passed\n");
    return failures == 0 ? 0 : 1;
}
