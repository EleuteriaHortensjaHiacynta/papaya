#pragma once
#include <raylib.h>
#include <functional>
#include <tuple>
#include <utility>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <memory>
#include "external_headers/json.hpp"

using json = nlohmann::json;

inline Rectangle fetchTexture(Texture2D atlas, int tileSize, int index)	 {
	int tileX = index % (atlas.width / tileSize);
	int tileY = index / (atlas.width / tileSize);

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


	//for scrolling widgets
	virtual float getHeight() const { return 0.0f; }
	virtual float getWidth() const { return 0.0f; }

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
		//std::cout << sMousePos.x << " " << sMousePos.y << std::endl;
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
class Grid : public Widget {
private:
	const int mRows;
	const int mColumns;
	int rowHeight;
	int columnWidth;

	//from what i read there's no need to use delete later 
	// because this pointer doesnt own the parent grid so it shouldnt even delete it
	Grid* mpParentGrid = nullptr;


public:

	std::vector<std::vector<Cell>> cells;
	// top left corner is the origin of the grid, everything gets set relative to it
	int originX;
	int originY;

	int selectedColumn = -1;
	int selectedRow = -1;
	Rectangle selectedCell = {-1, -1, -1, -1};


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

	void selectCell( int tileSize, int index) {
		selectedColumn = index % mColumns;
		selectedRow = index / mColumns;
		if (selectedRow >= mRows) selectedRow = mRows - 1;
		if (selectedColumn >= mColumns) selectedColumn = mColumns - 1;
		//selectedCell.x = originX + selectedColumn * columnWidth;
		//selectedCell.y = originX + selectedRow * rowHeight;
		//selectedCell.width = columnWidth;
		//selectedCell.height = rowHeight;
		selectedCell = cells[selectedRow][selectedColumn].rect;
		selectedCell.x -= 1;
		selectedCell.y -= 1;
	}

	void drawSelectedCell() {
		DrawRectangleLinesEx(selectedCell, 2, WHITE);
	}

	//loops through the grid and calls the draw function on every widget inside
	//if there is no widget it draws a gray square border instead
	void draw() override {
		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {

				/*Rectangle originOffset = cells[i][j].rect;
				originOffset.x += originX;
				originOffset.y += originY;*/

				if (cells[i][j].widget != nullptr) {
					//cells[i][j].widget->setPosition(originOffset);
					cells[i][j].widget->draw();
				}
				else {
					DrawRectangleLinesEx(cells[i][j].rect, 1, GRAY);
				}
			}
		}
	}

	//changes the origin of the grid and readjusts the cells
	void setPosition(Rectangle rect) override {
		originX = rect.x;
		originY = rect.y;
		giveCoordinates(originX, originY);
	}

	float getHeight() const override {
		return mRows * (rowHeight);
	}

	float getWidth() const override {
		return mColumns * columnWidth;
	}

};

class ScrollContainer : public Widget {
private:
	Rectangle mRect;
	std::shared_ptr<Widget> mChild;
	std::shared_ptr<Grid> mChildGrid;

	float mScrollY = 0.0f;
	float mScrollSpeed = 20.f;
	float mScrollX = 0.0f;


public:

	Camera2D camera;

	ScrollContainer(Rectangle rect)
		: mRect(rect) {

	};

	void setCamera() {
		camera.target = { mScrollX, mScrollY };
		camera.offset = { mRect.x , 0 };
		camera.zoom = 1.0f;
		camera.rotation = 0.0f;

	}

	Rectangle getRect() {
		return mRect;
	}

	void setRect(Rectangle rect) {
		mRect = rect;
	}

private:

	void clampScroll() {
		if (!mChild) return;

		float contentHeight = mChild->getHeight();
		float contentWidth = mChild->getWidth();
		float maxScrollY = std::max(0.0f, contentHeight - mRect.height);
		float maxScrollX = std::max(0.0f, contentWidth - mRect.width);

		if (mScrollY < 0) mScrollY = 0;
		if (mScrollY > maxScrollY) mScrollY = maxScrollY;

		if (mScrollX < 0) mScrollX = 0;
		if (mScrollX > maxScrollX) mScrollX = maxScrollX;
	}


public:

