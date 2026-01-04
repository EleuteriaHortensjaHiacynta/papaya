// papaya.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <raylib.h>
#include "Scenes/MainMenu.h"
#include "Scenes/scene_test_room.h"

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
                { 0.0f, 0.0f, (float)gameWidth, (float)gameHeight }, // source
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

    Vector2 MousePos = { 0.0f, 0.0f };

    const int WindowHeight = 720;
    const int WindowWidth = 1280;
    char State = 0;

    InitWindow(WindowWidth, WindowHeight, "papaya");

    SetTargetFPS(60);

    // this loop only changes the state 
    // the functions called have their own while loops with drawing etc
    // because thats the way i found to easily declare things once and not in every iteration
    // and this also makes it so for example the mouse position is only checked when required and not always

    while (!WindowShouldClose()) {
        switch (State) {
        case 0:
            MainMenu(MousePos, WindowWidth, WindowHeight);
            break;
        default:
            MainMenu(MousePos, WindowWidth, WindowHeight);
        }
    }

    CloseWindow();

    return 0;
}
*/

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
