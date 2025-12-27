#pragma once
#include <raylib.h>
#include <iostream>
#include "GUI/GUI_raylib.hpp"
#include "Scenes/scene_functions.hpp"


//inline void changeScene(int& state, int value, bool &shouldLeave) {
//	state = value;
//	shouldLeave = true;
//}

void sceneTest(bool &shouldQuit, int &state) {

	bool shouldLeave = false; 

	Grid menuGrid(5, 1, 80, 300, 200, 300);
	auto pQuitButton = std::make_shared<Button>(Rectangle { 0, 0, 80, 300 }, GRAY, RED, BLUE);

	pQuitButton->addText("Quit", 30, WHITE);
	pQuitButton->storeFunction([&shouldQuit]() {exitProgram(shouldQuit); } );
	menuGrid.insertWidget(4, 0, pQuitButton);
	pQuitButton->setPosition(menuGrid.cells[4][0].rect);

	auto pChangeSceneButton = std::make_shared<Button>(Rectangle{ 0, 0, 80, 280 }, GRAY, RED, BLUE);
	pChangeSceneButton->addText("Change scene", 30, WHITE);
	pChangeSceneButton->storeFunction([&state, &shouldLeave]() 
		{changeScene(state, 0, shouldLeave); });
	menuGrid.insertWidget(0, 0, pChangeSceneButton);
	pChangeSceneButton->setPosition(menuGrid.cells[0][0].rect);



	while (!WindowShouldClose() && !shouldQuit && !shouldLeave) {
		BeginDrawing();
		MousePosition::updateMousePos();
		menuGrid.draw();
		
		ClearBackground(GREEN);
		EndDrawing();
	}

}