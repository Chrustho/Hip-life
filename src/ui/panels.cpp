#include "panels.hpp"

#include <vector>

#include "raylib.h"
#include "raygui.h"

#include "app/app.hpp"
#include "io/patterns.hpp"
#include "io/rle.hpp"

namespace hl {

namespace {

const Color kBg{14, 13, 11, 244};
const Color kAmber{255, 176, 0, 255};
const Color kAmberHi{255, 201, 77, 255};
const Color kText{232, 217, 176, 255};
const Color kMuted{138, 124, 92, 255};
const Color kHair{58, 50, 36, 255};

float g_ts = 20.0f;

void gtext(const char* s, float x, float y, Color c) {
    DrawTextEx(GuiGetFont(), s, Vector2{x, y}, g_ts, 1.0f, c);
}

float gwidth(const char* s) { return MeasureTextEx(GuiGetFont(), s, g_ts, 1.0f).x; }

void draw_stats(const App& app) {
    const int w = 236;
    const int h = 156;
    int x = GetScreenWidth() - w - 16;
    int y = 16;
    DrawRectangle(x, y, w, h, Color{14, 13, 11, 235});
    DrawRectangleLines(x, y, w, h, Color{122, 90, 18, 255});
    DrawLine(x, y + 28, x + w, y + 28, kHair);
    gtext("readout", x + 12, y + 6, kMuted);

    int tx = x + 12;
    int vx = x + 110;
    int ty = y + 38;
    auto line = [&](const char* label, const char* value, Color vc) {
        gtext(label, float(tx), float(ty), kMuted);
        gtext(value, float(vx), float(ty), vc);
        ty += 20;
    };
    line("backend", app.sim->backend(), app.sim->backend()[0] == 'h' ? kAmberHi : kText);
    line("gen", TextFormat("%llu", (unsigned long long)app.sim->generation()), kText);
    line("pop", TextFormat("%llu", (unsigned long long)app.population), kText);
    line("fps", TextFormat("%d", GetFPS()), kText);
    line("steps/s", TextFormat("%.0f", app.steps_per_sec), kText);
    if (app.cycle_period == 1)
        line("state", "still", kAmberHi);
    else if (app.cycle_period > 1)
        line("state", TextFormat("cycle p%d", app.cycle_period), kAmberHi);
    else
        line("state", "evolving", kMuted);
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

void apply_theme() {
    GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, ColorToInt(kBg));
    GuiSetStyle(DEFAULT, LINE_COLOR, ColorToInt(kHair));

    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, ColorToInt(Color{122, 90, 18, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, ColorToInt(Color{22, 20, 15, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(kText));

    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, ColorToInt(kAmber));
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, ColorToInt(Color{34, 29, 17, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, ColorToInt(kAmberHi));

    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, ColorToInt(kAmberHi));
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, ColorToInt(Color{52, 43, 19, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, ColorToInt(Color{255, 216, 115, 255}));

    GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, ColorToInt(Color{50, 46, 38, 255}));
    GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, ColorToInt(Color{18, 17, 14, 255}));
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, ColorToInt(kMuted));
}

void draw_ui(App& app) {
    const float pw = kPanelWidth;
    const int sh = GetScreenHeight();
    DrawRectangle(0, 0, int(pw), sh, kBg);
    DrawLine(int(pw) - 1, 0, int(pw) - 1, sh, Color{122, 90, 18, 200});

    const float x = 16;
    const float w = pw - 32;
    const float gap = 8;
    float y = 16;

    auto section = [&](const char* name) {
        y += 10;
        gtext(name, x, y, kMuted);
        float tw = gwidth(name);
        DrawLine(int(x + tw + 10), int(y + g_ts * 0.5f), int(x + w), int(y + g_ts * 0.5f), kHair);
        y += g_ts + 8;
    };
    auto slider = [&](const char* name, const char* val, float* v, float lo, float hi) {
        gtext(name, x, y, kText);
        gtext(val, x + w - gwidth(val), y, kAmberHi);
        y += g_ts + 4;
        GuiSliderBar(Rectangle{x, y, w, 14}, nullptr, nullptr, v, lo, hi);
        y += 14 + gap;
    };

    DrawTextEx(GuiGetFont(), "hip-life", Vector2{x, y}, 30, 2, kAmber);
    y += 40;
    DrawLine(int(x), int(y), int(x + w), int(y), kHair);
    y += 8;

    if (GuiButton(Rectangle{x, y, w, 30}, app.running ? "# PAUSE   [space]" : "> RUN   [space]"))
        app.running = !app.running;
    y += 30 + gap;
    float hw = (w - gap) / 2;
    if (GuiButton(Rectangle{x, y, hw, 28}, "STEP  [.]")) app.ui_step();
    if (GuiButton(Rectangle{x + hw + gap, y, hw, 28}, "CLEAR  [C]")) app.ui_clear();
    y += 28 + gap;
    if (GuiButton(Rectangle{x, y, w, 28}, "RANDOMIZE  [R]")) app.ui_randomize();
    y += 28 + gap;

    section("SIMULATION");
    slider("density", TextFormat("%.2f", app.density), &app.density, 0.0f, 1.0f);
    slider("steps/s", TextFormat("%.0f", app.steps_per_sec), &app.steps_per_sec, 1.0f, 120.0f);
    float brushf = float(app.brush);
    slider("brush", TextFormat("%d", app.brush), &brushf, 1.0f, 25.0f);
    app.brush = int(brushf);

    section("RULE");
    int prev_rule = app.rule_index;
    GuiComboBox(Rectangle{x, y, w, 28}, rule_preset_names(), &app.rule_index);
    y += 28 + gap;
    if (app.rule_index != prev_rule) app.ui_apply_rule();

    if (app.rule_index == rule_preset_count()) {
        float cw = (w - 8) / 9;
        gtext("B", x, y + 2, kMuted);
        uint32_t birth = 0;
        for (int n = 0; n < 9; ++n) {
            bool b = (app.custom_rule.birth >> n) & 1u;
            GuiToggle(Rectangle{x + 20 + n * cw, y, cw - 2, 22}, TextFormat("%d", n), &b);
            if (b) birth |= rule_bit(n);
        }
        y += 26;
        gtext("S", x, y + 2, kMuted);
        uint32_t survive = 0;
        for (int n = 0; n < 9; ++n) {
            bool b = (app.custom_rule.survive >> n) & 1u;
            GuiToggle(Rectangle{x + 20 + n * cw, y, cw - 2, 22}, TextFormat("%d", n), &b);
            if (b) survive |= rule_bit(n);
        }
        y += 26 + gap;
        if (birth != app.custom_rule.birth || survive != app.custom_rule.survive) {
            app.custom_rule = Rule{birth, survive};
            app.ui_apply_rule();
        }
    }

    section("VIEW");
    bool tor = app.boundary == Boundary::Toroidal;
    if (GuiButton(Rectangle{x, y, hw, 28}, tor ? "> toroidal" : "toroidal"))
        app.ui_set_boundary(Boundary::Toroidal);
    if (GuiButton(Rectangle{x + hw + gap, y, hw, 28}, !tor ? "> fixed" : "fixed"))
        app.ui_set_boundary(Boundary::Fixed);
    y += 28 + gap;
    GuiCheckBox(Rectangle{x, y, 22, 22}, "  heatmap  [H]", &app.heatmap);
    y += 22 + gap;

    section("PATTERNS");
    if (GuiButton(Rectangle{x, y, w, 26}, app.selected_pattern < 0 ? "> paint" : "paint"))
        app.selected_pattern = -1;
    y += 26 + gap;
    const auto& pats = builtin_patterns();
    for (int i = 0; i < int(pats.size()); ++i) {
        bool sel = app.selected_pattern == i;
        if (GuiButton(Rectangle{x, y, w, 26},
                      sel ? TextFormat("> %s", pats[i].name.c_str()) : pats[i].name.c_str()))
            app.selected_pattern = i;
        y += 26 + gap;
    }

    y += 4;
    if (GuiButton(Rectangle{x, y, w, 28}, "save grid  .rle")) save_grid(app);

    gtext("pan WASD/MMB   zoom wheel   shot F2", x, sh - 26.0f, kMuted);

    draw_stats(app);
}

}
