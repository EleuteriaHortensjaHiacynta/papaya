#include <iostream>
#include <raylib.h>
#include "Scenes/main_menu.hpp"
#include "Scenes/scene_game.cpp"       // <--- DODAJ TO
#include "level_editor/scene_level_editor.hpp"

// main2() możesz usunąć lub zostawić do testów

int main() {
    std::cout << "Hello Papaya!\n";

    int windowWidth = 1600;
    int windowHeight = 900;

    InitWindow(windowWidth, windowHeight, "Papaya");
    SetTargetFPS(60);

    // Stan początkowy
    int state = 0;
    bool shouldQuit = false;

    while (!WindowShouldClose() && !shouldQuit) {
        switch (state) {
        case 0: // MENU
            sceneMainMenu(windowWidth, windowHeight, shouldQuit, state);
            break;

        case 1: // GAMEPLAY (To teraz działa!)
            sceneGame(windowWidth, windowHeight, shouldQuit, state);
            break;

        case 2: // EDITOR
            sceneLevelEditor(shouldQuit, state, windowHeight, windowWidth);
            break;

        default:
            state = 0;
            break;
        }
    }

    CloseWindow();
    return 0;
}