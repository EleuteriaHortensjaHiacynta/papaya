#include "Entities/Entity.h"

class Dummy : public Entity {
public:
	Vector2 mPosition;
	bool mIsHit = false;
	float mHitTimer = 0.0f; // jak d³ugo œwieci siê na czerwono po uderzeniu

	Dummy(float x, float y) : Entity({ x,y }, ENEMY) {
		mPosition = { x,y };
	}

	void update(float deltaTime) override {
		if (mIsHit) {
			mHitTimer -= deltaTime;
			if (mHitTimer <= 0) mIsHit = false;
		}
	}

	Rectangle getRect() override {
		return { mPosition.x, mPosition.y, 16, 32 }; //16x32
	}

	void draw() override {
		Color color = mIsHit ? RED : GREEN;
		DrawRectangleRec(getRect(), color);
	}

	void takeDamage() {
		mIsHit = true;
		mHitTimer = 0.2f;
	}
	
	void onCollision(Entity* other) override {

	}
};