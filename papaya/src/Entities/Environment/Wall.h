#pragma once
#include "Entities/Entity.h"

class Wall : public Entity {
public:
	Wall(float x, float y, float w, float h) : Entity({ x,y }, WALL) {
		mSize = { w,h };
	}

	void update(float deltaTime) override {
	} // welp �ciana �cianuje

	void draw() override {
		DrawRectangleV(mPosition, mSize, GRAY);
	}

	Rectangle getRect() override {
		return { mPosition.x, mPosition.y, mSize.x, mSize.y };
	}

	void onCollision(Entity* other) override {
		// chwilowo �ciany ignoruj� obra�enia
	}
};