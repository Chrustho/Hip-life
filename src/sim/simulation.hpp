#pragma once

#include <cstdint>
#include <memory>

#include "boundary.hpp"
#include "rule.hpp"

namespace hl {

class ISimulation {
public:
    virtual ~ISimulation() = default;

    virtual void resize(int width, int height) = 0;
    virtual void clear() = 0;
    virtual void randomize(float density, uint32_t seed) = 0;
    virtual void setCell(int x, int y, bool alive) = 0;
    virtual void step() = 0;

    // host-visible snapshot of the current generation; a GPU backend copies
    // device -> host here (Path B). valid until the next step().
    virtual const uint8_t* hostCells() = 0;
    virtual const uint32_t* hostAges() = 0;
    virtual uint64_t population() = 0;

    virtual const char* backend() const = 0;

    void setRule(const Rule& r) { rule_ = r; }
    Rule rule() const { return rule_; }
    void setBoundary(Boundary b) { boundary_ = b; }
    Boundary boundary() const { return boundary_; }
    int width() const { return w_; }
    int height() const { return h_; }
    uint64_t generation() const { return gen_; }

protected:
    int w_ = 0;
    int h_ = 0;
    Rule rule_ = rules::conway;
    Boundary boundary_ = Boundary::Toroidal;
    uint64_t gen_ = 0;
};

std::unique_ptr<ISimulation> make_cpu_simulation();

// returns nullptr when no usable HIP device is present; callers fall back to CPU.
std::unique_ptr<ISimulation> make_hip_simulation();

}
