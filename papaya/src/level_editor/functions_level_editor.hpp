#pragma once

#include <raylib.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <sstream>
#include <iomanip>

#include "external_headers/tinyfiledialogs.hpp"


inline std::string openChunkDialog(const char *tittle = "Select file to open") {

	std::filesystem::path currentPath = std::filesystem::current_path();

	std::filesystem::path saveDir = currentPath / "saved_chunks/";

	if (!std::filesystem::exists(saveDir)) {
		std::filesystem::create_directory(saveDir);
	}

	const char* patterns[] = { "*.json" };

	const char* result = tinyfd_openFileDialog(tittle, saveDir.string().c_str(), 1, patterns, "JSON files", 0);
	if (result) return std::string(result);
	return "";
}

// either all arguments need default values or none of them have defaults
inline std::string saveChunkDialog(const char* title = "Select file to save", int x = 0, int y = 0) {

	std::filesystem::path currentPath = std::filesystem::current_path();

	std::filesystem::path saveDir = currentPath / "saved_chunks";

	if (!std::filesystem::exists(saveDir)) {
		std::filesystem::create_directory(saveDir);
	}

	std::string fileName = "chunk-" + std::to_string(x) + "-" + std::to_string(y) + ".json";

	std::filesystem::path defaultPath = saveDir / fileName ;
	
	std::string defaultName = defaultPath.string();


	
	const char* patterns[] = { "*.json" };
	
	const char* result = tinyfd_saveFileDialog(title, defaultName.c_str(), 1, patterns, "JSON files");
	if (result) return std::string(result);
	return "";
}

inline std::string saveChunkToImage(const char* title) {

	const char* patterns[] = { "*.png" };

	std::string text = "image.png";

	const char* result = tinyfd_saveFileDialog(title, text.c_str(), 1, patterns, "PNG files");
	if (result) return std::string(result);
	return "";

}

inline void subgridSetup(std::shared_ptr<Grid> subGrid, std::shared_ptr<Grid> grid, int row, int column) {
	grid->insertWidget(row, column, subGrid);

	// .get gives a raw pointer instead of shared ptr
	subGrid->setParent(grid.get());
	subGrid->setPosition(grid->cells[row][column].rect);
	subGrid->expandSubgridToFillCell();
}

inline void gridButtonSetup(std::shared_ptr<Grid> grid, std::shared_ptr<Button> button, int row, int column) {
	grid->insertWidget(row, column, button);
	button->setPosition(grid->cells[row][column].rect);
}

inline void changeDisplayedCoordinate(std::shared_ptr<Button> button, std::string coordinate, int value) {
	std::string text = coordinate + std::to_string(value);
	button->addText(text.c_str(), 26, WHITE);
}

inline void changeChunkPos(int& chunkCoordinate, int changeBy) {
	if (IsKeyDown(KEY_LEFT_SHIFT)) chunkCoordinate += 10 * changeBy;
	else if (IsKeyDown(KEY_LEFT_CONTROL)) chunkCoordinate += 5 * changeBy;
	else chunkCoordinate += changeBy;
}

inline void chunkChangeAndDisplay(int& chunkCoordinate, int changeBy, std::shared_ptr<Button> button, std::string coordinate) {
	changeChunkPos(chunkCoordinate, changeBy);
	changeDisplayedCoordinate(button, coordinate, chunkCoordinate);
}

inline void zoomChangeDisplay(std::shared_ptr<Button> button, float zoom) {
	std::ostringstream zoomString;
	zoomString << std::fixed << std::setprecision(1) << zoom;
	button->addText(zoomString.str(), 28, WHITE);
}

inline void collisionSelection(std::shared_ptr<InteractiveGrid> grid, std::shared_ptr<Button> button) {
	if (grid->currentCollision == false) button->addText("Enable collision", 25, WHITE);
	else button->addText("Disable collision", 25, WHITE);
	if (CheckCollisionPointRec(MousePosition::sMousePos, button->positionSize) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		if (grid->currentCollision == false) {
			grid->currentCollision = true;
		}
		else {
			grid->currentCollision = false;
		}
	}
}

