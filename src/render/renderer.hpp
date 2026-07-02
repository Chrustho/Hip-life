#pragma once

#include <cstdint>
#include <memory>

namespace hl {

// decouples simulation state from how it reaches the screen. Path B uploads a
// host copy of the grid into a texture; a future zero-copy interop backend
// (Path A) would implement this same interface without touching sim or app.
class IGridRenderer {
public:
    virtual ~IGridRenderer() = default;

    virtual void resize(int w, int h) = 0;
    virtual void update(const uint8_t* cells, const uint32_t* ages, bool heatmap) = 0;
    virtual void draw(float cell_size) const = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

std::unique_ptr<IGridRenderer> make_raylib_renderer();

}
