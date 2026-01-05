#pragma once

#include <raylib.h>
#include <iostream>
#include <string>
#include <filesystem>
#include "external_headers/tinyfiledialogs.hpp"

inline std::string openFileDialog(const char *tittle = "Select file to open") {


	const char* patterns[] = { "*.json" };

	const char* result = tinyfd_openFileDialog(tittle, "", 1, patterns, "JSON files", 0);
	if (result) return std::string(result);
	return "";
}

// either all arguments need default values or none of them have defaults
inline std::string saveFileDialog(const char* tittle = "Select file to save", int x = 0, int y = 0) {

	std::filesystem::path currentPath = std::filesystem::current_path();

	std::filesystem::path saveDir = currentPath / "saved_chunks";

	if (!std::filesystem::exists(saveDir)) {
		std::filesystem::create_directory(saveDir);
	}

	std::string fileName = "chunk-" + std::to_string(x) + "-" + std::to_string(y) + ".json";

	std::filesystem::path defaultPath = saveDir / fileName ;
	
	std::string defaultName = defaultPath.string();


	
	const char* patterns[] = { "*.json" };
	
	const char* result = tinyfd_saveFileDialog(tittle, defaultName.c_str(), 1, patterns, "JSON files");
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

inline void changeDisplayedText(std::shared_ptr<Button> button, std::string coordinate, int value) {
	std::string text = "Chunk " + coordinate + ": " + std::to_string(value);
	button->addText(text.c_str(), 26, WHITE);
}

inline void changeChunkPos(int& chunkCoordinate, int changeBy) {
	if (IsKeyDown(KEY_LEFT_SHIFT)) chunkCoordinate += 10 * changeBy;
	else if (IsKeyDown(KEY_LEFT_CONTROL)) chunkCoordinate += 5 * changeBy;
	else chunkCoordinate += changeBy;
}

inline void chunkChangeAndDisplay(int& chunkCoordinate, int changeBy, std::shared_ptr<Button> button, std::string coordinate) {
	changeChunkPos(chunkCoordinate, changeBy);
	changeDisplayedText(button, coordinate, chunkCoordinate);
}