#pragma once

#include <raylib.h>
#include <iostream>
#include "GUI/GUI_raylib.hpp"
#include "Scenes/scene_functions.hpp"

void sceneLevelEditor(bool& shouldQuit, int& state, int windowHeight, int windowWidth) {
	shouldQuit = true;



	Texture2D textureAtlas = LoadTexture("src/GUI/textures/textureAtlasV1.png");
	



	int sidePanelWidth = 300;

	InteractiveGrid drawingScreen(64, 64, 310, 50, 8, 1200, 800);

	Grid topGrid(1, 10, 40, windowWidth / 10, 0, 0);
	//InteractiveGrid blockPanel()
	Grid bottomGrid(1, 4, 40, (windowWidth - sidePanelWidth) / 4, sidePanelWidth, windowHeight - 40);


	Grid tilePropertyGrid(4, 1, 50, sidePanelWidth, 0, windowHeight - 200);
	
	// tile property buttons
	auto pCollisionButton = std::make_shared<Button>(Rectangle{ 0, 0, 0, 0 }, GRAY, LIGHTGRAY, WHITE);
	tilePropertyGrid.insertWidget(2, 0, pCollisionButton);
	pCollisionButton->setPosition(tilePropertyGrid.mCells[2][0].rect);
	pCollisionButton->addText("Enable collision", 30, WHITE);

	auto pDamageButton = std::make_shared<Button>(Rectangle{ 0, 0, 0, 0 }, GRAY, LIGHTGRAY, WHITE);
	tilePropertyGrid.insertWidget(3, 0, pDamageButton);
	pDamageButton->setPosition(tilePropertyGrid.mCells[3][0].rect);
	pDamageButton->addText("Enable damage", 30, WHITE);


	int buttonTextureSize = 62;

	Grid textureSelection(10, 5, buttonTextureSize, buttonTextureSize, 0, 60);

	// tile selection buttons

	/*auto pID0Button = std::make_shared<Button>(textureSelection.mCells[0][0].rect, textureAtlas, 0, 62, 62);
	textureSelection.insertWidget(0, 0, pID0Button);

	pID0Button->storeFunction([&drawingScreen]() {drawingScreen.mCurrentID = 0;});

	auto pID1Button = std::make_shared<Button>(textureSelection.mCells[0][1].rect, textureAtlas, 1, 62, 62);
	textureSelection.insertWidget(0, 1, pID1Button);

	pID1Button->storeFunction([&drawingScreen]() {drawingScreen.mCurrentID = 1;});*/

	int textureID = 0;

	for (size_t row = 0; row < textureSelection.mCells.size(); row++) {
		for (size_t col = 0; col < textureSelection.mCells[row].size(); col++) {
			auto& cell = textureSelection.mCells[row][col];

			auto pButton = std::make_shared<Button>(cell.rect, textureAtlas, textureID, buttonTextureSize, buttonTextureSize);
			pButton->storeFunction([&drawingScreen, textureID]() {drawingScreen.mCurrentID = textureID;});
			textureSelection.insertWidget(row, col, pButton);
			textureID++;
		}
		
	}





	while(!WindowShouldClose()) {
		BeginDrawing();
		MousePosition::updateMousePos();
		topGrid.draw();
		bottomGrid.draw();
		tilePropertyGrid.draw();
		textureSelection.draw();
		drawingScreen.renderLines();
		drawingScreen.draw(textureAtlas);
		drawingScreen.gridInteraction();
		if (IsKeyPressed(KEY_E)) {
			
			drawingScreen.mCurrentID++;

			std::cout << "Current ID: " << drawingScreen.mCurrentID << std::endl;
		}
		else if (drawingScreen.mCurrentID > 14) {
			drawingScreen.mCurrentID = 0;
		}
		
		EndDrawing();
		ClearBackground(SKYBLUE);
	}

	UnloadTexture(textureAtlas);
	

	drawingScreen.toJson("drawingScreen0.json");
	drawingScreen.chunkToJson("drawingScreen1.json",0 ,0);

}




