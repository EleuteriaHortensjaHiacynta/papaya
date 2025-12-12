#pragma once
#include <raylib.h>
#include <functional>
#include <tuple>
#include <utility>

class Button {
private:
	std::function<void()> mPassedFunction;


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

	void draw(Color Colour) {
		DrawRectangleRec(PositionSize, Colour);
	}

	//the template allows for passing a function of any type
	// and also allows passing any number of arguments of any type for that function
	template<typename Funct, typename... Args>
	// && allows to pass lvalues and rvalues which i dont exactly know what they are but i think it needs to be like this
	// gotta read more about them
	void storeFunction(Funct&& function, Args&&... args) {

		//all i know is that the compiler only accepted it like this and not when it was all done without a tuple
		//inside the lambda function AND needed a change from compiling using the c++14 standard to c++17
		//ill look into it later
		auto argsTuple = std::make_tuple(std::forward<Args>(args)...);

		//the lambda function that im almost sure i understand now
		//almost
		mPassedFunction = [function = std::forward<Funct>(function), argsTuple = std::move(argsTuple)]() mutable {
			std::apply(function, argsTuple);
			};
	}

	void detectMouseInteraction() {
		MousePos = GetMousePosition();
		//control output
		std::cout << MousePos.x << " " << MousePos.y << std::endl;
		if (CheckCollisionPointRec(MousePos, PositionSize)) {
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				draw(ColourClick);
				if (mPassedFunction) {
					mPassedFunction();
				}
			}
			else {
				draw(ColourHover);
			}
		}
		else {
			draw(ColourBase);
		}
	}


};