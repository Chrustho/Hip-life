#pragma once

#include <deque>
#include <memory>

#include "raylib.h"

#include "render/renderer.hpp"
#include "sim/simulation.hpp"

namespace hl {

class App {
public:
    void init(int grid_w, int grid_h);
    void run();

    // actions invoked from the UI layer
    void ui_step();
    void ui_clear();
    void ui_randomize();
    void ui_apply_rule();
    void ui_set_boundary(Boundary b);

    // state shared with the UI layer (ui/panels.cpp)
    std::unique_ptr<ISimulation> sim;
    std::unique_ptr<IGridRenderer> renderer;

    int gw = 0;
    int gh = 0;
    float cell_size = 8.0f;
    Camera2D camera{};

    bool running = false;
    float steps_per_sec = 15.0f;

    int brush = 3;
    float density = 0.3f;

    int rule_index = 0;
    Rule custom_rule = rules::conway;
    Boundary boundary = Boundary::Toroidal;
    bool heatmap = false;

    int selected_pattern = -1;

    uint64_t population = 0;
    int cycle_period = 0;
    bool ui_capture = false;

private:
    void apply_rule();
    void step_once();
    void record_hash();
    void handle_input();
    void advance(double dt);
    void paint(bool alive);
    void stamp_pattern();
    bool cell_at_cursor(int& cx, int& cy) const;
    void center_view();

    double accumulator_ = 0.0;
    std::deque<uint64_t> hashes_;
};

const Rule* rule_presets();
int rule_preset_count();
const char* rule_preset_names();

}
