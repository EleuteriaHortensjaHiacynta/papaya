#pragma once
#include <raylib.h>
#include "GUI/GUI_raylib.hpp"
#include <iostream>
#include "Scenes/scene_functions.hpp"


void testerFunct() {
	std::cout << "It actually worked" << std::endl;
}

void quitFunct(bool &shouldQuit){
	shouldQuit = true;
}


void sceneMainMenu(int WindowWidth, int WindowHeight, bool &shouldQuit, int &state) {

	bool shouldLeave = false;
	float ExitButtonHeight = 50;
	float ExitButtonWidth = 300;

	// jak Rectangle wsadzasz do DrawRectangleRec to {xpos, ypos, width, height}
	 
	Rectangle MainButtons[5];
	for (int i = 0; i < 5; i++) {
		MainButtons[i] = { (float) WindowWidth / 4 - ExitButtonWidth / 2, (i * 0.5f + 2.5f) * (float) WindowHeight / 5 - ExitButtonHeight / 2 , ExitButtonWidth, ExitButtonHeight };

	}

	Button ButtonTest0(MainButtons[0], GRAY, RED, SKYBLUE);
	Button ButtonTest1(MainButtons[1], GRAY, RED, SKYBLUE);
	Button ButtonTest2(MainButtons[2], GRAY, RED, SKYBLUE);
	Button ButtonTest3(MainButtons[3], GRAY, RED, SKYBLUE);
	Button ButtonTest4(MainButtons[4], GRAY, RED, SKYBLUE);
	//std::ref kiedy chcemy uzyc reference aka & do guzika
	ButtonTest4.storeFunction(quitFunct, std::ref(shouldQuit));
	ButtonTest4.addText("Quit", 40, WHITE);


	Grid gridTest(3, 3, 200, 200, 500, 10);


	//makes a smart pointer so theres no manual deleting
	auto subgridTest = std::make_shared<Grid>(10, 3, 50, 50, 0, 0);

	gridTest.insertWidget(1, 1, subgridTest);
	subgridTest->setPosition(gridTest.cells[1][1].rect);
	subgridTest->setParent(&gridTest);
	subgridTest->expandSubgridToFillCell();

	//ButtonTest0.storeFunction(changeScene, std::ref(state), 1, std::ref(shouldLeave));
	ButtonTest0.storeFunction([&state, &shouldLeave]() {
		changeScene(state, 1, shouldLeave); });
	ButtonTest0.addText("Change scene to test", 30, WHITE);



	while (!WindowShouldClose() && !shouldQuit && (state == 0)) {
		BeginDrawing();
		MousePosition::updateMousePos();
		ButtonTest0.detectMouseInteraction();
		ButtonTest1.detectMouseInteraction();
		ButtonTest2.detectMouseInteraction();
		ButtonTest3.detectMouseInteraction();
		ButtonTest4.detectMouseInteraction();
		gridTest.draw();
		std::cout << state << std::endl;
		ClearBackground(GREEN);
		EndDrawing();
	}

}