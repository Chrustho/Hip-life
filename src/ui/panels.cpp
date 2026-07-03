#include "panels.hpp"

#include <vector>

#include "raylib.h"
#include "raygui.h"

#include "app/app.hpp"
#include "io/patterns.hpp"
#include "io/rle.hpp"

namespace hl {

namespace {

void draw_stats(const App& app) {
    const int w = 186;
    int x = GetScreenWidth() - w - 10;
    int y = 10;
    DrawRectangle(x, y, w, 128, Color{0, 0, 0, 170});
    DrawRectangleLines(x, y, w, 128, Color{80, 80, 90, 255});
    int tx = x + 10;
    int ty = y + 8;
    auto line = [&](const char* s) {
        DrawText(s, tx, ty, 12, RAYWHITE);
        ty += 18;
    };
    line(TextFormat("backend   %s", app.sim->backend()));
    line(TextFormat("gen       %llu", (unsigned long long)app.sim->generation()));
    line(TextFormat("pop       %llu", (unsigned long long)app.population));
    line(TextFormat("fps       %d", GetFPS()));
    line(TextFormat("steps/s   %.0f", app.steps_per_sec));
    if (app.cycle_period == 1)
        line("state     still");
    else if (app.cycle_period > 1)
        line(TextFormat("cycle     period %d", app.cycle_period));
    else
        line("state     evolving");
}

void save_grid(App& app) {
    Pattern p;
    p.w = app.gw;
    p.h = app.gh;
    p.has_rule = true;
    p.rule = app.sim->rule();
    const uint8_t* c = app.sim->hostCells();
    p.cells.assign(c, c + size_t(p.w) * p.h);
    std::string err;
    save_rle_file("hip-life.rle", p, "hip-life", err);
}

}

void draw_ui(App& app) {
    float pw = kPanelWidth;
    DrawRectangle(0, 0, int(pw), GetScreenHeight(), Color{22, 22, 28, 235});
    DrawRectangleLines(0, 0, int(pw), GetScreenHeight(), Color{60, 60, 70, 255});

    float x = 12, y = 12, w = pw - 24, gap = 6;
    auto row = [&](float h) {
        Rectangle r{x, y, w, h};
        y += h + gap;
        return r;
    };
    auto half = [&](float h, int side) {
        float hw = (w - gap) / 2;
        Rectangle r{x + side * (hw + gap), y, hw, h};
        if (side == 1) y += h + gap;
        return r;
    };

    DrawText("hip-life", int(x), int(y), 20, RAYWHITE);
    y += 30;

    if (GuiButton(row(26), app.running ? "Pause  [Space]" : "Play  [Space]")) app.running = !app.running;
    if (GuiButton(half(24, 0), "Step  [.]")) app.ui_step();
    if (GuiButton(half(24, 1), "Clear  [C]")) app.ui_clear();
    if (GuiButton(row(24), "Randomize  [R]")) app.ui_randomize();

    GuiSliderBar(row(18), "density", TextFormat("%.2f", app.density), &app.density, 0.0f, 1.0f);
    GuiSliderBar(row(18), "steps/s", TextFormat("%.0f", app.steps_per_sec), &app.steps_per_sec, 1.0f, 120.0f);
    float brushf = float(app.brush);
    GuiSliderBar(row(18), "brush", TextFormat("%d", app.brush), &brushf, 1.0f, 25.0f);
    app.brush = int(brushf);

    GuiLabel(row(16), "Rule");
    int prev_rule = app.rule_index;
    GuiComboBox(row(24), rule_preset_names(), &app.rule_index);
    if (app.rule_index != prev_rule) app.ui_apply_rule();

    if (app.rule_index == rule_preset_count()) {
        float cw = (w - 8 * 2) / 9;
        GuiLabel(Rectangle{x, y, 14, 18}, "B");
        uint32_t birth = 0;
        for (int n = 0; n < 9; ++n) {
            bool b = (app.custom_rule.birth >> n) & 1u;
            GuiToggle(Rectangle{x + 16 + n * (cw + 0.9f), y, cw, 18}, TextFormat("%d", n), &b);
            if (b) birth |= rule_bit(n);
        }
        y += 24;
        GuiLabel(Rectangle{x, y, 14, 18}, "S");
        uint32_t survive = 0;
        for (int n = 0; n < 9; ++n) {
            bool b = (app.custom_rule.survive >> n) & 1u;
            GuiToggle(Rectangle{x + 16 + n * (cw + 0.9f), y, cw, 18}, TextFormat("%d", n), &b);
            if (b) survive |= rule_bit(n);
        }
        y += 24 + gap;
        if (birth != app.custom_rule.birth || survive != app.custom_rule.survive) {
            app.custom_rule = Rule{birth, survive};
            app.ui_apply_rule();
        }
    }

    GuiLabel(row(16), "Boundary");
    if (GuiButton(half(24, 0), app.boundary == Boundary::Toroidal ? "> Toroidal" : "Toroidal"))
        app.ui_set_boundary(Boundary::Toroidal);
    if (GuiButton(half(24, 1), app.boundary == Boundary::Fixed ? "> Fixed" : "Fixed"))
        app.ui_set_boundary(Boundary::Fixed);

    GuiCheckBox(row(20), "Heatmap  [H]", &app.heatmap);

    GuiLabel(row(16), "Patterns  (click to place)");
    if (GuiButton(row(22), app.selected_pattern < 0 ? "> Paint" : "Paint")) app.selected_pattern = -1;
    const auto& pats = builtin_patterns();
    for (int i = 0; i < int(pats.size()); ++i) {
        bool sel = app.selected_pattern == i;
        if (GuiButton(row(22), sel ? TextFormat("> %s", pats[i].name.c_str()) : pats[i].name.c_str()))
            app.selected_pattern = i;
    }

    if (GuiButton(row(24), "Save grid .rle")) save_grid(app);

    GuiLabel(Rectangle{x, GetScreenHeight() - 26.0f, w, 18}, "pan WASD/MMB  zoom wheel  shot F2");

    draw_stats(app);
}

}
