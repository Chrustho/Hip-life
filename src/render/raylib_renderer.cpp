#include "renderer.hpp"

#include <vector>

#include "raylib.h"

namespace hl {

namespace {

Color age_color(uint32_t age) {
    // young cells start near white and cool toward blue/violet as they persist.
    constexpr uint32_t cap = 48;
    float t = float(age > cap ? cap : age) / float(cap);
    unsigned char r = (unsigned char)(230 * (1.0f - t) + 60 * t);
    unsigned char g = (unsigned char)(240 * (1.0f - t) + 30 * t);
    unsigned char b = (unsigned char)(200 * (1.0f - t) + 220 * t);
    return Color{r, g, b, 255};
}

class RaylibRenderer final : public IGridRenderer {
public:
    ~RaylibRenderer() override {
        if (tex_.id) UnloadTexture(tex_);
    }

    void resize(int w, int h) override {
        w_ = w;
        h_ = h;
        pixels_.assign(size_t(w) * h, Color{0, 0, 0, 255});
        if (tex_.id) UnloadTexture(tex_);
        Image img = GenImageColor(w, h, BLACK);
        tex_ = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(tex_, TEXTURE_FILTER_POINT);
    }

    void update(const uint8_t* cells, const uint32_t* ages, bool heatmap) override {
        const Color dead{18, 18, 22, 255};
        const Color live{235, 235, 240, 255};
        size_t n = size_t(w_) * h_;
        for (size_t i = 0; i < n; ++i) {
            if (!cells[i]) {
                pixels_[i] = dead;
            } else {
                pixels_[i] = heatmap ? age_color(ages[i]) : live;
            }
        }
        UpdateTexture(tex_, pixels_.data());
    }

    void draw(float cell_size) const override {
        Rectangle src{0, 0, float(w_), float(h_)};
        Rectangle dst{0, 0, w_ * cell_size, h_ * cell_size};
        DrawTexturePro(tex_, src, dst, Vector2{0, 0}, 0.0f, WHITE);
    }

    int width() const override { return w_; }
    int height() const override { return h_; }

private:
    int w_ = 0;
    int h_ = 0;
    std::vector<Color> pixels_;
    Texture2D tex_{};
};

}

std::unique_ptr<IGridRenderer> make_raylib_renderer() {
    return std::make_unique<RaylibRenderer>();
}

}
