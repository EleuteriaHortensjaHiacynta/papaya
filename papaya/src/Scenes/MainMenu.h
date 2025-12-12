#pragma once
#include <raylib.h>
#include "GUI/GUIRayLib.h"
#include <iostream>


void testerFunct() {
	std::cout << "It actually worked" << std::endl;
}

void MainMenu(Vector2 MousePos, int WindowWidth, int WindowHeight) {

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
	ButtonTest4.storeFunction(testerFunct);

	while (!WindowShouldClose()) {
		BeginDrawing();
		ButtonTest0.detectMouseInteraction();
		ButtonTest1.detectMouseInteraction();
		ButtonTest2.detectMouseInteraction();
		ButtonTest3.detectMouseInteraction();
		ButtonTest4.detectMouseInteraction();
		ClearBackground(GREEN);
		EndDrawing();
	}
}