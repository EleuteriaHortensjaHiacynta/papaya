#pragma once
#include <raylib.h>
#include <functional>
#include <tuple>
#include <utility>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "external_headers/json.hpp"

using json = nlohmann::json;

inline Rectangle fetchTexture(Texture2D atlas, int tileSize, int index) {
	const int ATLAS_WIDTH = 8;
	const int ATLAS_HEIGHT = 8; 

	int tileX = index % ATLAS_WIDTH;
	int tileY = index / ATLAS_HEIGHT;

	return Rectangle{
	(float)tileX * tileSize,
	(float)tileY * tileSize,
	(float)tileSize,
	(float)tileSize
	};
}

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

	Texture2D mAtlas;
	bool hasImage = false;
	int mHeldID;
	int mImHeight;
	int mImWidth;

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

	Button(Rectangle positionSize, Texture2D atlas, int ID, int imWidth, int imHeight) {
		this->positionSize = positionSize;
		mAtlas = atlas;
		hasImage = true;
		mHeldID = ID;
		mImWidth = imWidth;
		mImHeight = imHeight;
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

		bool hovered = CheckCollisionPointRec(MousePosition::sMousePos, positionSize);
		bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

		if (hasImage) {
			Rectangle src = fetchTexture(mAtlas, 8, mHeldID);
			Rectangle dst = {
				positionSize.x,
				positionSize.y,
				(float)mImWidth,
				(float)mImHeight
			};
			if (clicked) {
				DrawTexturePro(mAtlas, src, dst, Vector2{ 0, 0 }, 0.0f, GRAY);
			}
			else {
				DrawTexturePro(mAtlas, src, dst, Vector2{ 0, 0 }, 0.0f, WHITE);
			}

		}

		else {
			if (clicked) {
				render(colourClick);
			}
			else if (hovered) {
				render(colourHover);
			}
			else {
				render(colourBase);
			}
		}

		if (clicked && mPassedFunction) {
			mPassedFunction();
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
	const int mColumns;
	int rowHeight;
	int columnWidth;

	//from what i read there's no need to use delete later 
	// because this pointer doesnt own the parent grid so it shouldnt even delete it
	Grid* mpParentGrid = nullptr;


public:
	std::vector<std::vector<Cell>> mCells;
	// top left corner is the origin of the grid, everything gets set relative to it
	int mOriginX;
	int mOriginY;


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
		mCells(mRows, std::vector<Cell>(mColumns))
	{

		giveCoordinates(gridOriginX, gridOriginY);

	}

	
	//sets the grids origin and then gives every cell its own dimensions and location
	//based on the data stored inside the grid already
	void giveCoordinates(int xPos, int yPos) {
		mOriginX = xPos;
		mOriginY = yPos;

		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				mCells[i][j].rect = {
					(float)(mOriginX + j * columnWidth),
					(float)(mOriginY + i * rowHeight),
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

			giveCoordinates(mOriginX, mOriginY);
		}

	}

	//using a pointer inserts a widget inside a given cell in the grid
	void insertWidget(int row, int column, std::shared_ptr<Widget> passedWidget) {
		if (row < mRows && row >= 0 && column < mColumns && column >= 0) {
			mCells[row][column].widget = passedWidget;
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
				if (mCells[i][j].widget != nullptr) {
					mCells[i][j].widget->draw();
				}
				else {
					DrawRectangleLinesEx(mCells[i][j].rect, 1, GRAY);
				}
			}
		}
	}

	//changes the origin of the grid and readjusts the mCells
	void Widget::setPosition(Rectangle rect) override {
		mOriginX = rect.x;
		mOriginY = rect.y;
		giveCoordinates(mOriginX, mOriginY);
	}

	
	
};

struct Line {
	Vector2 p1;
	Vector2 p2;
};

struct TileData {
	int textureID;
	int layer;
	bool collision;
	bool damage;
};



struct Chunk {
	int x;
	int y;

	TileData array[64][64];
};


//json library functions so that it works with TileData struct