inline void damageSelection(std::shared_ptr<InteractiveGrid> grid, std::shared_ptr<Button> button) {
	if (grid->currentDamage == false) button->addText("Enable damage", 25, WHITE);
	else button->addText("Disable damage", 25, WHITE);
	if (CheckCollisionPointRec(MousePosition::sMousePos, button->positionSize) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		if (grid->currentDamage == false) {
			grid->currentDamage = true;
		}
		else {
			grid->currentDamage = false;
		}
	}
}

inline void enableCollisionOvelay(std::shared_ptr<InteractiveGrid> grid, std::shared_ptr<Button> button) {
	if (grid->collisionOverlayEnabled == false) button->addText("Collision overlay", 18, WHITE);
	else button->addText("Collision overlay", 18, BLACK);
	if (CheckCollisionPointRec(MousePosition::sMousePos, button->positionSize) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		if (grid->collisionOverlayEnabled == false) {
			grid->collisionOverlayEnabled = true;
		}
		else {
			grid->collisionOverlayEnabled = false;
		}
	}
}

inline void enableDamageOvelay(std::shared_ptr<InteractiveGrid> grid, std::shared_ptr<Button> button) {
	if (grid->damageOverlayEnabled == false) button->addText("Damage overlay", 18, WHITE);
	else button->addText("Damage overlay", 18, BLACK);
	if (CheckCollisionPointRec(MousePosition::sMousePos, button->positionSize) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		if (grid->damageOverlayEnabled == false) {
			grid->damageOverlayEnabled = true;
		}
		else {
			grid->damageOverlayEnabled = false;
		}
	}
}

//allows to toggle tile collision, damage and their corresponding overlays
// "Q" for toggling tile collision, "Shift + Q" for collision overlay
// "E" for toggling tile damage, "Shift + E" for damage overlay
inline void toggleDamageCollisionAndOverlaysKeyboard(std::shared_ptr<InteractiveGrid> grid) {
	if (IsKeyPressed(KEY_Q)) {
		if (IsKeyDown(KEY_LEFT_SHIFT)) {
			grid->collisionOverlayEnabled = !grid->collisionOverlayEnabled;
		}
		else grid->currentCollision = !grid->currentCollision;
	}
	else if (IsKeyPressed(KEY_E)) {
		if (IsKeyDown(KEY_LEFT_SHIFT)) {
			grid->damageOverlayEnabled = !grid->damageOverlayEnabled;
		}
		else grid->currentDamage = !grid->currentDamage;
	}
}


inline void turnChunkToImage(int tileSize, std::shared_ptr<InteractiveGrid> grid, Texture2D atlas, const std::string &path) {
	RenderTexture2D mapTarget = LoadRenderTexture(64 * tileSize, 64 * tileSize);

	BeginTextureMode(mapTarget);
	ClearBackground(BLANK);

	grid->drawToImage(atlas);

	EndTextureMode();

	Image mapImage = LoadImageFromTexture(mapTarget.texture);

	ImageFlipVertical(&mapImage);
	ExportImage(mapImage, path.c_str());

	UnloadImage(mapImage);
	UnloadRenderTexture(mapTarget);

}


inline void changeTileEntitySelector(bool isTile, bool enlarged, std::shared_ptr<ScrollContainer> tilePanel, std::shared_ptr<ScrollContainer> entityPanel, Rectangle tempLocation, Rectangle normalLocation, Rectangle enlargedLocation, std::shared_ptr<InteractiveGrid> screen) {
	if (isTile) {
		screen->isTileMode = true;
		tilePanel->setRect(normalLocation);
		entityPanel->setRect(tempLocation);
		if (enlarged) {
			tilePanel->setRect(enlargedLocation);
		}
		tilePanel->draw();
	}
	else {
		screen->isTileMode = false;
		tilePanel->setRect(tempLocation);
		entityPanel->setRect(normalLocation);
		if (enlarged) {
			entityPanel->setRect(enlargedLocation);
		}
		entityPanel->draw();
	}
}