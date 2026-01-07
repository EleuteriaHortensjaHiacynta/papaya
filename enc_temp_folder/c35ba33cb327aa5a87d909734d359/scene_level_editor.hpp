#pragma once

#include <raylib.h>
#include <iostream>
#include "GUI/GUI_raylib.hpp"
#include "Scenes/scene_functions.hpp"
#include "level_editor/functions_level_editor.hpp"


void sceneLevelEditor(bool& shouldQuit, int& state, int windowHeight, int windowWidth) {
	shouldQuit = true;


	const int TILE_SIZE = 8;

	Texture2D textureAtlas = LoadTexture("assets/tiles/texture_atlas_stars.png");
	int chunkX = 0;
	int chunkY = 0;
	
	//for giving buttons their size when theyre gonna have it changed immediately after creation
	Rectangle b = { 0, 0, 0, 0 };


	int sidePanelWidth = 300;

	InteractiveGrid drawingScreen(64, 64, 310, 50, 8, 1200, 800);

	auto pTopGrid = std::make_shared<Grid>(1, 10, 40, windowWidth / 10, 0, 0);
	//InteractiveGrid blockPanel()
	auto pBottomGrid = std::make_shared<Grid>(1, 8, 40, (windowWidth - sidePanelWidth) / 8, sidePanelWidth, windowHeight - 40);


	Grid tilePropertyGrid(2, 1, 50, sidePanelWidth, 0, windowHeight - 100);
	
	// tile property buttons
	auto pCollisionButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	tilePropertyGrid.insertWidget(0, 0, pCollisionButton);
	pCollisionButton->setPosition(tilePropertyGrid.cells[0][0].rect);
	pCollisionButton->addText("Enable collision", 30, WHITE);

	auto pDamageButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	tilePropertyGrid.insertWidget(1, 0, pDamageButton);
	pDamageButton->setPosition(tilePropertyGrid.cells[1][0].rect);
	pDamageButton->addText("Enable damage", 30, WHITE);


	//=====================================================================================================================
	// texture selection panel
	//=====================================================================================================================

	
	int buttonSlotSize = 64;
	int buttonTextureSize = 60;
	int textureColumns = textureAtlas.width / TILE_SIZE;
	int textureRows = textureAtlas.height / TILE_SIZE;
	int tileAmount = textureColumns * textureRows;
	int maxIndex = tileAmount - 1;


	auto textureSelection = std::make_shared<Grid>(textureRows, textureColumns, buttonSlotSize, buttonSlotSize, 0, 60);

	bool scrollPanelEnlarged = false;
	Rectangle scrollPanelSizeBase = { 0, 40, 300, 760 };
	Rectangle scrollPanelSizeEnlarged = { 0, 40, 1100, 760 };

	auto scrollPanel = std::make_shared <ScrollContainer>(scrollPanelSizeBase);
	scrollPanel->setChild(textureSelection);
	scrollPanel->setGridChild(textureSelection);
	scrollPanel->setCamera();
	Rectangle temp = scrollPanel->getRect();
	temp.y += 2;
	textureSelection->setPosition(temp);

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

	auto pChunkXControls = std::make_shared<Grid>(2, 1, 0, 0, 0, 0);

	auto pXUp = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	auto pXDown = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);


	subgridSetup(pChunkXControls, pBottomGrid, 0, 1);

	gridButtonSetup(pChunkXControls, pXUp, 0, 0);
	gridButtonSetup(pChunkXControls, pXDown, 1, 0);

	auto pChunkYControls = std::make_shared<Grid>(2, 1, 0, 0, 0, 0);

	subgridSetup(pChunkYControls, pBottomGrid, 0, 3);

	auto pYUp = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	auto pYDown = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);

	gridButtonSetup(pChunkYControls, pYUp, 0, 0);
	gridButtonSetup(pChunkYControls, pYDown, 1, 0);





	//=====================================================================================================================
	//cheating with buttons for displaying coordinates and giving functions to the side buttons
	//=====================================================================================================================

	auto pXDisplay = std::make_shared<Button>(b, DARKGRAY, DARKGRAY, DARKGRAY);
	gridButtonSetup(pBottomGrid, pXDisplay, 0, 0);
	
	pXDisplay->addText("Chunk X: " + std::to_string(chunkX), 26, WHITE);

	pXUp->storeFunction([&]() {
		chunkChangeAndDisplay(chunkX, 1, pXDisplay, "Chunk X:");
		});

	pXUp->addText("++", 20, WHITE);
	
	pXDown->storeFunction([&]() {
		chunkChangeAndDisplay(chunkX, -1, pXDisplay, "Chunk X:");
		});

	pXDown->addText("--", 20, WHITE);
	

	auto pYDisplay = std::make_shared<Button>(b, DARKGRAY, DARKGRAY, DARKGRAY);
	gridButtonSetup(pBottomGrid, pYDisplay, 0, 2);

	pYDisplay->addText("Chunk Y: " + std::to_string(chunkY), 26, WHITE);

	pYUp->storeFunction([&]() {
		chunkChangeAndDisplay(chunkY, 1, pYDisplay, "Chunk Y:");
		});

	pYUp->addText("++", 20, WHITE);

	pYDown->storeFunction([&]() {
		chunkChangeAndDisplay(chunkY, -1, pYDisplay, "Chunk Y:");
		});

	pYDown->addText("--", 20, WHITE);

	//=====================================================================================================================
	// zoom display
	//=====================================================================================================================

	auto pZoomDisplay = std::make_shared<Button>(b, GRAY, GRAY, GRAY);

	gridButtonSetup(pBottomGrid, pZoomDisplay, 0, 4);

	//=====================================================================================================================
	// collision, damage and position of hovered over tile
	//=====================================================================================================================


	auto pTileCollisionDisplay = std::make_shared<Button>(b, GRAY, GRAY, GRAY);
	auto pTileDamageDisplay = std::make_shared<Button>(b, GRAY, GRAY, GRAY);

	gridButtonSetup(pBottomGrid, pTileCollisionDisplay, 0, 5);
	gridButtonSetup(pBottomGrid, pTileDamageDisplay, 0, 6);

	auto pTilePosDiv = std::make_shared<Grid>(1, 2, 0, 0, 0, 0);
	auto pTileXDisplay = std::make_shared<Button>(b, GRAY, GRAY, GRAY);
	auto pTileYDisplay = std::make_shared<Button>(b, GRAY, GRAY, GRAY);

	subgridSetup(pTilePosDiv, pBottomGrid, 0, 7);
	gridButtonSetup(pTilePosDiv, pTileXDisplay, 0, 0);
	gridButtonSetup(pTilePosDiv, pTileYDisplay, 0, 1);

	pTileXDisplay->addText("X", 28, WHITE);
	pTileYDisplay->addText("Y", 28, WHITE);

	//=====================================================================================================================
	// save and load buttons
	//=====================================================================================================================

	// save button
	auto pSaveButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	gridButtonSetup(pTopGrid, pSaveButton, 0, 0);
	pSaveButton->addText("Save chunk", 25, WHITE);

	// & in the lambda allows it to access all variables in the scope
	pSaveButton->storeFunction([&]() {
		std::string path = saveChunkDialog("Save Chunk File", chunkX, chunkY);
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
	gridButtonSetup(pTopGrid, pLoadButton, 0, 1);
	pLoadButton->addText("Load chunk", 25, WHITE);

	pLoadButton->storeFunction([&]() {
		std::string path = openChunkDialog("Load Chunk File");
		if (!path.empty()) {
			std::cout << "Selected file: " << path << std::endl;
			drawingScreen.jsonToChunk(path, chunkX, chunkY);

			//update the displays
			changeDisplayedCoordinate(pXDisplay, "Chunk X:", chunkX);
			changeDisplayedCoordinate(pYDisplay, "Chunk Y:", chunkY);
		}
		});

	//=====================================================================================================================
	// save chunk as image button
	//=====================================================================================================================


	auto pSaveToImage = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	gridButtonSetup(pTopGrid, pSaveToImage, 0, 4);
	pSaveToImage->addText("Save as image", 20, WHITE);


	//pSaveButton->storeFunction([&]() {
	//	std::string path = saveChunkDialog("Save Chunk File", chunkX, chunkY);
	//	if (!path.empty()) {
	//		//control output
	//		std::cout << "Selected file to open: " << path << std::endl;
	//		drawingScreen.chunkToJson(path, chunkX, chunkY);
	//		//more control outputs
	//		std::cout << "Saved chunk to: " << path << std::endl;
	//	}
	//	}
	//);

	pSaveToImage->storeFunction([&]() {
		std::string path = saveChunkToImage("Save chunk as image");
		if (!path.empty()) {
			turnChunkToImage(TILE_SIZE, drawingScreen, textureAtlas, path);
		}
		});


	//=====================================================================================================================
	// damage and collision overlay buttons
	//=====================================================================================================================

	auto pDamageOverlayButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);
	auto pCollisionOverlayButton = std::make_shared<Button>(b, GRAY, LIGHTGRAY, WHITE);

	gridButtonSetup(pTopGrid, pCollisionOverlayButton, 0, 2);
	gridButtonSetup(pTopGrid, pDamageOverlayButton, 0, 3);



	//=====================================================================================================================
	//main loop
	//=====================================================================================================================

	while(!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(SKYBLUE);


		MousePosition::updateMousePos();

		//the panel where the drawing is done
		//drawn first so it doesnt interfere with the rest of ui
		drawingScreen.renderLines();
		drawingScreen.draw(textureAtlas);
		if (!CheckCollisionPointRec(MousePosition::sMousePos, scrollPanel->getRect())) {
			drawingScreen.gridInteraction();
		}
		
		//this draws the texture selection
		
		scrollPanel->draw();

		

		if (IsKeyPressed(KEY_TAB)) {
			scrollPanelEnlarged = !scrollPanelEnlarged;
			if (scrollPanelEnlarged) {
				scrollPanel->setRect(scrollPanelSizeEnlarged);
			}
			else scrollPanel->setRect(scrollPanelSizeBase);
		}

		pTopGrid->draw();
		pBottomGrid->draw();
		tilePropertyGrid.draw();
		

		zoomChangeDisplay(pZoomDisplay, drawingScreen.getCameraZoom());


		Vector2 hoveredTilePos= drawingScreen.hoveredCell();
		if (hoveredTilePos.x >= 0 && hoveredTilePos.y >= 0 && hoveredTilePos.x <= 63 && hoveredTilePos.y <= 63) {
			changeDisplayedCoordinate(pTileXDisplay, "X: ", hoveredTilePos.x);
			changeDisplayedCoordinate(pTileYDisplay, "Y: ", hoveredTilePos.y);
			
			std::string collision;
			if (drawingScreen.tileData[hoveredTilePos.y][hoveredTilePos.x].collision == 0) collision = "no collision";
			else collision = "has collision";
			pTileCollisionDisplay->addText(collision, 20, WHITE);
			
			std::string damage;
			if (drawingScreen.tileData[hoveredTilePos.y][hoveredTilePos.x].damage == 0) damage = "no damage";
			else damage = "deals damage";
			pTileDamageDisplay->addText(damage, 20, WHITE);

		}

		//im so tired that like an hour after making these i realised that i couldve just put functions inside the buttons
		collisionSelection(drawingScreen, pCollisionButton);
		damageSelection(drawingScreen, pDamageButton);
		enableCollisionOvelay(drawingScreen, pCollisionOverlayButton);
		enableDamageOvelay(drawingScreen, pDamageOverlayButton);
		toggleDamageCollisionAndOverlaysKeyboard(drawingScreen);



		//selects a texture to the right
		if (IsKeyPressed(KEY_D)) {
			drawingScreen.currentID++;
		}
		//selects the texture to the left
		else if (IsKeyPressed(KEY_A)) {
			drawingScreen.currentID--;
		}
		//selects the texture down a row
		else if (IsKeyPressed(KEY_S)) {
			drawingScreen.currentID += textureColumns;
		}
		//selects the texture up a row
		else if (IsKeyPressed(KEY_W)) {
			drawingScreen.currentID -= textureColumns;
		}

		if (drawingScreen.currentID > maxIndex) {
			drawingScreen.currentID = 0;
		}
		if (drawingScreen.currentID < 0) {
			drawingScreen.currentID = maxIndex;
		}

		textureSelection->selectCell(8, drawingScreen.currentID);
		//textureSelection->drawSelectedCell();

		
		EndDrawing();
		
	}

	UnloadTexture(textureAtlas);
	

	//drawingScreen.toJson("drawingScreen0.json");
	//drawingScreen.chunkToJson("drawingScreen1.json",0 ,0);

}




