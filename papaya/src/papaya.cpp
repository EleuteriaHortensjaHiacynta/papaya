// papaya.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <raylib.h>

#include "Scenes/MainMenu.h"
#include "Scenes/scene_test_room.h"
#include <cmath>

#include "Scenes/main_menu.hpp"
#include "level_editor/scene_level_editor.hpp"


#include "Saves/map.hpp"
#include <fstream>

// int main() {
//     std::fstream file("test.map", std::ios::out | std::ios::trunc);
//     auto map = MapSaver(file);
//     Block block1 = { 0, 160, 32, 4, 0, Layers::BACKGROUND, ExtraData::COLLIDABLE };
//     Block block2 = { 100, 110, 8, 1, 0, Layers::BACKGROUND, ExtraData::COLLIDABLE };
//     Block block3 = { 220, 70, 40, 255, 0, Layers::BACKGROUND, ExtraData::COLLIDABLE };
//     Block block4 = { 0, 0, 2, 20, 0, Layers::BACKGROUND, ExtraData::COLLIDABLE };
//     map.addBlock(block1);
//     map.addBlock(block2);
//     map.addBlock(block3);
//     map.addBlock(block4);

//     return 0;
// }

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
}

