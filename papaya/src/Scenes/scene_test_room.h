#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Entity.h"
#include "Entities/Player.h"
#include "Entities/Wall.h"
#include "Entities/Dummy.h"

// --- POPRAWIONA KLASA ENEMY ---
// Wklejam j¹ tutaj w ca³oœci, bo zmiany by³y w œrodku metod.
// Upewnij siê, ¿e podmienisz swoj¹ star¹ definicjê Enemy na tê.
class Enemy : public Entity {
public:
	Vector2 mPosition;
	Vector2 mVelocity;
	Vector2 mSize;
	// DODANO: Potrzebne do poprawnej kolizji
	Vector2 mPrevPosition;

	float mSpeed = 40.0f;
	float mGravity = 500.0f;
	int mHealth = 3;

	bool mIsFacingRight = true;
	bool mIsDead = false;

	float mHurtTimer = 0.0f;

	Enemy(float startX, float startY) : Entity({ startX, startY }, ENEMY) {
		mPosition = { startX, startY };
		mPrevPosition = mPosition; // Inicjalizacja
		mVelocity = { mSpeed, 0 };
		mSize = { 16.0f, 16.0f };
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

		// DODANO: Kluczowe dla kolizji!
		mPrevPosition = mPosition;

		mVelocity.y += mGravity * deltaTime;

		mPosition.x += mVelocity.x * deltaTime;
		mPosition.y += mVelocity.y * deltaTime;

		if (mHurtTimer > 0) {
			mHurtTimer -= deltaTime;
		}
	}

	Rectangle getRect() override {
		return { mPosition.x, mPosition.y, mSize.x, mSize.y };
	}

