#pragma once
#include "raylib.h"
#include <vector>

enum EntityType {
	PLAYER,
	WALL,
	BULLET,
	ENEMY
};

class Entity {
public:
	EntityType type;
	Vector2 position;
	bool active = true;

	// konstruktor
	Entity(Vector2 pos, EntityType t) : position(pos), type(t) {}

	// wirtualny destruktor
	virtual ~Entity() {}

	virtual void Update(float deltaTime) = 0;
	virtual void Draw() = 0;
	virtual Rectangle GetRect() = 0;

	// odpowiednik on_hit
	virtual void OnCollision(Entity* other) = 0;
};