#pragma once

#include <raylib.h>
#include <iostream>
#include "GUI/GUI_raylib.hpp"
#include "Scenes/scene_functions.hpp"
#include "level_editor/functions_level_editor.hpp"


void sceneLevelEditor(bool& shouldQuit, int& state, int windowHeight, int windowWidth) {
	shouldQuit = true;



	Texture2D textureAtlas = LoadTexture("src/GUI/textures/texture_atlas_v3.png");
	int chunkX = 0;
	int chunkY = 0;
	



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


	// save button
	auto pSaveButton = std::make_shared<Button>(Rectangle{ 0, 0, 0, 0 }, GRAY, LIGHTGRAY, WHITE);
	topGrid.insertWidget(0, 0, pSaveButton);
	pSaveButton->setPosition(topGrid.mCells[0][0].rect);
	pSaveButton->addText("Save chunk", 20, WHITE);

	// & in the lambda allows it to access all variables in the scope
	pSaveButton->storeFunction([&]() {
		std::string path = openFileDialog("Save Chunk File");
		if (!path.empty()) {
			//control output
			std::cout << "Selected file to open: " << path << std::endl;
			drawingScreen.chunkToJson(path, chunkX, chunkY);
			//more control outputs
			std::cout << "Saved chunk to: " << path << std::endl;
		}
		});


	// texture selection panel

	int buttonSlotSize = 62;
	int buttonTextureSize = 60;
	int tileAmount = 100;
	int textureColumns = 5;

	//Grid textureSelection((int) (tileAmount/textureColumns), textureColumns, buttonSlotSize, buttonSlotSize, 0, 60);

	auto textureSelection = std::make_shared<Grid>((int)(tileAmount / textureColumns), textureColumns, buttonSlotSize, buttonSlotSize, 0, 60);

	auto scrollPanel = std::make_shared <ScrollContainer>(Rectangle{ 0, 60, 310, 600 });
	scrollPanel->setChild(textureSelection);
	scrollPanel->setCamera();

	int textureID = 0;

	for (size_t row = 0; row < textureSelection->mCells.size(); row++) {
		for (size_t col = 0; col < textureSelection->mCells[row].size(); col++) {
			auto& cell = textureSelection->mCells[row][col];

			auto pButton = std::make_shared<Button>(cell.rect, textureAtlas, textureID, buttonTextureSize, buttonTextureSize);
			pButton->storeFunction([&drawingScreen, textureID]() {drawingScreen.mCurrentID = textureID;});
			textureSelection->insertWidget(row, col, pButton);
			textureID++;
		}
		
	}





	while(!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(SKYBLUE);


		MousePosition::updateMousePos();
		topGrid.draw();
		bottomGrid.draw();
		tilePropertyGrid.draw();
		
		
		//this draws the texture selection
		scrollPanel->draw();
		//textureSelection.draw();

		drawingScreen.renderLines();
		drawingScreen.draw(textureAtlas);
		drawingScreen.gridInteraction();
		//selects a texture to the right
		if (IsKeyPressed(KEY_E)) {
			
			drawingScreen.mCurrentID++;

			std::cout << "Current ID: " << drawingScreen.mCurrentID << std::endl;
		}
		//selects the texture to the left
		else if (IsKeyPressed(KEY_Q)) {
			drawingScreen.mCurrentID--;
		}
		//selects the texture down a row
		else if (IsKeyPressed(KEY_A)) {
			drawingScreen.mCurrentID += textureColumns;
		}
		//selects the texture up a row
		else if (IsKeyPressed(KEY_D)) {
			drawingScreen.mCurrentID -= textureColumns;
		}

		if (drawingScreen.mCurrentID > tileAmount) {
			drawingScreen.mCurrentID = 0;
		}
		if (drawingScreen.mCurrentID < 0) {
			drawingScreen.mCurrentID = 144;
		}
		

		
		EndDrawing();
		
	}

	UnloadTexture(textureAtlas);
	

	drawingScreen.toJson("drawingScreen0.json");
	drawingScreen.chunkToJson("drawingScreen1.json",0 ,0);

}




