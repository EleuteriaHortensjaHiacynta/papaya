// papaya.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <raylib.h>
#include <cmath>
#include <fstream>
#include "Scenes/scene_test_room.h"

#include "Scenes/scene_test_room.h"
#include "Scenes/main_menu.hpp"
#include "level_editor/scene_level_editor.hpp"
#include "Saves/saves.hpp"

int main2() {
    Saves saves("assets/saves/main_save");
    saves.loadFromEditorDir("assets/editor/");

    for (const auto& entityS : saves.getEntities().getAll()) {
        std::cout << "Entity at (" << entityS.x << ", " << entityS.y << ") Type: " << static_cast<int>(entityS.entityType) << " Health: " << static_cast<int>(entityS.health) << std::endl;
    }

    for (const auto& block : saves.getMap().getAll()) {
        std::cout << "Block at (" << block.mPosition.x << ", " << block.mPosition.y << ") Size: (" << block.mSize.x << ", " << block.mSize.y << ")\n";
    }
}

int main() {

    const int screenWidth = 1280; //tymczasowo const
    const int screenHeigt = 720; //tymczasowo const

    const int gameWidth = 320;
    const int gameHeight = 180;

    InitWindow(screenWidth, screenHeigt, "Papaya - Test Room");

    // Tworzenie wirtualne ekranu
    RenderTexture2D target = LoadRenderTexture(gameWidth, gameHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); // �eby pixele by�y ostre
    
    SetTargetFPS(60);

    SceneTestRoom* testLevel = new SceneTestRoom();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // Logika
        testLevel->Update(dt);

        // Rysowanie na ma�ym ekranie
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);
            // Rysowania sceny(gracza+mapy)
            testLevel->Draw();
        EndTextureMode();

        // Rysowanie rozci�gni�tego ekranu
        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(target.texture,
                { 0.0f, 0.0f, (float)gameWidth, -(float)gameHeight }, // source
                { 0.0f, 0.0f, (float)screenWidth, (float)screenHeigt }, // dest
                { 0.0f, 0.0f }, // origin
                0.0f, // rotation
                WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    delete testLevel;
    CloseWindow();
    return 0;
}

/*
int main() {
    std::cout << "Hello World!\n";

    int windowHeight = 900;
    int windowWidth = 1600;
    int state = 0;
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
            break;
        case 2:
            sceneLevelEditor(shouldQuit, state, windowHeight, windowWidth);
            break;
        default:
            sceneMainMenu(windowWidth, windowHeight, shouldQuit, state);
            break;
        }
    }

    CloseWindow();

    return 0;
}*/