void to_json(json& j, const TileData& t) {
	j = json{
		{"textureID", t.textureID},
		{"layer", t.layer},
		{"collision", t.collision},
		{"damage", t.damage}
	};
}

void from_json(const json& j, TileData& t) {
	j.at("textureID").get_to(t.textureID);
	j.at("layer").get_to(t.layer);
	j.at("collision").get_to(t.collision);
	j.at("damage").get_to(t.damage);
}

//json conversion Chunk struct

void to_json(json& j, const Chunk& t) {
	j["x"] = t.x;
	j["y"] = t.y;

	j["array"] = json::array();

	for (int row = 0; row < 64; row++) {
		json rowArray = json::array();
		for (int column = 0; column < 64; column++) {
			rowArray.push_back(t.array[row][column]);
		}
		j["array"].push_back(rowArray);
	}
}

void from_json(const json& j, Chunk& t) {
	j.at("x").get_to(t.x);
	j.at("y").get_to(t.y);

	const auto& arr = j.at("array");

	for (int row = 0; row < 64; row++) {
		for (int column = 0; column < 64; column++) {
			arr[row][column].get_to(t.array[row][column]);
		}
	}
}


class InteractiveGrid {
private:
	const int mRows;

	const int mColumns;

	Camera2D mCamera;

public:
	std::vector<std::vector<Cell>> mCells;
	// top left corner is the origin of the grid, everything gets set relative to it
	int mOriginX;
	int mOriginY;
	int mTileSize;
	int mGridWidth;
	int mGridHeight;
	int mScreenWidth;
	int mScreenHeight;
	Rectangle mViewport;

	//tile variables
	int mCurrentlyDrawnID;
	int mCurrentID;
	int mCurrentLayer;
	bool mCurrentCollision;
	bool mCurrentDamage;

	//used for visualising the center of mCells in the grid without textures
	std::vector<Line> mVerticalLines;
	std::vector<Line> mHorizontalLines;

	std::vector< std::vector < TileData > > mTileData;

	InteractiveGrid(int passedRows,
		int passedColumns,
		int gridOriginX,
		int gridOriginY,
		int tileSize,
		int screenWidth,
		int screenHeight)
		: mRows(passedRows),
		mColumns(passedColumns),
		mCells(mRows, std::vector<Cell>(mColumns)),
		mTileData(mRows, std::vector<TileData>(mColumns)),
		mVerticalLines(mColumns),
		mHorizontalLines(mRows),
		mTileSize(tileSize),
		mOriginX(gridOriginX),
		mOriginY(gridOriginY),
		mScreenHeight(screenHeight),
		mScreenWidth(screenWidth)

	{
		mCurrentID = 1;
		mCurrentLayer = 0;
		mCurrentCollision = false;
		mCurrentDamage = false;

		giveCoordinates(mOriginX, mOriginY);
		createLines();

		mCamera.target = {0, 0};
		//mCamera.offset = { mViewport.x + mGridWidth / 2.0f, mViewport.y + mGridHeight / 2.0f };
		mCamera.offset = { mViewport.x, mViewport.y };
		mCamera.rotation = 0.0f;
		mCamera.zoom = 1.5f;

	}

