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
	
	//for giving buttons their size when theyre gonna have it changed immediately after creation
	Rectangle b = { 0, 0, 0, 0 };


	int sidePanelWidth = 300;

	InteractiveGrid drawingScreen(64, 64, 310, 50, 8, 1200, 800);

	Grid topGrid(1, 10, 40, windowWidth / 10, 0, 0);
	//InteractiveGrid blockPanel()
	Grid bottomGrid(1, 4, 40, (windowWidth - sidePanelWidth) / 4, sidePanelWidth, windowHeight - 40);


	Grid tilePropertyGrid(4, 1, 50, sidePanelWidth, 0, windowHeight - 200);
	
	// tile property buttons
	auto pCollisionButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	tilePropertyGrid.insertWidget(2, 0, pCollisionButton);
	pCollisionButton->setPosition(tilePropertyGrid.cells[2][0].rect);
	pCollisionButton->addText("Enable collision", 30, WHITE);

	auto pDamageButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	tilePropertyGrid.insertWidget(3, 0, pDamageButton);
	pDamageButton->setPosition(tilePropertyGrid.cells[3][0].rect);
	pDamageButton->addText("Enable damage", 30, WHITE);


	//=====================================================================================================================
	//=====================================================================================================================
	//=====================================================================================================================

	// texture selection panel
	int buttonSlotSize = 62;
	int buttonTextureSize = 60;
	int tileAmount = 100;
	int textureColumns = 5;


	auto textureSelection = std::make_shared<Grid>((int)(tileAmount / textureColumns), textureColumns, buttonSlotSize, buttonSlotSize, 0, 60);

	auto scrollPanel = std::make_shared <ScrollContainer>(Rectangle{ 0, 60, 310, 600 });
	scrollPanel->setChild(textureSelection);
	scrollPanel->setCamera();

	int textureID = 0;

	for (size_t row = 0; row < textureSelection->cells.size(); row++) {
		for (size_t col = 0; col < textureSelection->cells[row].size(); col++) {
			auto& cell = textureSelection->cells[row][col];

			auto pButton = std::make_shared<Button>(cell.rect, textureAtlas, textureID, buttonTextureSize, buttonTextureSize);
			pButton->storeFunction([&drawingScreen, textureID]() {drawingScreen.currentID = textureID;});
			textureSelection->insertWidget(row, col, pButton);
			textureID++;
		}
		
	}





	//=====================================================================================================================
	//chunk coordinate changing
	//=====================================================================================================================
	auto pChunkXInfo = std::make_shared<Grid>(1, 2, 0, 0, 0, 0);

	bottomGrid.insertWidget(0, 0, pChunkXInfo);
	pChunkXInfo->setParent(&bottomGrid);
	pChunkXInfo->setPosition(bottomGrid.cells[0][0].rect);
	pChunkXInfo->expandSubgridToFillCell();


	auto pChunkYInfo = std::make_shared<Grid>(1, 2, 0, 0, 0, 0);

	bottomGrid.insertWidget(0, 1, pChunkYInfo);
	pChunkYInfo->setParent(&bottomGrid);
	pChunkYInfo->setPosition(bottomGrid.cells[0][1].rect);
	pChunkYInfo->expandSubgridToFillCell();


	auto pChunkXControls = std::make_shared<Grid>(2, 1, 0, 0, 0, 0);

	auto pXUp = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	auto pXDown = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);


	subgridSetup(pChunkXControls, pChunkXInfo, 0, 1);

	gridButtonSetup(pChunkXControls, pXUp, 0, 0);
	gridButtonSetup(pChunkXControls, pXDown, 1, 0);

	auto pChunkYControls = std::make_shared<Grid>(2, 1, 0, 0, 0, 0);

	subgridSetup(pChunkYControls, pChunkYInfo, 0, 1);

	auto pYUp = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	auto pYDown = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);

	gridButtonSetup(pChunkYControls, pYUp, 0, 0);
	gridButtonSetup(pChunkYControls, pYDown, 1, 0);





	//=====================================================================================================================
	//cheating with buttons for displaying coordinates and giving functions to the side buttons
	//=====================================================================================================================

	auto pXDisplay = std::make_shared<Button>(b, DARKGRAY, DARKGRAY, DARKGRAY);
	gridButtonSetup(pChunkXInfo, pXDisplay, 0, 0);
	
	pXDisplay->addText("Chunk X: " + std::to_string(chunkX), 26, WHITE);

	pXUp->storeFunction([&]() {
		chunkChangeAndDisplay(chunkX, 1, pXDisplay, "X");
		});

	pXUp->addText("++", 20, WHITE);
	
	pXDown->storeFunction([&]() {
		chunkChangeAndDisplay(chunkX, -1, pXDisplay, "X");
		});

	pXDown->addText("--", 20, WHITE);
	

	auto pYDisplay = std::make_shared<Button>(b, DARKGRAY, DARKGRAY, DARKGRAY);
	gridButtonSetup(pChunkYInfo, pYDisplay, 0, 0);

	pYDisplay->addText("Chunk Y: " + std::to_string(chunkY), 26, WHITE);

	pYUp->storeFunction([&]() {
		chunkChangeAndDisplay(chunkY, 1, pYDisplay, "Y");
		});

	pYUp->addText("++", 20, WHITE);

	pYDown->storeFunction([&]() {
		chunkChangeAndDisplay(chunkY, -1, pYDisplay, "Y");
		});

	pYDown->addText("--", 20, WHITE);

	//=====================================================================================================================
	// save and load buttons
	//=====================================================================================================================

	// save button
	auto pSaveButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	topGrid.insertWidget(0, 0, pSaveButton);
	pSaveButton->setPosition(topGrid.cells[0][0].rect);
	pSaveButton->addText("Save chunk", 20, WHITE);

	// & in the lambda allows it to access all variables in the scope
	pSaveButton->storeFunction([&]() {
		std::string path = saveFileDialog("Save Chunk File", chunkX, chunkY);
		if (!path.empty()) {
			//control output
			std::cout << "Selected file to open: " << path << std::endl;
			drawingScreen.chunkToJson(path, chunkX, chunkY);
			//more control outputs
			std::cout << "Saved chunk to: " << path << std::endl;
		}
		}
	);

	// load button 
	auto pLoadButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	topGrid.insertWidget(0, 1, pLoadButton);
	pLoadButton->addText("Load chunk", 20, WHITE);
	pLoadButton->setPosition(topGrid.cells[0][1].rect);

	pLoadButton->storeFunction([&]() {
		std::string path = openFileDialog("Load Chunk File");
		if (!path.empty()) {
			std::cout << "Selected file: " << path << std::endl;
			drawingScreen.jsonToChunk(path, chunkX, chunkY);

			//update the displays
			changeDisplayedText(pXDisplay, "X", chunkX);
			changeDisplayedText(pYDisplay, "Y", chunkY);
		}
		});


	//=====================================================================================================================
	//main loop
	//=====================================================================================================================


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
			
			drawingScreen.currentID++;

			std::cout << "Current ID: " << drawingScreen.currentID << std::endl;
		}
		//selects the texture to the left
		else if (IsKeyPressed(KEY_Q)) {
			drawingScreen.currentID--;
		}
		//selects the texture down a row
		else if (IsKeyPressed(KEY_A)) {
			drawingScreen.currentID += textureColumns;
		}
		//selects the texture up a row
		else if (IsKeyPressed(KEY_D)) {
			drawingScreen.currentID -= textureColumns;
		}

		if (drawingScreen.currentID > tileAmount) {
			drawingScreen.currentID = 0;
		}
		if (drawingScreen.currentID < 0) {
			drawingScreen.currentID = 144;
		}
		

		
		EndDrawing();
		
	}

	UnloadTexture(textureAtlas);
	

	//drawingScreen.toJson("drawingScreen0.json");
	//drawingScreen.chunkToJson("drawingScreen1.json",0 ,0);

}




