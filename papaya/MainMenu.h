#pragma once
#include <raylib.h>

void MainMenu(Vector2 MousePos, int WindowWidth, int WindowHeight) {
	MousePos = GetMousePosition();
	std::cout << MousePos.x << " " << MousePos.y << std::endl;

	float ExitButtonHeight = 50;
	float ExitButtonWidth = 300;

	// jak Rectangle wsadzasz do DrawRectangleRec to {xpos, ypos, width, height}
	//Rectangle Button = { WindowWidth / 4 - ExitButtonWidth / 2, WindowHeight / 5 - ExitButtonHeight / 2 , ExitButtonWidth, ExitButtonHeight};
	//Rectangle Button1 = { WindowWidth / 4 - ExitButtonWidth / 2, 1.5*WindowHeight / 5 - ExitButtonHeight / 2 , ExitButtonWidth, ExitButtonHeight };
	//Rectangle Button2 = { WindowWidth / 4 - ExitButtonWidth / 2, 2*WindowHeight / 5 - ExitButtonHeight / 2 , ExitButtonWidth, ExitButtonHeight };

	Rectangle MainButtons[5];
	for (int i = 0; i < 5; i++) {
		MainButtons[i] = { (float) WindowWidth / 4 - ExitButtonWidth / 2, (i * 0.5f + 2.5f) * (float) WindowHeight / 5 - ExitButtonHeight / 2 , ExitButtonWidth, ExitButtonHeight };
		DrawRectangleRec(MainButtons[i], GRAY);
		if (CheckCollisionPointRec(MousePos, MainButtons[i])) {
			DrawRectangleRec(MainButtons[i], RED);
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) DrawRectangleRec(MainButtons[i], SKYBLUE); ;
		} 
	}
	//DrawRectangleRec(Button, GRAY);
	//DrawRectangleRec(Button1, GRAY);
	//DrawRectangleRec(Button2, GRAY);


	/*if (CheckCollisionPointRec(MousePos, Button)) DrawRectangleRec(Button, RED);*/
}