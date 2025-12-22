#pragma once
#include <raylib.h>
#include "GUI/GUIRayLib.hpp"
#include <iostream>


void testerFunct() {
	std::cout << "It actually worked" << std::endl;
}

void quitFunct(bool &ShouldQuit){
	ShouldQuit = true;
}

void sceneMainMenu(Vector2 MousePos, int WindowWidth, int WindowHeight, bool &ShouldQuit) {

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
	ButtonTest4.storeFunction(quitFunct, std::ref(ShouldQuit));
	ButtonTest4.addText("Quit", 40, WHITE);


	Grid gridTest(3, 3, 200, 200, 500, 10);


	//makes a smart pointer so theres no manual deleting
	auto subgridTest = std::make_shared<Grid>(10, 3, 50, 50, 0, 0);

	gridTest.insertWidget(1, 1, subgridTest);
	subgridTest->setPosition(gridTest.cells[1][1].rect);
	subgridTest->setParent(&gridTest);
	subgridTest->expandSubgridToFillCell();



	while (!WindowShouldClose() && !ShouldQuit ) {
		BeginDrawing();
		ButtonTest0.detectMouseInteraction();
		ButtonTest1.detectMouseInteraction();
		ButtonTest2.detectMouseInteraction();
		ButtonTest3.detectMouseInteraction();
		ButtonTest4.detectMouseInteraction();
		gridTest.draw();
		ClearBackground(GREEN);
		EndDrawing();
	}

}