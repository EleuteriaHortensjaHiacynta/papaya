#pragma once
#include "Entity.h"

class Wall : public Entity {
public:
	Vector2 size;

	Wall(float x, float y, float w, float h) : Entity({ x,y }, WALL) {
		size = { w,h };
	}

	void Update(float deltaTime) override {
	} // welp œciana œcianuje

	void Draw() override {
		DrawRectangleV(position, size, GRAY);
	}

	Rectangle GetRect() override {
		return { position.x, position.y, size.x, size.y };
	}

	void OnCollision(Entity* other) override {
		// chwilowo œciany ignoruj¹ obra¿enia
	}
};