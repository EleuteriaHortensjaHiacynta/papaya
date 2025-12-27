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


//this class is used so whenever there is more than one button or item that needs to know the mouse position
//there is only one call to update the mouse position 
//the variable is accessed throguh MousePosition::sMousePos
class MousePosition {
public:
	// inline allows a header-defined static variable to have one shared instance and not one instance per header inclusion
	inline static Vector2 sMousePos;

	static void updateMousePos() {
		sMousePos = GetMousePosition();
		// control output
		std::cout << sMousePos.x << " " << sMousePos.y << std::endl;
	}
};

class Button : public Widget {
private:
	std::function<void()> mPassedFunction;

public:
	Rectangle positionSize;

	Color colourBase;
	Color colourHover;
	Color colourClick;
	Color textColour;

	std::string text;
	int fontSize = 0;
	int textLength = 0;

	int textX;
	int textY;

	Button(Rectangle positionSize, Color base, Color hover, Color click) {
		this->positionSize = positionSize;
		colourBase = base;
		colourHover = hover;
		colourClick = click;
	}

	//adds text to the button and roughly centers it
	void addText(std::string passedText, int passedFontSize, Color passedTextColour) {
		text = passedText;
		textLength = MeasureText(text.c_str(), passedFontSize);
		textColour = passedTextColour;
		fontSize = passedFontSize;
		textX = positionSize.x + (positionSize.width - textLength) / 2;
		textY = positionSize.y + (positionSize.height - fontSize) / 2;
	}


	//draws the button on screen
	void render(Color colour) {
		DrawRectangleRec(positionSize, colour);
		DrawText(text.c_str(), textX, textY, fontSize, textColour);
	}


	//the template allows for passing a function of any type
	// and also allows passing any number of arguments of any type for that function
	template<typename Funct, typename... Args>


	// && allows to pass lvalues and rvalues which i dont exactly know what they are but i think it needs to be like this
	// gotta read more about them
	//honestly just pass lambdas here because they dont break with multiple references and non references
	void storeFunction(Funct&& function, Args&&... args) {
		
		//all i know is that the compiler only accepted it like this and not when it was all done without a tuple
		//inside the lambda function AND needed a change from compiling using the c++14 standard to c++17
		//ill look into it later
		auto argsTuple = std::make_tuple(std::forward<Args>(args)...);


		//the lambda function that im almost sure i understand now
		//almost
		mPassedFunction =
			[function = std::forward<Funct>(function),
			argsTuple = std::move(argsTuple)]() mutable {
			std::apply(function, argsTuple);
			};

	}


