#include "app.hpp"

#include <cmath>
#include <cstdio>

#include "io/patterns.hpp"
#include "sim/sim_core.hpp"
#include "ui/panels.hpp"

namespace hl {

namespace {

const Rule kPresets[] = {rules::conway, rules::highlife, rules::daynight, rules::seeds};
const char kPresetNames[] = "Conway;HighLife;Day&Night;Seeds;Custom";
constexpr int kCustomIndex = 4;

}

const Rule* rule_presets() { return kPresets; }
int rule_preset_count() { return int(sizeof(kPresets) / sizeof(kPresets[0])); }
const char* rule_preset_names() { return kPresetNames; }

void App::init(int grid_w, int grid_h) {
    gw = grid_w;
    gh = grid_h;

    sim = make_hip_simulation();
    if (!sim) sim = make_cpu_simulation();
    sim->resize(gw, gh);
    sim->setBoundary(boundary);
    apply_rule();

    renderer = make_raylib_renderer();
    renderer->resize(gw, gh);

    camera.zoom = 1.0f;
    camera.rotation = 0.0f;
    center_view();
}

void App::center_view() {
    float world_w = gw * cell_size;
    float world_h = gh * cell_size;
    camera.offset = Vector2{GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    camera.target = Vector2{world_w / 2.0f, world_h / 2.0f};
}

void App::ui_step() { step_once(); }
void App::ui_clear() {
    sim->clear();
    hashes_.clear();
}
void App::ui_randomize() {
    sim->randomize(density, uint32_t(GetTime() * 1000));
    hashes_.clear();
}
void App::ui_apply_rule() { apply_rule(); }
void App::ui_set_boundary(Boundary b) {
    boundary = b;
    sim->setBoundary(b);
}

void App::apply_rule() {
    if (rule_index < kCustomIndex) {
        custom_rule = kPresets[rule_index];
        sim->setRule(kPresets[rule_index]);
    } else {
        sim->setRule(custom_rule);
    }
}

void App::step_once() {
    sim->step();
    record_hash();
}

void App::record_hash() {
    const uint8_t* c = sim->hostCells();
    uint64_t h = 1469598103934665603ULL;
    size_t n = size_t(gw) * gh;
    for (size_t i = 0; i < n; ++i) {
        h ^= c[i];
        h *= 1099511628211ULL;
    }
    cycle_period = 0;
    int dist = 1;
    for (uint64_t prev : hashes_) {
        if (prev == h) {
            cycle_period = dist;
            break;
        }
        ++dist;
    }
    hashes_.push_front(h);
    if (hashes_.size() > 64) hashes_.pop_back();
}

bool App::cell_at_cursor(int& cx, int& cy) const {
    Vector2 w = GetScreenToWorld2D(GetMousePosition(), camera);
    cx = int(std::floor(w.x / cell_size));
    cy = int(std::floor(w.y / cell_size));
    return cx >= 0 && cx < gw && cy >= 0 && cy < gh;
}

void App::paint(bool alive) {
    int cx, cy;
    if (!cell_at_cursor(cx, cy)) return;
    int r = brush / 2;
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
            sim->setCell(cx + dx, cy + dy, alive);
    hashes_.clear();
}

void App::stamp_pattern() {
    const auto& pats = builtin_patterns();
    if (selected_pattern < 0 || selected_pattern >= int(pats.size())) return;
    const Pattern& p = pats[selected_pattern].pattern;
    int cx, cy;
    if (!cell_at_cursor(cx, cy)) return;
    for (int y = 0; y < p.h; ++y)
        for (int x = 0; x < p.w; ++x)
            if (p.at(x, y)) sim->setCell(cx + x, cy + y, true);
    hashes_.clear();
}

void App::handle_input() {
    ui_capture = CheckCollisionPointRec(GetMousePosition(),
                                        Rectangle{0, 0, kPanelWidth, float(GetScreenHeight())});

    if (IsKeyPressed(KEY_SPACE)) running = !running;
    if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_RIGHT)) step_once();
    if (IsKeyPressed(KEY_C)) {
        sim->clear();
        hashes_.clear();
    }
    if (IsKeyPressed(KEY_R)) {
        sim->randomize(density, uint32_t(GetTime() * 1000));
        hashes_.clear();
    }
    if (IsKeyPressed(KEY_H)) heatmap = !heatmap;
    if (IsKeyPressed(KEY_B)) {
        boundary = boundary == Boundary::Toroidal ? Boundary::Fixed : Boundary::Toroidal;
        sim->setBoundary(boundary);
    }
    if (IsKeyPressed(KEY_F2)) {
        char name[64];
        std::snprintf(name, sizeof(name), "hip-life-gen%llu.png", (unsigned long long)sim->generation());
        TakeScreenshot(name);
    }
    if (IsKeyPressed(KEY_LEFT_BRACKET) && brush > 1) --brush;
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) ++brush;

    float pan = 400.0f * GetFrameTime() / camera.zoom;
    if (IsKeyDown(KEY_W)) camera.target.y -= pan;
    if (IsKeyDown(KEY_S)) camera.target.y += pan;
    if (IsKeyDown(KEY_A)) camera.target.x -= pan;
    if (IsKeyDown(KEY_D)) camera.target.x += pan;

    if (ui_capture) return;

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 world = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.offset = GetMousePosition();
        camera.target = world;
        camera.zoom *= (wheel > 0 ? 1.15f : 1.0f / 1.15f);
        if (camera.zoom < 0.05f) camera.zoom = 0.05f;
        if (camera.zoom > 40.0f) camera.zoom = 40.0f;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 d = GetMouseDelta();
        camera.target.x -= d.x / camera.zoom;
        camera.target.y -= d.y / camera.zoom;
    }

    if (selected_pattern >= 0) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) stamp_pattern();
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        paint(true);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) paint(false);
}

void App::advance(double dt) {
    if (!running) return;
    accumulator_ += dt;
    double interval = 1.0 / (steps_per_sec > 0.1f ? steps_per_sec : 0.1f);
    int budget = 0;
    while (accumulator_ >= interval && budget < 256) {
        step_once();
        accumulator_ -= interval;
        ++budget;
    }
}

void App::run() {
    while (!WindowShouldClose()) {
        handle_input();
        advance(GetFrameTime());

        renderer->update(sim->hostCells(), sim->hostAges(), heatmap);
        population = sim->population();

        BeginDrawing();
        ClearBackground(Color{12, 12, 15, 255});

        BeginMode2D(camera);
        renderer->draw(cell_size);
        DrawRectangleLinesEx(Rectangle{0, 0, gw * cell_size, gh * cell_size}, 1.0f / camera.zoom,
                             Color{60, 60, 70, 255});
        if (!ui_capture) {
            int cx, cy;
            if (cell_at_cursor(cx, cy)) {
                int r = brush / 2;
                Rectangle b{(cx - r) * cell_size, (cy - r) * cell_size, brush * cell_size,
                            brush * cell_size};
                DrawRectangleLinesEx(b, 1.0f / camera.zoom, Color{240, 220, 90, 200});
            }
        }
        EndMode2D();

        draw_ui(*this);
        EndDrawing();
    }
}

}
