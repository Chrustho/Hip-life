#include "simulation.hpp"

#include <utility>

#include "grid.hpp"
#include "sim_core.hpp"

namespace hl {

namespace {

class CpuSimulation final : public ISimulation {
public:
    void resize(int width, int height) override {
        cur_.resize(width, height);
        nxt_.resize(width, height);
        w_ = width;
        h_ = height;
        gen_ = 0;
    }

    void clear() override {
        cur_.clear();
        gen_ = 0;
    }

    void randomize(float density, uint32_t seed) override {
        cur_.randomize(density, seed);
        gen_ = 0;
    }

    void setCell(int x, int y, bool alive) override { cur_.set(x, y, alive); }

    void step() override {
        const uint8_t* c = cur_.cells.data();
        const uint32_t* a = cur_.age.data();
        uint8_t* nc = nxt_.cells.data();
        uint32_t* na = nxt_.age.data();
        for (int y = 0; y < h_; ++y) {
            for (int x = 0; x < w_; ++x) {
                int i = y * w_ + x;
                uint8_t alive = next_state(c, w_, h_, x, y, rule_, boundary_);
                nc[i] = alive;
                na[i] = next_age(a[i], c[i], alive);
            }
        }
        std::swap(cur_, nxt_);
        ++gen_;
    }

    const uint8_t* hostCells() override { return cur_.cells.data(); }
    const uint32_t* hostAges() override { return cur_.age.data(); }
    uint64_t population() override { return cur_.population(); }
    const char* backend() const override { return "cpu"; }

private:
    Grid cur_;
    Grid nxt_;
};

}

std::unique_ptr<ISimulation> make_cpu_simulation() {
    return std::make_unique<CpuSimulation>();
}

}
