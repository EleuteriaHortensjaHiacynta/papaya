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
#include "Saves/map.hpp"
#include "Saves/entities.hpp"

// int main() {
//     // Open file for read+write in binary mode (create/truncate) so saver can both write and later read for sorting
//     std::fstream f("Assets/entities.dat", std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

//     {
//         EntitySaver saver(f);
//         saver.addEntityS({ 100, 150, PLAYER, 5 });
//         saver.addEntityS({ 100, 150, ENEMY, 5 });
//         // let saver be destroyed at end of scope to flush/sort
//     }
//     f.close();

//     std::cout << "Entities saved to file.\n";
//     std::fstream f2("Assets/entities.dat", std::ios::in | std::ios::binary);
//     EntityLoader loader(f2);
//     auto entities = loader.getAll();
//     std::cout << "Entities loaded from file:\n";
//     std::cout << "Loaded " << entities.size() << " entities from file.\n";

//     for (const auto& entity : entities) {
//         std::cout << "Entity Type: " << entity.entityType << ", Position: (" << entity.x << ", " << entity.y << "), Health: " << static_cast<int>(entity.health) << "\n";
//     }
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
}*/

