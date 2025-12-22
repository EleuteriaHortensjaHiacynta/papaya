// papaya.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <raylib.h>
#include "Scenes/MainMenu.hpp"


int main() {
    std::cout << "Hello World!\n";

    Vector2 MousePos = { 0.0f, 0.0f };

    const int WindowHeight = 720;
    const int WindowWidth = 1280;
    char State = 0;
    bool ShouldQuit = false;

    InitWindow(WindowWidth, WindowHeight, "papaya");

    SetTargetFPS(60);

    // this loop only changes the state 
    // the functions called have their own while loops with drawing etc
    // because thats the way i found to easily declare things once and not in every iteration
    // and this also makes it so for example the mouse position is only checked when required and not always

    while (!WindowShouldClose() && !ShouldQuit) {
        switch (State) {
        case 0:
            sceneMainMenu(MousePos, WindowWidth, WindowHeight, ShouldQuit);
            break;
        default:
            sceneMainMenu(MousePos, WindowWidth, WindowHeight, ShouldQuit);
        }
    }

    CloseWindow();

    return 0;
}