	void setChild(std::shared_ptr<Widget> child) {
		mChild = child;
	}

	void setGridChild(std::shared_ptr<Grid> grid) {
		mChildGrid = grid;
	}

	void setPosition(Rectangle rect) override {
		mRect = rect;
	}

	void draw() override {

		bool mouseInside = CheckCollisionPointRec(MousePosition::sMousePos, mRect);
		if (mouseInside) {
			if (IsKeyDown(KEY_LEFT_SHIFT)) {
				float wheel = GetMouseWheelMove();
				mScrollX -= wheel * mScrollSpeed;
			}
			else {
				float wheel = GetMouseWheelMove();
				mScrollY -= wheel * mScrollSpeed;
			}
		}

		clampScroll();
		setCamera();

		BeginScissorMode((int)mRect.x, (int)mRect.y, (int)mRect.width, (int)mRect.height);
		BeginMode2D(camera);
		DrawRectangle(mRect.x, mRect.y, mChild->getWidth(), mChild->getWidth(), SKYBLUE);
		if (mChild) {
			Vector2 prevMouse = MousePosition::sMousePos;
			if (mouseInside) {
				MousePosition::sMousePos = GetScreenToWorld2D(prevMouse, camera);
			}
			else {
				//this is simultaneously the smartest and dumbest way to avoid interacting with buttons that arent visible 
				MousePosition::sMousePos = { -10000000000.0f, -10000000000.0f };
			}
			
			mChild->draw();
			if (mChildGrid) {
				mChildGrid->drawSelectedCell();
			}
			
			MousePosition::sMousePos = prevMouse;
		}
		EndMode2D();
		EndScissorMode();
	}



};


struct Line {
	Vector2 p1;
	Vector2 p2;
};

struct TileData {
	int textureID;
	//not needed anymore
	//int layer;
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
		//{"layer", t.layer},
		{"collision", t.collision},
		{"damage", t.damage}
	};
}

