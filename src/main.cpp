#include "raylib.h"

#include "app/app.hpp"

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "hip-life");
    SetTargetFPS(120);

    hl::App app;
    app.init(256, 160);
    app.run();

    CloseWindow();
    return 0;
}