	void giveCoordinates(int xPos, int yPos) {

		mGridHeight = mRows * mTileSize;
		mGridWidth = mColumns * mTileSize;

		mViewport = { (float) xPos, (float) yPos, (float)mScreenWidth, (float)mScreenHeight };

		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				mCells[i][j].rect = {
					(float)(j * mTileSize),
					(float)(i * mTileSize),
					(float)mTileSize,
					(float)mTileSize
				};
				mTileData[i][j] = { 0, 0, 0, 0 };
			}
		}
	}

	void createLines() {
		for (int i = 0; i < mColumns; i++) {
			float pX = mTileSize / 2 + i * mTileSize;
			float p1Y = 0;
			float p2Y = mRows * mTileSize;

			mVerticalLines[i].p1 = { pX, p1Y };
			mVerticalLines[i].p2 = { pX, p2Y };

		}

		for (int i = 0; i < mRows; i++) {
			float pY = mTileSize / 2 + i * mTileSize;
			float p1X = 0;
			float p2X = mColumns * mTileSize;
			mHorizontalLines[i].p1 = { p1X ,pY };
			mHorizontalLines[i].p2 = { p2X, pY };
		}
	}

	void renderLines() {
		BeginScissorMode(mViewport.x, mViewport.y, mViewport.width, mViewport.height);
		BeginMode2D(mCamera);
		for (int i = 0; i < mColumns; i++) {
			DrawLineV(mVerticalLines[i].p1, mVerticalLines[i].p2, WHITE);
		}
		for (int i = 0; i < mRows; i++) {
			DrawLineV(mHorizontalLines[i].p1, mHorizontalLines[i].p2, WHITE);
		}
		EndMode2D();
		EndScissorMode();
	}

	void draw(Texture2D atlas) {
		BeginScissorMode(mViewport.x, mViewport.y, mViewport.width, mViewport.height);
		BeginMode2D(mCamera);
		
		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				mCurrentlyDrawnID = mTileData[i][j].textureID;
				Rectangle src = fetchTexture(atlas, mTileSize, mCurrentlyDrawnID);
				DrawTextureRec(atlas, src, Vector2{mCells[i][j].rect.x, mCells[i][j].rect.y}, WHITE);
			}
		}

		EndMode2D();
		EndScissorMode();
	}

	void gridInteraction() {
		
		int rowPos = -1;
		int columnPos = -1;
		if (!CheckCollisionPointRec(MousePosition::sMousePos, mViewport)) return ;
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			Vector2 mouse = GetScreenToWorld2D(MousePosition::sMousePos, mCamera);
			int column = (mouse.x) / mTileSize;
			int row = (mouse.y) / mTileSize;
			insertData(row, column);
			//std::cout << "Grid position: " << row << " ; " << column << std::endl;
			
		}


		//zoom controls
		float wheelDelta = GetMouseWheelMove();
		if (!wheelDelta == 0.0f) {

			Vector2 mouseBefore = GetScreenToWorld2D(MousePosition::sMousePos, mCamera);

			//zooming with scroll wheel 
			mCamera.zoom += (float)GetMouseWheelMove() * 0.2f;

			//restricting the zoom amount
			if (mCamera.zoom < 1.5f) {
				mCamera.zoom = 1.5f;
			}
			else if (mCamera.zoom > 7.0f) mCamera.zoom = 7.0f;

			Vector2 mouseAfter = GetScreenToWorld2D(MousePosition::sMousePos, mCamera);

			mCamera.target.x += mouseBefore.x - mouseAfter.x;
			mCamera.target.y += mouseBefore.y - mouseAfter.y;

		}
		
		
		//panning
		//works for both mmd and rmb so its usable on a touchpad too
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
			Vector2 delta = GetMouseDelta();
			mCamera.target.x -= delta.x * 0.3f;
			mCamera.target.y -= delta.y * 0.3f;
		}
		else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
			Vector2 delta = GetMouseDelta();
			mCamera.target.x -= delta.x * 0.5f;
			mCamera.target.y -= delta.y * 0.5f;
		}
	}

	void insertData(int row, int column) {
		if (row < 0 || row >= mRows) return;
		if (column < 0 || column >= mColumns) return;
		mTileData[row][column] = {mCurrentID, mCurrentLayer, mCurrentCollision, mCurrentDamage};
	}

	//the const after arguments says that the function cant change anything
	void toJson(const std::string& fileName) const {
		json givenFile;
		givenFile["gridData"] = mTileData;
		std::ofstream file(fileName);
		file << givenFile.dump(0);
		return;
	}

	void chunkToJson(const std::string& fileName, int x, int y) const {
		Chunk temp;
		temp.x = x;
		temp.y = y;

		for (int row = 0; row < 64; row++) {
			for (int column = 0; column < 64; column++) {
				temp.array[row][column] = mTileData[row][column];
			}
		}
		
		json chunkFile;
		chunkFile["chunkData"] = temp;
		std::ofstream file(fileName);
		file << chunkFile.dump(0);
		return;
	}

};