void from_json(const json& j, TileData& t) {
	j.at("textureID").get_to(t.textureID);
	//j.at("layer").get_to(t.layer);
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

	bool collisionOverlayEnabled = false;
	bool damageOverlayEnabled = false;

	std::vector<std::vector<Cell>> cells;
	// top left corner is the origin of the grid, everything gets set relative to it
	int originX;
	int originY;
	int tileSize;
	int gridWidth;
	int gridHeight;
	int screenWidth;
	int screenHeight;
	Rectangle viewport;

	//tile variables
	int currentlyDrawnID;
	int currentID;
	//int currentLayer;
	bool currentCollision;
	bool currentDamage;

	//used for visualising the center of cells in the grid without textures
	std::vector<Line> verticalLines;
	std::vector<Line> horizontalLines;

	std::vector< std::vector < TileData > > tileData;

	InteractiveGrid(int passedRows,
		int passedColumns,
		int gridOriginX,
		int gridOriginY,
		int tileSize,
		int screenWidth,
		int screenHeight)
		: mRows(passedRows),
		mColumns(passedColumns),
		cells(mRows, std::vector<Cell>(mColumns)),
		tileData(mRows, std::vector<TileData>(mColumns)),
		verticalLines(mColumns + 1),
		horizontalLines(mRows + 1),
		tileSize(tileSize),
		originX(gridOriginX),
		originY(gridOriginY),
		screenHeight(screenHeight),
		screenWidth(screenWidth)

	{
		currentID = 1;
		//currentLayer = 0;
		currentCollision = false;
		currentDamage = false;

		giveCoordinates(originX, originY);
		createLines();

		mCamera.target = { 0, 0 };
		//mCamera.offset = { viewport.x + gridWidth / 2.0f, viewport.y + gridHeight / 2.0f };
		mCamera.offset = { viewport.x, viewport.y };
		mCamera.rotation = 0.0f;
		mCamera.zoom = 1.5f;

	}

	float getCameraZoom() {
		return mCamera.zoom;
	}

	void giveCoordinates(int xPos, int yPos) {

		gridHeight = mRows * tileSize;
		gridWidth = mColumns * tileSize;

		viewport = { (float)xPos, (float)yPos, (float)screenWidth, (float)screenHeight };

		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				cells[i][j].rect = {
					(float)(j * tileSize),
					(float)(i * tileSize),
					(float)tileSize,
					(float)tileSize
				};
				tileData[i][j] = { 0, 0, 0 };
			}
		}
	}

	void createLines() {
		for (int i = 0; i < mColumns + 1; i++) {
			float pX = /*tileSize / 2 + */i * tileSize;
			float p1Y = 0;
			float p2Y = mRows * tileSize;

			verticalLines[i].p1 = { pX, p1Y };
			verticalLines[i].p2 = { pX, p2Y };

		}

		for (int i = 0; i < mRows + 1; i++) {
			float pY = /*tileSize / 2 +*/ i * tileSize;
			float p1X = 0;
			float p2X = mColumns * tileSize;
			horizontalLines[i].p1 = { p1X ,pY };
			horizontalLines[i].p2 = { p2X, pY };
		}
	}

	void renderLines() {
		BeginScissorMode(viewport.x, viewport.y, viewport.width, viewport.height);
		BeginMode2D(mCamera);
		for (int i = 0; i < mColumns + 1; i++) {
			DrawLineV(verticalLines[i].p1, verticalLines[i].p2, WHITE);
			if(i % 4 == 0) DrawLineV(verticalLines[i].p1, verticalLines[i].p2, DARKGRAY);
			if (i % 16 == 0) DrawLineV(verticalLines[i].p1, verticalLines[i].p2, GREEN);
		}
		for (int i = 0; i < mRows + 1; i++) {
			DrawLineV(horizontalLines[i].p1, horizontalLines[i].p2, WHITE);
			if (i % 4 == 0) DrawLineV(horizontalLines[i].p1, horizontalLines[i].p2, DARKGRAY);
			if (i % 16 == 0) DrawLineV(horizontalLines[i].p1, horizontalLines[i].p2, GREEN);
		}
		EndMode2D();
		EndScissorMode();
	}

	void draw(Texture2D atlas) {
		BeginScissorMode(viewport.x, viewport.y, viewport.width, viewport.height);
		BeginMode2D(mCamera);

		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				currentlyDrawnID = tileData[i][j].textureID;
				Rectangle src = fetchTexture(atlas, tileSize, currentlyDrawnID);
				DrawTextureRec(atlas, src, Vector2{ cells[i][j].rect.x, cells[i][j].rect.y }, WHITE);
			}
		}

		if (collisionOverlayEnabled) {
			for (int i = 0; i < mRows; i++) {
				for (int j = 0; j < mColumns; j++) {
					if (tileData[i][j].collision) {
						DrawRectangle(cells[i][j].rect.x, cells[i][j].rect.y, tileSize / 2, tileSize / 2, WHITE);
					}
					
				}
			}
		}

		if (damageOverlayEnabled) {
			for (int i = 0; i < mRows; i++) {
				for (int j = 0; j < mColumns; j++) {
					if (tileData[i][j].damage) {
						DrawRectangle(cells[i][j].rect.x + (tileSize / 2), cells[i][j].rect.y + (tileSize / 2), tileSize / 2, tileSize / 2, LIGHTGRAY);
					}

				}
			}
		}

		EndMode2D();
		EndScissorMode();
	}

	void drawToImage(Texture2D atlas) {
		for (int i = 0; i < mRows; i++) {
			for (int j = 0; j < mColumns; j++) {
				currentlyDrawnID = tileData[i][j].textureID;
				Rectangle src = fetchTexture(atlas, tileSize, currentlyDrawnID);
				DrawTextureRec(atlas, src, Vector2{ cells[i][j].rect.x, cells[i][j].rect.y }, WHITE);
			}
		}
	}

	Vector2 hoveredCell() {
		Vector2 mouse = GetScreenToWorld2D(MousePosition::sMousePos, mCamera);
		Vector2 pos;
		pos.x = (mouse.x) / tileSize;
		pos.y = (mouse.y) / tileSize;
		return pos;
	}

	void insertData(int row, int column) {
		if (row < 0 || row >= mRows) return;
		if (column < 0 || column >= mColumns) return;
		tileData[row][column] = { currentID/*, currentLayer*/, currentCollision, currentDamage };
	}

	void gridInteraction() {

		int rowPos = -1;
		int columnPos = -1;
		if (!CheckCollisionPointRec(MousePosition::sMousePos, viewport)) return;
		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
			Vector2 position = hoveredCell();
			int column = position.x;
			int row = position.y;
			insertData(row, column);
			//std::cout << "Grid position: " << row << " ; " << column << std::endl;
		}
		// lmb is used as erase mode
		else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !IsKeyDown(KEY_LEFT_SHIFT)) {
			Vector2 position = hoveredCell();
			int column = position.x;
			int row = position.y;
			if (row < 0 || row >= mRows) return;
			if (column < 0 || column >= mColumns) return;
			tileData[row][column] = { 0, 0, 0};
		}


		//zoom controls
		float wheelDelta = GetMouseWheelMove();
		if (wheelDelta != 0.0f) {

			Vector2 mouseBefore = GetScreenToWorld2D(MousePosition::sMousePos, mCamera);

			//zooming with scroll wheel 
			mCamera.zoom += (float)GetMouseWheelMove() * 0.2f;

			//restricting the zoom amount
			if (mCamera.zoom < 1.5f) {
				mCamera.zoom = 1.5f;
			}
			else if (mCamera.zoom > 15.0f) mCamera.zoom = 15.0f;

			Vector2 mouseAfter = GetScreenToWorld2D(MousePosition::sMousePos, mCamera);

			mCamera.target.x += mouseBefore.x - mouseAfter.x;
			mCamera.target.y += mouseBefore.y - mouseAfter.y;

		}


		//panning
		//works for both mmd and rmb + shift so its usable on a touchpad too
		if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && IsKeyDown(KEY_LEFT_SHIFT)) {
			Vector2 delta = GetMouseDelta();
			mCamera.target.x -= delta.x / mCamera.zoom;
			mCamera.target.y -= delta.y / mCamera.zoom;
		}
		else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
			Vector2 delta = GetMouseDelta();
			mCamera.target.x -= delta.x / mCamera.zoom;
			mCamera.target.y -= delta.y / mCamera.zoom;
		}
	}

	//the const after arguments says that the function cant change anything
	void toJson(const std::string& fileName) const {
		json givenFile;
		givenFile["gridData"] = tileData;
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
				temp.array[row][column] = tileData[row][column];
			}
		}

		json chunkFile;
		chunkFile["chunkData"] = temp;
		std::ofstream file(fileName);
		file << chunkFile.dump(0);
		return;
	}

	void jsonToChunk(const std::string& fileName, int &x, int &y) {
		std::ifstream chunkFile(fileName);

		if (!chunkFile.is_open()) return;

		json chunk;
		chunkFile >> chunk;
		chunkFile.close();

		if (!chunk.contains("chunkData") || !chunk["chunkData"].contains("array") || !chunk["chunkData"].contains("x") || !chunk["chunkData"].contains("y") ) return;

		auto& arrayData = chunk["chunkData"]["array"];

		for (int row = 0; row < 64; row++) {
			for (int column = 0; column < 64; column++) {
				tileData[row][column] = arrayData[row][column].get<TileData>();
			}
		}

		x = chunk["chunkData"]["x"].get<int>();
		y = chunk["chunkData"]["y"].get<int>();

	}

};