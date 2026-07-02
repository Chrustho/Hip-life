#include <hip/hip_runtime.h>

#include <memory>
#include <utility>

#include "grid.hpp"
#include "hip_check.hpp"
#include "kernels.hpp"
#include "simulation.hpp"

namespace hl {

namespace {

class HipSimulation final : public ISimulation {
public:
    ~HipSimulation() override { free_device(); }

    void resize(int width, int height) override {
        free_device();
        host_.resize(width, height);
        w_ = width;
        h_ = height;
        gen_ = 0;
        size_t cells = size_t(w_) * h_;
        HIP_CHECK(hipMalloc(&dcur_, cells));
        HIP_CHECK(hipMalloc(&dnxt_, cells));
        HIP_CHECK(hipMalloc(&dage_, cells * sizeof(uint32_t)));
        HIP_CHECK(hipMalloc(&dnage_, cells * sizeof(uint32_t)));
        host_dirty_ = true;
        device_dirty_ = false;
    }

    void clear() override {
        host_.clear();
        gen_ = 0;
        host_dirty_ = true;
        device_dirty_ = false;
    }

    void randomize(float density, uint32_t seed) override {
        host_.randomize(density, seed);
        gen_ = 0;
        host_dirty_ = true;
        device_dirty_ = false;
    }

    void setCell(int x, int y, bool alive) override {
        sync_host();
        host_.set(x, y, alive);
        host_dirty_ = true;
    }

    void step() override {
        if (host_dirty_) {
            size_t cells = size_t(w_) * h_;
            HIP_CHECK(hipMemcpy(dcur_, host_.cells.data(), cells, hipMemcpyHostToDevice));
            HIP_CHECK(hipMemcpy(dage_, host_.age.data(), cells * sizeof(uint32_t), hipMemcpyHostToDevice));
            host_dirty_ = false;
        }
        launch_life_naive(dcur_, dage_, dnxt_, dnage_, w_, h_, rule_, boundary_);
        HIP_CHECK(hipGetLastError());
        std::swap(dcur_, dnxt_);
        std::swap(dage_, dnage_);
        ++gen_;
        device_dirty_ = true;
    }

    const uint8_t* hostCells() override {
        sync_host();
        return host_.cells.data();
    }

    const uint32_t* hostAges() override {
        sync_host();
        return host_.age.data();
    }

    uint64_t population() override {
        sync_host();
        return host_.population();
    }

    const char* backend() const override { return "hip"; }

private:
    void sync_host() {
        if (!device_dirty_) return;
        size_t cells = size_t(w_) * h_;
        HIP_CHECK(hipMemcpy(host_.cells.data(), dcur_, cells, hipMemcpyDeviceToHost));
        HIP_CHECK(hipMemcpy(host_.age.data(), dage_, cells * sizeof(uint32_t), hipMemcpyDeviceToHost));
        device_dirty_ = false;
    }

    void free_device() {
        if (dcur_) static_cast<void>(hipFree(dcur_));
        if (dnxt_) static_cast<void>(hipFree(dnxt_));
        if (dage_) static_cast<void>(hipFree(dage_));
        if (dnage_) static_cast<void>(hipFree(dnage_));
        dcur_ = dnxt_ = nullptr;
        dage_ = dnage_ = nullptr;
    }

    Grid host_;
    uint8_t* dcur_ = nullptr;
    uint8_t* dnxt_ = nullptr;
    uint32_t* dage_ = nullptr;
    uint32_t* dnage_ = nullptr;
    bool host_dirty_ = false;
    bool device_dirty_ = false;
};

}

std::unique_ptr<ISimulation> make_hip_simulation() {
    int count = 0;
    if (hipGetDeviceCount(&count) != hipSuccess || count == 0) return nullptr;
    return std::make_unique<HipSimulation>();
}

}
