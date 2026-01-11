#pragma once
#include <raylib.h>

inline void changeScene(int& state, int value, bool& shouldLeave) {
	state = value;
	shouldLeave = true;
}

inline void exitProgram(bool& shouldQuit) {
	shouldQuit = true;
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

inline float smoothing(float current, float target, float speed) {
	return current + (target - current) * (1.0f - expf(-speed * GetFrameTime()));
}

inline void shiftingBackground(Texture2D image, Vector2 mouse, int windowWidth, int windowHeight, float xShift, float yShift, float time) {
	static float shiftX = 0.0f;
	static float shiftY = 0.0f;
	
	float x = mouse.x * 0.1f / (windowWidth * 5.0f);
	float y = mouse.y * 0.1f / (windowHeight * 5.0f);

	float baseX = sinf(time * 0.25f) * 0.005f;
	float baseY = cosf(time * 0.25f) * 0.005f;
	
	float mouseShiftX = x * xShift;
	float mouseShiftY = y * yShift;

	float shiftBaseX = baseX + mouseShiftX;
	float shiftBaseY = baseY + mouseShiftY;

	shiftX = smoothing(shiftX, shiftBaseX, 0.1f);
	shiftY = smoothing(shiftY, shiftBaseY, 0.1f);

	DrawTexturePro(image, Rectangle{ (float)image.width * (0.25f + shiftX), (float)image.height * (0.25f + shiftY), (float)image.width * 0.75f, (float)image.height * 0.75f}, Rectangle{0, 0, (float)windowWidth, (float)windowHeight}, {0, 0}, 0.0f, WHITE);
}