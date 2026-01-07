// papaya.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <raylib.h>
#include "Scenes/main_menu.hpp"
#include "Scenes/Test.hpp"
#include "level_editor/scene_level_editor.hpp"


int main() {
    std::cout << "Hello World!\n";

    int windowHeight = 900;
    int windowWidth = 1600;
    int state = 2;
    bool shouldQuit = false;

    InitWindow(windowWidth, windowHeight, "papaya");

    SetTargetFPS(60);



    // this loop only changes the state 
    // the functions called have their own while loops with drawing etc
    // because thats the way i found to easily declare things once and not in every iteration
    // and this also makes it so for example the mouse position is only checked when required and not always

    while (!WindowShouldClose() && !shouldQuit) {
        std::cout << state << std::endl;
        switch (state) {
        case 0:
            sceneMainMenu(windowWidth, windowHeight, shouldQuit, state);
            break;
        case 1:
            sceneTest(shouldQuit, state);
            break;
        case 2:
            sceneLevelEditor(shouldQuit, state, windowHeight, windowWidth);
        default:
            sceneMainMenu(windowWidth, windowHeight, shouldQuit, state);
        }
    }

    CloseWindow();

    return 0;
}
