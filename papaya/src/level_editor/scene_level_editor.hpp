#pragma once

#include <raylib.h>
#include <iostream>
#include "GUI/GUI_raylib.hpp"
#include "Scenes/scene_functions.hpp"

void sceneLevelEditor(bool& shouldQuit, int& state, int windowHeight, int windowWidth) {
	shouldQuit = true;



	Texture2D textureAtlas = LoadTexture("src/GUI/textures/testingAtlas0.png");
	



	int sidePanelWidth = 300;

	Grid topGrid(1, 10, 40, windowWidth / 10, 0, 0);
	//InteractiveGrid blockPanel()
	Grid bottomGrid(1, 4, 40, (windowWidth - sidePanelWidth) / 4, sidePanelWidth, windowHeight - 40);
	Grid tilePropertyGrid(4, 1, 50, sidePanelWidth, 0, windowHeight - 200);

	InteractiveGrid test(64, 64, 310, 50, 8);


	while(!WindowShouldClose()) {
		BeginDrawing();
		MousePosition::updateMousePos();
		topGrid.draw();
		bottomGrid.draw();
		tilePropertyGrid.draw();
		test.renderLines();
		test.draw(textureAtlas);
		test.gridInteraction();

		std::cout << "Testing." << std::endl;
		
		EndDrawing();
		ClearBackground(SKYBLUE);
	}

	UnloadTexture(textureAtlas);
	

	test.toJson("test0.json");

}




