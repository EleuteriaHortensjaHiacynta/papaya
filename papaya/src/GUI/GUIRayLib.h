#pragma once
#include <raylib.h>

class Button {
	public:
		Rectangle PositionSize;
		Vector2 MousePos;
		Color ColourBase;
		Color ColourHover;
		Color ColourClick;

	Button(Rectangle POSITIONSIZE, Color Base, Color Hover, Color Click) {
		PositionSize = POSITIONSIZE;
		ColourBase = Base;
		ColourHover = Hover;
		ColourClick = Click;
	}

	void Draw(Color Colour) {
		DrawRectangleRec(PositionSize, Colour);
	}

	void MouseDetection() {
		MousePos = GetMousePosition();
		//control output
		std::cout << MousePos.x << " " << MousePos.y << std::endl;
		if (CheckCollisionPointRec(MousePos, PositionSize)) {
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				Draw(ColourClick);
			}
			else {
				Draw(ColourHover);
			}
		}
		else {
			Draw(ColourBase);
		}
	}
};