	//updates the position of the mouse 
	void detectMouseInteraction() {
		if (CheckCollisionPointRec(MousePosition::sMousePos, positionSize)) {
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
		textX = positionSize.x + (positionSize.width - textLength) / 2;
		textY = positionSize.y + (positionSize.height - fontSize) / 2;
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

	
	//sets the grids origin and then gives every cell its own dimensions and location
	//based on the data stored inside the grid already
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

	//if the grid is a subgrid then this function is used with the pointer to the parent grid
	void setParent(Grid* parent) {
		mpParentGrid = parent;
	}

	//checks if the grid is a subgrid based on the pointer
	bool isSubgrid() {
		if (mpParentGrid == nullptr) {
			return false;
		}
		else {
			return true;
		}
	}


	//when the grid is a subgrid uses the pointer to the parent to resize itself to fit the cell its in
	void expandSubgridToFillCell() {
		if (isSubgrid() == true) {
			columnWidth = mpParentGrid->columnWidth / mColumns;
			rowHeight = mpParentGrid->rowHeight / mRows;

			giveCoordinates(originX, originY);
		}

	}

	//using a pointer inserts a widget inside a given cell in the grid
	void insertWidget(int row, int column, std::shared_ptr<Widget> passedWidget) {
		if (row < mRows && row >= 0 && column < mColumns && column >= 0) {
			cells[row][column].widget = passedWidget;
		}
	}


	//===============================================================================
	//overriden Widget functions ====================================================
	//===============================================================================


	//loops through the grid and calls the draw function on every widget inside
	//if there is no widget it draws a gray square border instead
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

	//changes the origin of the grid and readjusts the cells
	void Widget::setPosition(Rectangle rect) override {
		originX = rect.x;
		originY = rect.y;
		giveCoordinates(originX, originY);
	}

	
	
};

struct Line {
	Vector2 p1;
	Vector2 p2;
};

class InteractiveGrid {
private:
	const int mRows;

	const int mColumns;

public:
	std::vector<std::vector<Cell>> cells;
	// top left corner is the origin of the grid, everything gets set relative to it
	int originX;
	int originY;
	int tileSize;


	std::vector<Line> verticalLines;
	std::vector<Line> horizontalLines;

	InteractiveGrid(int passedRows,
		int passedColumns,
		int gridOriginX,
		int gridOriginY,
		int tileSize)
		: mRows(passedRows),
		mColumns(passedColumns),
		cells(mRows, std::vector<Cell>(mColumns)),
		verticalLines(mColumns),
		horizontalLines(mRows),
		tileSize(tileSize)
	{

		giveCoordinates(gridOriginX, gridOriginY);
		createLines();
	}

	void giveCoordinates(int xPos, int yPos) {
		originX = xPos;
		originY = yPos;

		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				cells[i][j].rect = {
					(float)(originX + j * tileSize),
					(float)(originY + i * tileSize),
					(float)tileSize,
					(float)tileSize
				};
			}
		}
	}

	void createLines() {
		for (int i = 0; i < mColumns; i++) {
			float pX = originX + tileSize / 2 + i * tileSize;
			float p1Y = originY;
			float p2Y = originY + mRows * tileSize;

			verticalLines[i].p1 = { pX, p1Y };
			verticalLines[i].p2 = { pX, p2Y } ;

		}

		for (int i = 0; i < mRows; i++) {
			float pY = originY + tileSize / 2 + i * tileSize;
			float p1X = originX;
			float p2X = originX + mColumns * tileSize;
			horizontalLines[i].p1 = { p1X ,pY };
			horizontalLines[i].p2 = { p2X, pY };
		}
	}

	void renderLines() {
		for (int i = 0; i < mColumns; i++) {
			DrawLineV(verticalLines[i].p1, verticalLines[i].p2, WHITE);
		}
		for (int i = 0; i < mRows; i++) {
			DrawLineV(horizontalLines[i].p1, horizontalLines[i].p2, WHITE);
		}
		
	}

	void draw() {
		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
					DrawRectangleLinesEx(cells[i][j].rect, 1, GRAY);
			}
		}
	}

	void interactionDetection() {
		Rectangle temp = { (float)originX, (float)originY, (float)(mColumns * tileSize), (float)(mRows * tileSize) };
		int rowPos = -1;
		int columnPos = -1;
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(MousePosition::sMousePos, temp)) {
			/*for (int i = 0; i < mRows; i++) {
				if (CheckCollisionPointLine(MousePosition::sMousePos, horizontalLines[i].p1, horizontalLines[i].p2, tileSize / 2)) {
					rowPos = i;
					break;
				}
			}
			for (int i = 0; i < mColumns; i++) {
				if (CheckCollisionPointLine(MousePosition::sMousePos, verticalLines[i].p1, verticalLines[i].p2, tileSize / 2)) {
					columnPos = i;
					break;
				}
			}
			std::cout << "Grid position: " << rowPos << " ; " << columnPos << std::endl;
		*/
			Vector2 mouse = GetMousePosition();
			int column = (mouse.x - originX) / tileSize;
			int row = (mouse.y - originY) / tileSize;

			std::cout << "Grid position: " << row << " ; " << column << std::endl;
		}

		
	}

};