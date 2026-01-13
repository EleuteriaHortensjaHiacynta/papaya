#pragma once
#include "raylib.h"
#include <vector>

enum EntityType { PLAYER, WALL, BULLET, ENEMY };

class Entity {
public:
	Vector2 mPosition;
	Vector2 mSize;
	EntityType mType;
	bool mActive = true;

	int mID = -1;

	// konstruktor
	Entity(Vector2 pos, EntityType t) : mPosition(pos), mType(t), mSize({ 0,0 }) {}
	
	virtual ~Entity() {} // wirtualny destruktor

	virtual void update(float deltaTime) = 0;
	virtual void draw() const {};
	
	virtual Rectangle getRect() const {
		return { mPosition.x, mPosition.y, mSize.x, mSize.y };
	}

	virtual void onCollision(Entity* other) = 0; // odpowiednik on_hit
};