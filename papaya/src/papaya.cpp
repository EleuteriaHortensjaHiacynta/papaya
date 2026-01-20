#include <iostream>
#include "raylib.h"

#include "Scenes/main_menu.hpp"
#include "level_editor/scene_level_editor.hpp"
#include "Core/GameWrapper.hpp"

int main() {
    std::cout << "Starting Papaya Engine...\n";

    const int WINDOW_W = 1600;
    const int WINDOW_H = 900;

    InitWindow(WINDOW_W, WINDOW_H, "Papaya");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    int state = 0;
    int previousState = 0;
    bool shouldQuit = false;

    GameWrapper game(shouldQuit, state);

    while (!WindowShouldClose() && !shouldQuit) {

        if (previousState == 0 && state == 1) {
            game.reloadSave();
        }
        previousState = state;

        switch (state) {
        case 0:
            sceneMainMenu(WINDOW_W, WINDOW_H, shouldQuit, state);
            break;

        case 1:
            game.runFrame(GetScreenWidth(), GetScreenHeight());
            break;

        case 2:
            sceneLevelEditor(shouldQuit, state, WINDOW_H, WINDOW_W);
            break;

        default:
            state = 0;
            break;
        }
    }

    CloseWindow();
    return 0;
}