	// --- POPRAWIONA KOLIZJA (Skopiowana logika z Playera) ---
	void onCollision(Entity* pOther) override {
		if (pOther->mType == WALL) {
			Rectangle otherRect = pOther->getRect();

			float prevBottom = mPrevPosition.y + mSize.y;
			float prevRight = mPrevPosition.x + mSize.x;
			float prevLeft = mPrevPosition.x;

			// Pod³oga
			if (prevBottom <= otherRect.y + 5.0f) {
				mPosition.y = otherRect.y - mSize.y;
				mVelocity.y = 0;
			}
			// Œciany boczne (Zawracanie)
			else {
				if (prevRight <= otherRect.x + 5.0f) { // Uderzy³ z lewej
					mPosition.x = otherRect.x - mSize.x;
					mVelocity.x *= -1;
					mIsFacingRight = !mIsFacingRight;
				}
				else if (prevLeft >= otherRect.x + otherRect.width - 5.0f) { // Uderzy³ z prawej
					mPosition.x = otherRect.x + otherRect.width;
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

// --- SCENA ---
class SceneTestRoom {
public:
	std::vector<Entity*> mEntities;
	std::vector<Entity*> walls;
	std::vector<Enemy*> enemies;

	Player* pPlayer;
	Camera2D mCamera = { 0 };

	SceneTestRoom() {
		pPlayer = new Player(30, 100);
		mEntities.push_back(pPlayer);

		mEntities.push_back(new Dummy(180, 128));

		auto addWall = [&](float x, float y, float w, float h) {
			Wall* wObj = new Wall(x, y, w, h);
			mEntities.push_back(wObj);
			walls.push_back(wObj);
			};

		// MAPA
		addWall(0, 160, 320, 40);       // Pod³oga lewa
		addWall(100, 110, 80, 10);      // Platforma œrodkowa
		addWall(220, 70, 400, 11110);   // Wielka œciana prawa (zaczyna siê na X=220)
		addWall(0, 0, 20, 200);

		// WROGOWIE
		// Wróg 1: Nad lew¹ pod³og¹ (OK)
		enemies.push_back(new Enemy(50, 100));

		// Wróg 2: POPRAWIONY SPAWN
		// Musi byæ na lewo od X=220. Dajemy go na X=180, nad Dummy.
		enemies.push_back(new Enemy(180, 50));

		mCamera.target = { pPlayer->mPosition.x, pPlayer->mPosition.y };
		mCamera.offset = { 160.0f, 90.0f };
		mCamera.rotation = 0.0f;
		mCamera.zoom = 1.0f;
	}

	~SceneTestRoom() {
		for (Entity* e : mEntities) delete e;
		mEntities.clear();
		walls.clear();
		for (Enemy* e : enemies) delete e;
		enemies.clear();
	}

	void Update(float deltaTime) {
		for (Entity* pEntity : mEntities) {
			if (pEntity->mActive) {
				pEntity->update(deltaTime);
			}
		}

		// LOGIKA WROGÓW
		for (int i = 0; i < enemies.size(); i++) {
			Enemy* e = enemies[i];

			// 1. Fizyka (Tu dzia³a nowa grawitacja i mPrevPosition)
			e->update(deltaTime);

			// 2. Kolizje (Tu dzia³a nowa funkcja onCollision)
			for (auto& wall : walls) {
				if (CheckCollisionRecs(e->getRect(), wall->getRect())) {
					e->onCollision(wall);
				}
			}

			if (e->mIsDead) {
				delete e;
				enemies.erase(enemies.begin() + i);
				i--;
			}
		}

		// KOLIZJA GRACZ VS WRÓG
		for (Enemy* e : enemies) {
			if (!e->mIsDead && CheckCollisionRecs(pPlayer->getRect(), e->getRect())) {

				if (pPlayer->mInvincibilityTimer <= 0) {
					pPlayer->mHealth--;

					// Obliczanie kierunku odrzutu
					float playerCenterX = pPlayer->mPosition.x + pPlayer->mSize.x / 2.0f;
					float enemyCenterX = e->mPosition.x + e->mSize.x / 2.0f;
					float recoilDir = (playerCenterX < enemyCenterX) ? -1.0f : 1.0f;

					pPlayer->mVelocity.x = recoilDir * 300.0f;
					pPlayer->mVelocity.y = -150.0f;

					pPlayer->mRecoilTimer = 0.2f;

					pPlayer->mInvincibilityTimer = 1.0f;

					pPlayer->mIsDashing = false;
					pPlayer->mIsAttacking = false;
				}
			}
		}

		for (auto it1 = mEntities.begin(); it1 != mEntities.end(); ++it1) {
			for (auto it2 = mEntities.begin(); it2 != mEntities.end(); ++it2) {
				Entity* pA = *it1;
				Entity* pB = *it2;
				if (pA == pB) continue;
				if (!pA->mActive || !pB->mActive) continue;

				if (CheckCollisionRecs(pA->getRect(), pB->getRect())) {
					pA->onCollision(pB);
				}
			}
		}

		if (pPlayer->mIsAttacking) {
			for (Entity* pEntity : mEntities) handlePlayerHit(pPlayer, pEntity);
			for (Enemy* pEnemy : enemies) handlePlayerHit(pPlayer, pEnemy);
		}

		if (pPlayer->mPosition.y > 500) {
			pPlayer->mPosition = { 30,100 };
			pPlayer->mVelocity = { 0,0 };
		}

		if (pPlayer) {
			pPlayer->updateAnimation(deltaTime);
			mCamera.target.x = floor(pPlayer->mPosition.x);
			mCamera.target.y = floor(pPlayer->mPosition.y);
		}
	}

	void Draw() {
		ClearBackground(DARKGRAY);

		BeginMode2D(mCamera);
		for (Entity* pEntity : mEntities) {
			if (pEntity->mActive) pEntity->draw();
		}
		for (Enemy* e : enemies) {
			e->draw();
		}
		EndMode2D();

		DrawText("ARROWS - Move / Climb", 10, 10, 10, WHITE);
		DrawText("SPACE - Jump / Wall Jump", 10, 25, 10, WHITE);
		DrawText("C - Dash", 10, 40, 10, WHITE);
		DrawText("Z - Attack (Try Down + Z)", 10, 55, 10, WHITE);
		//DrawText("X - Spirit Chakram", 10, 70, 10, WHITE);
		//DrawText("1/2/3 - Change Weapon", 10, 85, 10, WHITE);
		DrawText(TextFormat("HP : %d",pPlayer->mHealth), 10, 85, 10, WHITE);
	}

private:
	void handlePlayerHit(Player* pPlayer, Entity* target) {
		if (target == pPlayer || target->mType == WALL) return;

		if (CheckCollisionRecs(pPlayer->mAttackArea, target->getRect())) {
			bool alreadyHit = false;
			for (Entity* hit : pPlayer->mHitEntities) {
				if (hit == target) { alreadyHit = true; break; }
			}
			if (alreadyHit) return;

			Dummy* pDummy = dynamic_cast<Dummy*>(target);
			if (pDummy && !pDummy->mIsHit) pDummy->takeDamage();

			Enemy* pEnemy = dynamic_cast<Enemy*>(target);
			if (pEnemy) pEnemy->takeDamage(1);

			pPlayer->mHitEntities.push_back(target);
			WeaponStats stats = pPlayer->getCurrentWeaponStats();

			if (pPlayer->mAttackDir == 2) {
				pPlayer->mVelocity.y = stats.pogoForce;
				pPlayer->mIsPogoJump = true;
				pPlayer->mIsJumping = false;
				pPlayer->mJumpBufferCounter = 0;
				pPlayer->mIsDashing = false;
				pPlayer->mCanDash = true;
			}

			if (pPlayer->mAttackDir == 0) {
				float playerCenterX = pPlayer->mPosition.x + pPlayer->mSize.x / 2.0f;
				float enemyCenterX = target->getRect().x + target->getRect().width / 2.0f;
				float recoilDir = (playerCenterX < enemyCenterX) ? -1.0f : 1.0f;

				pPlayer->mVelocity.x = recoilDir * stats.recoilSelf;
				if (pPlayer->mIsGrounded) {
					pPlayer->mVelocity.y = -25.0f;
					pPlayer->mIsGrounded = false;
				}
				pPlayer->mRecoilTimer = 0.20f;
				pPlayer->mIsDashing = false;
				pPlayer->mCanDash = true;
			}
		}
	}
};