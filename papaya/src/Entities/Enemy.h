#pragma once
#include "Entities/Entity.h"

class Enemy : public Entity {
public:
	Vector2 mPosition;
	Vector2 mVelocity;
	Vector2 mSize;

	float mSpeed = 40.0f;
	float mGravity = 500.0f;
	//int mMaxHealth = 3;
	int mHealth = 3;

	bool mIsFacingRight = true;
	bool mIsDead = false;

	float mHurtTimer = 0.0f;

	Enemy(float startX, float startY) : Entity({ startX,startY }, ENEMY) {
		mPosition = { startX,startY };
		mVelocity = { mSpeed, 0 };
		mSize = { 16.0f,16.0f };
	}

	void takeDamage(int amount) {
		mHealth -= amount;
		mHurtTimer = 0.2f;

		mVelocity.y = -150.0f;

		if (mHealth <= 0) {
			mIsDead = true;
		}
	}

	void update(float deltaTime) override {
		if (mIsDead) return;

		mVelocity.x += mVelocity.x * deltaTime;
		mVelocity.y += mVelocity.y * deltaTime;

		if (mHurtTimer > 0) {
			mHurtTimer -= deltaTime;
		}

	}

	Rectangle getRect() override{
		return { mPosition.x, mPosition.y, mSize.x, mSize.y };
	}

	void onCollision(Entity* pOther) override{
		if (pOther->mType = WALL) {
			Rectangle otherRect = pOther->getRect();

			// kolizja pionowa
			if (mPosition.y + mSize.y <= otherRect.y + 5.0f && mVelocity.y >= 0) {
				mPosition.y = otherRect.y - mSize.y;
				mVelocity.y = 0;
			}
			// kolizja pozioma
			else {
				bool hitLeft = (mPosition.x + mSize.x > otherRect.x && mPosition.x < otherRect.x);
				bool hitRight = (mPosition.x < otherRect.x + otherRect.width && mPosition.x + mSize.x > otherRect.x + otherRect.width);
				
				if (hitLeft || hitRight) {
					if (mVelocity.x > 0) mPosition.x = otherRect.x - mSize.x;
					else mPosition.x = otherRect.x + otherRect.width;

					mVelocity.x *= -1;
					mIsFacingRight = !mIsFacingRight;
				}
			}
		}
	}

	void draw() override {
		if (mIsDead) return;

		Color enemyColor = (mHurtTimer > 0) ? WHITE : GREEN;

		DrawRectangle((int)mPosition.x, (int)mPosition.y, (int)mSize.x, (int)mSize.y, enemyColor);
		
		float eyeOffset = mIsFacingRight ? 8.0f : 2.0f;
		DrawRectangle((int)mPosition.x + eyeOffset, (int)mPosition.y + 4, 4, 4, BLACK);
	}
};