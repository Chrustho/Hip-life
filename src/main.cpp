#include "raylib.h"

int main() {
    InitWindow(1280, 800, "hip-life");
    SetTargetFPS(60);

    Image img = GenImageChecked(64, 64, 8, 8, Color{28, 28, 34, 255}, Color{40, 40, 48, 255});
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{12, 12, 15, 255});
        DrawTexturePro(tex, Rectangle{0, 0, 64, 64},
                       Rectangle{0, 0, float(GetScreenWidth()), float(GetScreenHeight())},
                       Vector2{0, 0}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadTexture(tex);
    CloseWindow();
    return 0;
}
