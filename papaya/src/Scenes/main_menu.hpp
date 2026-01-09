#pragma once
#include <raylib.h>
#include "GUI/GUI_raylib.hpp"
#include "Scenes/scene_functions.hpp"

inline void sceneMainMenu(int WindowWidth, int WindowHeight, bool& shouldQuit, int& state) {
	bool shouldLeave = false;
	Texture2D starBackground = LoadTexture("assets/backgrounds/star_background.png");




	while (!WindowShouldClose() && !shouldQuit && (state == 0)) {
		BeginDrawing();
		ClearBackground(GREEN);
		DrawTexturePro(starBackground, Rectangle{ 0, 0, (float) starBackground.width, (float)starBackground.height }, Rectangle{ 0, 0, (float)WindowWidth, (float)WindowHeight }, { 0, 0 }, 0.0f, WHITE);


		EndDrawing();
	}
}