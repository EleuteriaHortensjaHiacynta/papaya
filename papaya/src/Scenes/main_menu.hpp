#pragma once
#include <raylib.h>
#include "GUI/GUI_raylib.hpp"
#include "Scenes/scene_functions.hpp"

inline void sceneMainMenu(int windowWidth, int windowHeight, bool& shouldQuit, int& state) {
	
	float time = 0;

	float mouseIdleTime = 0;

	Vector2 mouseDelta;

	float backgroundBaseMotionX = 0;
	float backgroundBaseMotionY = 0;

	float backgroundAnimationX = 0;
	float backgroundAnimationY = 0;

	float starsSmall0X = 0.0f;
	float starsSmall0Y = 0.0f;
	float starsSmall1X = 0.0f;
	float starsSmall1Y = 0.0f;
	float starsBigX = 0.0f;
	float starsBigY = 0.0f;

	bool shouldLeave = false;
	Texture2D noStarsBackground = LoadTexture("assets/backgrounds/background_no_stars.png");
	Texture2D starsBigBackground = LoadTexture("assets/backgrounds/background_stars_big.png");
	Texture2D starsSmall0Background = LoadTexture("assets/backgrounds/background_stars_small0.png");
	Texture2D starsSmall1Background = LoadTexture("assets/backgrounds/background_stars_small1.png");

	Rectangle b = { 0, 0, 0, 0 };

	auto pMainGrid = std::make_shared<Grid>(10, 1, 80, 500, 100, 50);

	//=====================================================================================================================
	// save loading buttons
	//=====================================================================================================================


	auto pSave1Button = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pSave1Button->addText("Load save 1", 40, WHITE);

	gridButtonSetup(pMainGrid, pSave1Button, 0, 0);


	auto pSave2Button = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pSave2Button->addText("Load save 2", 40, WHITE);

	gridButtonSetup(pMainGrid, pSave2Button, 1, 0);
 

	auto pSave3Button = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pSave3Button->addText("Load save 3", 40, WHITE);
	
	gridButtonSetup(pMainGrid, pSave3Button, 2, 0);


	//=====================================================================================================================
	// delete save buttons
	//=====================================================================================================================

	auto pDeleteSave1Button = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pDeleteSave1Button->addText("Delete save 1", 40, WHITE);

	pDeleteSave1Button->storeFunction([]() {
		clearDir("assets/saves/main_save");
		});

	gridButtonSetup(pMainGrid, pDeleteSave1Button, 4, 0);

	auto pDeleteSave2Button = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pDeleteSave2Button->addText("Delete save 2", 40, WHITE);

	gridButtonSetup(pMainGrid, pDeleteSave2Button, 5, 0);

	pDeleteSave2Button->storeFunction([]() {
		clearDir("assets/saves/secondary_save");
		});

	auto pDeleteSave3Button = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pDeleteSave3Button->addText("Delete save 3", 40, WHITE);

	gridButtonSetup(pMainGrid, pDeleteSave3Button, 6, 0);

	pDeleteSave3Button->storeFunction([]() {
		clearDir("assets/saves/tertiary_save");
		});


	//=====================================================================================================================
	// switch to level editor
	//=====================================================================================================================

	auto pStartLevelEditorButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pStartLevelEditorButton->addText("Open the level editor", 40, WHITE);

	gridButtonSetup(pMainGrid, pStartLevelEditorButton, 8, 0);

	pStartLevelEditorButton->storeFunction([&]() {
		changeScene(state, 2, shouldLeave);
		});

	//=====================================================================================================================
	// quit button
	//=====================================================================================================================

	auto pQuitButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	pQuitButton->addText("Quit the game", 40, WHITE);

	gridButtonSetup(pMainGrid, pQuitButton, 9, 0);

	pQuitButton->storeFunction([&]() {
		exitProgram(shouldQuit);
		});



	while (!WindowShouldClose() && !shouldQuit && (state == 0)) {

		time = GetTime();
		mouseDelta = GetMouseDelta();

		MousePosition::updateMousePos();
		Vector2 mouse = MousePosition::sMousePos;

		BeginDrawing();
		ClearBackground(GREEN);
		DrawTexturePro(noStarsBackground, Rectangle{ (float)noStarsBackground.width, (float)noStarsBackground.height, (float)noStarsBackground.width, (float)noStarsBackground.height}, Rectangle{ 0, 0, (float)windowWidth, (float)windowHeight }, { 0, 0 }, 0.0f, WHITE);
		
		shiftingBackground(starsSmall0Background, mouse, windowWidth, windowHeight, -0.75, 0.75, time, starsSmall0X, starsSmall0Y);

		shiftingBackground(starsSmall1Background, mouse, windowWidth, windowHeight, -0.75, 0.75, time, starsSmall1X, starsSmall1Y);

		shiftingBackground(starsBigBackground, mouse, windowWidth, windowHeight, 1.5, 1.5, time, starsBigX, starsBigY);
		pMainGrid->draw();

		EndDrawing();
	}

	UnloadTexture(noStarsBackground);
	UnloadTexture(starsBigBackground);
	UnloadTexture(starsSmall0Background);
	UnloadTexture(starsSmall1Background);
}