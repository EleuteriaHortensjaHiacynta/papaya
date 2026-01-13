#pragma once
#include "Entity.h"

class Wall : public Entity {
public:
    bool mCollidable = true;   // <-- TO MUSI BYÆ
    bool mDamaging = false;    // <-- TO MUSI BYÆ

    Wall(float x, float y, float w, float h) : Entity({ x,y }, WALL) {
        mPosition = { x, y };
        mSize = { w, h };
    }

    Wall(float x, float y, float w, float h, bool collidable, bool damaging)
        : Entity({ x, y }, WALL) {
        mPosition = { x, y };
        mSize = { w, h };
        mCollidable = collidable;
        mDamaging = damaging;
    }

    void update(float deltaTime) override {}

    Rectangle getRect() const override {
        return { mPosition.x, mPosition.y, mSize.x, mSize.y };
    }

    void draw() const override {
        // Opcjonalnie rysuj œcianê
        // DrawRectangleRec(getRect(), DARKGRAY);
    }

    void onCollision(Entity* other) override {}
};