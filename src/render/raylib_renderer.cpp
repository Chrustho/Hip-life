#include "renderer.hpp"

#include <vector>

#include "raylib.h"

namespace hl {

namespace {

Color lerp_color(Color a, Color b, float t) {
    return Color{(unsigned char)(a.r + (b.r - a.r) * t), (unsigned char)(a.g + (b.g - a.g) * t),
                 (unsigned char)(a.b + (b.b - a.b) * t), 255};
}

Color age_color(uint32_t age) {
    // thermal ramp: freshly born cells burn white/amber and cool to deep red.
    constexpr uint32_t cap = 64;
    const Color stops[] = {{255, 244, 214, 255}, {255, 176, 0, 255}, {255, 106, 0, 255},
                           {194, 24, 7, 255},    {96, 12, 12, 255}};
    float t = float(age > cap ? cap : age) / float(cap) * 4.0f;
    int i = int(t);
    if (i >= 4) return stops[4];
    return lerp_color(stops[i], stops[i + 1], t - i);
}

class RaylibRenderer final : public IGridRenderer {
public:
    ~RaylibRenderer() override {
        if (tex_.id) UnloadTexture(tex_);
    }

    void resize(int w, int h) override {
        w_ = w;
        h_ = h;
        pixels_.assign(size_t(w) * h, kDead);
        if (tex_.id) UnloadTexture(tex_);
        Image img = GenImageColor(w, h, kDead);
        tex_ = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(tex_, TEXTURE_FILTER_POINT);
    }

    void update(const uint8_t* cells, const uint32_t* ages, bool heatmap) override {
        size_t n = size_t(w_) * h_;
        for (size_t i = 0; i < n; ++i) {
            if (!cells[i])
                pixels_[i] = kDead;
            else
                pixels_[i] = heatmap ? age_color(ages[i]) : kLive;
        }
        UpdateTexture(tex_, pixels_.data());
    }

    void draw(float cell_size) const override {
        Rectangle src{0, 0, float(w_), float(h_)};
        Rectangle dst{0, 0, w_ * cell_size, h_ * cell_size};
        DrawTexturePro(tex_, src, dst, Vector2{0, 0}, 0.0f, WHITE);

        float spread = cell_size * 0.75f;
        Rectangle glow{-spread, -spread, dst.width + 2 * spread, dst.height + 2 * spread};
        BeginBlendMode(BLEND_ADDITIVE);
        SetTextureFilter(tex_, TEXTURE_FILTER_BILINEAR);
        DrawTexturePro(tex_, src, glow, Vector2{0, 0}, 0.0f, Color{255, 255, 255, 70});
        SetTextureFilter(tex_, TEXTURE_FILTER_POINT);
        EndBlendMode();
    }

    int width() const override { return w_; }
    int height() const override { return h_; }

private:
    static constexpr Color kDead{7, 7, 6, 255};
    static constexpr Color kLive{255, 190, 66, 255};

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
