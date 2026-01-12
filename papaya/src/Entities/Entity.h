#pragma once
#include "raylib.h"
#include <vector>

enum EntityType { PLAYER, WALL, BULLET, ENEMY };

class Entity {
public:
	Vector2 mPosition;
	EntityType mType;
	bool mActive = true;
	int mHealth = 1;

	// konstruktor
	Entity(Vector2 pos, EntityType t) : mPosition(pos), mType(t) {}
	
	virtual ~Entity() {} // wirtualny destruktor

	virtual void update(float deltaTime) = 0;
	virtual void draw() = 0;
	virtual Rectangle getRect() = 0;
	
	virtual void onCollision(Entity* other) = 0; // odpowiednik on_hit
};