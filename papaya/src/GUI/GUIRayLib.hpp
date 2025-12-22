#pragma once
#include <raylib.h>
#include <functional>
#include <tuple>
#include <utility>
#include <string>
#include <vector>


class Button {
private:
	std::function<void()> mPassedFunction;


public:
	Rectangle PositionSize;
	Vector2 MousePos;
	
	Color ColourBase;
	Color ColourHover;
	Color ColourClick;
	Color TextColour;
	
	std::string Text;
	int FontSize;
	int TextLength;

	int TextX;
	int TextY;

	Button(Rectangle POSITIONSIZE, Color Base, Color Hover, Color Click) {
		PositionSize = POSITIONSIZE;
		ColourBase = Base;
		ColourHover = Hover;
		ColourClick = Click;
	}

	void addText(std::string PassedText, int PassedFontSize, Color PassedTextColour) {
		Text = PassedText;
		TextLength = MeasureText(Text.c_str(), PassedFontSize);
		TextColour = PassedTextColour;
		FontSize = PassedFontSize;
		TextX = PositionSize.x + (PositionSize.width - TextLength) / 2;
		TextY = PositionSize.y + (PositionSize.height - FontSize) / 2;
	}

	void draw(Color Colour) {
		DrawRectangleRec(PositionSize, Colour);
		DrawText(Text.c_str(), TextX, TextY, FontSize, TextColour);
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
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

class Grid {
private:
	const int Rows;
	const int RowHeight;

	const int Columns;
	const int ColumnWidth;

	std::vector < std::vector < Rectangle > > Cells;

public:

	//top left corner is the origin of the grid, everything gets set relative to it
	int OriginX;
	int OriginY;
	
	Grid(int PassedRows, int PassedColumns, int PassedRowHeight, int PassedColumnWidth,
		int GridOriginX, int GridOriginY)
		: Rows(PassedRows), Columns(PassedColumns),
		RowHeight(PassedRowHeight), ColumnWidth(PassedColumnWidth) 
	{
		OriginX = GridOriginX;
		OriginY = GridOriginY;


	}


};