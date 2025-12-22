#pragma once
#include <raylib.h>
#include <functional>
#include <tuple>
#include <utility>
#include <string>
#include <vector>
#include <iostream>

class Widget {
public:
	//using virtual makes it so when using the draw function using a pointer 
	//the function from the class is used instead of the widgets
	void virtual draw() = 0;


	//allows for setting the position of a widget after its creation
	void virtual setPosition(Rectangle rect) = 0;

	//from what im understanding this makes the destructor delete both widget and the derived classes when using delete
	virtual ~Widget() = default;
};

class Button : public Widget {
private:
	std::function<void()> mPassedFunction;

public:
	Rectangle positionSize;
	Vector2 mousePos;

	Color colourBase;
	Color colourHover;
	Color colourClick;
	Color textColour;

	std::string text;
	int fontSize;
	int textLength;

	int textX;
	int textY;

	Button(Rectangle positionSize, Color base, Color hover, Color click) {
		this->positionSize = positionSize;
		colourBase = base;
		colourHover = hover;
		colourClick = click;
	}

	void addText(std::string passedText, int passedFontSize, Color passedTextColour) {
		text = passedText;
		textLength = MeasureText(text.c_str(), passedFontSize);
		textColour = passedTextColour;
		fontSize = passedFontSize;
		textX = positionSize.x + (positionSize.width - textLength) / 2;
		textY = positionSize.y + (positionSize.height - fontSize) / 2;
	}

	void render(Color colour) {
		DrawRectangleRec(positionSize, colour);
		DrawText(text.c_str(), textX, textY, fontSize, textColour);
	}

	template<typename Funct, typename... Args>
	void storeFunction(Funct&& function, Args&&... args) {
		auto argsTuple = std::make_tuple(std::forward<Args>(args)...);

		mPassedFunction =
			[function = std::forward<Funct>(function),
			argsTuple = std::move(argsTuple)]() mutable {
			std::apply(function, argsTuple);
			};
	}

	void detectMouseInteraction() {
		mousePos = GetMousePosition();

		// control output
		std::cout << mousePos.x << " " << mousePos.y << std::endl;

		if (CheckCollisionPointRec(mousePos, positionSize)) {
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
				render(colourClick);
				if (mPassedFunction) {
					mPassedFunction();
				}
			}
			else {
				render(colourHover);
			}
		}
		else {
			render(colourBase);
		}
	}


	void Widget::draw() override {
		detectMouseInteraction();
	}

	void Widget::setPosition(Rectangle rect) override {
		positionSize.x = rect.x;
		positionSize.y = rect.y;
		positionSize.width = rect.width;
		positionSize.height = rect.height;
	}
};

struct Cell {
	Rectangle rect;

	std::shared_ptr<Widget> widget;

};

//Grid also inherits from Widget class so that it can hold subgrids

class Grid : public Widget{
private:
	const int mRows;
	int rowHeight;

	const int mColumns;
	int columnWidth;


	//from what i read there's no need to use delete later 
	// because this pointer doesnt own the parent grid so it shouldnt even delete it
	Grid* mpParentGrid = nullptr;

	

public:
	std::vector<std::vector<Cell>> cells;
	// top left corner is the origin of the grid, everything gets set relative to it
	int originX;
	int originY;

	Grid(int passedRows,
		int passedColumns,
		int passedRowHeight,
		int passedColumnWidth,
		int gridOriginX,
		int gridOriginY)
		: mRows(passedRows),
		mColumns(passedColumns),
		rowHeight(passedRowHeight),
		columnWidth(passedColumnWidth),
		cells(mRows, std::vector<Cell>(mColumns))
	{

		giveCoordinates(gridOriginX, gridOriginY);

	}

	void giveCoordinates(int xPos, int yPos) {
		originX = xPos;
		originY = yPos;

		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				cells[i][j].rect = {
					(float)(originX + j * columnWidth),
					(float)(originY + i * rowHeight),
					(float)columnWidth,
					(float)rowHeight
				};
			}
		}
	}

	void setParent(Grid* parent) {
		mpParentGrid = parent;
	}

	bool isSubgrid() {
		if (mpParentGrid == nullptr) {
			return false;
		}
		else {
			return true;
		}
	}

	void Widget::draw() override {
		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				if (cells[i][j].widget != nullptr) {
					cells[i][j].widget->draw();
				}
				else {
					DrawRectangleLinesEx(cells[i][j].rect, 1, GRAY);
				}
			}
		}
	}

	void Widget::setPosition(Rectangle rect) override {
		originX = rect.x;
		originY = rect.y;
		giveCoordinates(originX, originY);
	}

	void insertWidget(int row, int column, std::shared_ptr<Widget> passedWidget) {
		if (row < mRows && row >= 0 && column < mColumns && column >= 0) {
			cells[row][column].widget = passedWidget;
		}
	}


	//should only works when its a subgrid
	void expandSubgridToFillCell() {
		if (isSubgrid() == true) {
			columnWidth = mpParentGrid->columnWidth / mColumns;
			rowHeight = mpParentGrid->rowHeight / mRows;

			giveCoordinates(originX, originY);
		}

	}
	
};