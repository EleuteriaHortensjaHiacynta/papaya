#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Player.h"
#include "Entities/Wall.h"
#include "Entities/Dummy.h"

class SceneTestRoom {
public:
	std::vector<Entity*> mEntities;
	Player* pPlayer; // wskaŸnik na gracza (przyda siê do kamery)

	Camera2D mCamera = { 0 };

	// Konstruktor
	SceneTestRoom() {
		// inicjalizacja gracza (do pozycji startowej)
		pPlayer = new Player(30, 100);
		mEntities.push_back(pPlayer); // dodajemy gracza do gry

		// incicjalizacja dummy
		mEntities.push_back(new Dummy(180, 128));

		// TWORZENIE POZIOMU
		mEntities.push_back(new Wall(0, 160, 320, 40)); // pod³oga
		mEntities.push_back(new Wall(100, 110, 80, 10)); // lewa platforma
		mEntities.push_back(new Wall(220, 70, 60, 10)); // prawa platforma

		// konfiguracja kamery 
		mCamera.target = { pPlayer->mPosition.x, pPlayer->mPosition.y };
		mCamera.offset = { 160.0f, 90.0f };
		mCamera.rotation = 0.0f;
		mCamera.zoom = 1.0f;
	}

	// Destruktor
	~SceneTestRoom() {
		for (Entity* e : mEntities) {
			delete e;
		}
		mEntities.clear();
	}

	void Update(float deltaTime) {

		// Update wszystkich entity
		for (Entity* pEntity : mEntities) {
			if (pEntity->mActive) {
				pEntity->update(deltaTime);
			}
		}


		// Detekcja kolizji dla wszystkich entity
		for (auto it1 = mEntities.begin(); it1 != mEntities.end(); ++it1) {
			for (auto it2 = mEntities.begin(); it2 != mEntities.end(); ++it2) {
				Entity* pA = *it1;
				Entity* pB = *it2;

				if (pA == pB) continue; // nie sprawdzamy samych ze sob¹
				if (!pA->mActive || !pB->mActive) continue; // niesprawdzamy nieaktywnych

				if (CheckCollisionRecs(pA->getRect(), pB->getRect())) {
					pA->onCollision(pB); // na odwrót { b -> OC(a); } i tak siê wykona przez pêtle 
				}
			}
		}

		// POGO (uderzenie przeciwnika jak gracz atakiem w dó³, sprawia, ¿e gracz siê odbija
		if (pPlayer->mIsAttacking) {
			for (Entity* pEntity : mEntities) {
				if (pEntity != pPlayer && pEntity->mType != WALL) {
					if (CheckCollisionRecs(pPlayer->mAttackArea, pEntity->getRect())) {

						bool alreadyHit = false;
						for (Entity* hit : pPlayer->mHitEntities) {
							if (hit == pEntity) { alreadyHit = true; break; }
						}
						if (alreadyHit) continue;

						Dummy* pEnemy = dynamic_cast<Dummy*>(pEntity);
						if (pEnemy && !pEnemy->mIsHit) {
							
							pEnemy->takeDamage();
							pPlayer->mHitEntities.push_back(pEntity);

							// (POGO) Atak w dó³							
							if (pPlayer->mAttackDir == 2) {
								pPlayer->mVelocity.y = -150.0f; // odbicie w górê

								pPlayer->mIsPogoJump = true;
								pPlayer->mIsJumping = false;

								pPlayer->mJumpBufferCounter = 0;

								pPlayer->mIsDashing = false;
								pPlayer->mDashTimer = 0.0f;
								pPlayer->mCanDash = true;
							}
							
							// (Odrzut) Atak w bok
							if (pPlayer->mAttackDir == 0) {

								float playerCenterX = pPlayer->mPosition.x + pPlayer->mSize.x / 2.0f;
								float enemyCenterX = pEnemy->mPosition.x + 16.0f / 2.0f;

								float recoilDir = (playerCenterX < enemyCenterX) ? -1.0f : 1.0f;

								float recoilForceX = 90.0f; // lekkie odbicie do boku
								float recoilForceY = 0.0f;

								pPlayer->mVelocity.x = recoilDir * recoilForceX;

								pPlayer->mVelocity.y = recoilForceY;
								pPlayer->mIsGrounded = false;

								// Aktywujemy timer, ¿eby player.h wiedzia³, ¿e ma siê "œlizgaæ" 
								pPlayer->mRecoilTimer = 0.25f;

								pPlayer->mIsDashing = false;
								pPlayer->mCanDash = true;
							}
						}
					}
				}
			}
		}


		// reset gracza do pozycji wyjœciowej po wypadniêciu za mapê
		if (pPlayer->mPosition.y > 250) {
			pPlayer->mPosition = { 30,100 };
			pPlayer->mVelocity = { 0,0 };
		}

		if (pPlayer) {
			pPlayer->updateAnimation(deltaTime);

			// kamera
			float targetX = pPlayer->mPosition.x;
			float targetY = pPlayer->mPosition.y;

			mCamera.target.x = floor(targetX);
			mCamera.target.y = floor(targetY);
		}
	}

	void Draw() {
		// rysowanie t³a
		ClearBackground(BLACK);

		// tryb kamery 2D
		BeginMode2D(mCamera);

			for (Entity* pEntity : mEntities) {
				if (pEntity->mActive) pEntity->draw();
			}

		EndMode2D();

		// Debug info
		DrawText("Sterowanie: arrow keys, \nX - sword swing,\nZ - dash,\nSPACE - jump", 5, 5, 5, DARKGRAY);
		DrawText("Zielone pole -> training dummy", 5, 160, 5, GREEN);
		DrawText("Polecam przetesowac:\nSuperdash\nSkakanie po przeciwniku (POGO)", 150, 5, 5, DARKBLUE);
		//DrawText(TextFormat("Pos: %.0f, %.0f", pPlayer->mPosition.x, pPlayer->mPosition.y), 10, 40, 20, LIGHTGRAY);
		//DrawText(TextFormat("State: %d (0=IDLE, 1=WALK, 2=J_UP)", pPlayer->mCurrentState), 10, 60, 20, GREEN);
		//DrawText(TextFormat("Grounded: %d", pPlayer->mIsGrounded), 10, 80, 20, GREEN);
		//DrawText(TextFormat("Vel Y: %.2f", pPlayer->mVelocity.y), 10, 100, 20, GREEN);
		//DrawText(TextFormat("Frame: %d / %d", pPlayer->mCurrentFrameRelative, pPlayer->mLengthFrames), 10, 120, 20, GREEN);

		//DrawText(TextFormat("Attack Dir: %d (0=Side, 1=Up, 2=Down)", pPlayer->mAttackDir), 10, 140, 20, YELLOW);
		//DrawText(TextFormat("Current Frame: %d", pPlayer->mStartFrame + pPlayer->mCurrentFrameRelative), 10, 160, 20, YELLOW);
	}
};