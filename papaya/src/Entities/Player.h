#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Entity.h"

inline float approach(float current, float target, float increase) {
	if (current < target) {
		return (current + increase > target) ? target : current + increase;
	}
	else {
		return (current - increase < target) ? target : current - increase;
	}
}

struct Ghost {
	Vector2 position;
	Rectangle frameRec;
	bool facingRight;
	float alpha;
};

enum AnimState {
	IDLE, WALK, JUMP_UP, JUMP_DOWN, TURN, ATTACK, DASH
};

class Player : public Entity {
public:
	Vector2 mPosition;
	Vector2 mPrevPosition;
	Vector2 mVelocity;
	Vector2 mSize;

	// Fizyka
	float mMaxSpeed = 100.0f;
	float mAcceleration = 1110.0f;
	float mFriction = 990.0f;
	float mGravity = 500.0f;
	float mJumpForce = 250.0f;
	float mRecoilFriction = 20.0f;

	// Dash
	float mDashTimer = 0.0f;
	float mDashDuration = 0.2f;
	float mMomentumTimer = 0.0f;
	float mDashSpeed = 400.0f;
	bool mIsDashing = false;
	bool mCanDash = true;


	// Timery
	float mJumpBufferTime = 0.1f;
	float mCoyoteTime = 0.15f;
	float mJumpBufferCounter = 0.0f;
	float mCoyoteTimeCounter = 0.0f;
	float mRecoilTimer = 0.0f;

	// Walka
	float mAttackTimer = 0.0f;
	float mAttackDuration = 0.33f;
	bool mIsAttacking = false;
	int mAttackDir = 0; // 0 - boki, 1 - góra, 2 - dó³
	int mAttackDamage = 1;
	Rectangle mAttackArea = { 0,0,0,0 };


	// Stan
	bool mIsGrounded = false;
	bool mIsFacingRight = true;
	bool mIsJumping = false;
	bool mIsPogoJump = false;

	AnimState mCurrentState = IDLE;

	// GRAFIKA I ANIMACJA
	Texture2D mTexture; // sprite sheet
	Rectangle mFrameRec;

	Texture2D mSlashTexture; // Assets/slash.png
	float mSlashTimer = 0.0f;

	int mStartFrame = 0;
	int mLengthFrames = 0; // ile klatek trwa animacja
	int mCurrentFrameRelative = 0;
	float mFrameSpeed = 0.1f;
	float mFrameTimer = 0.0f;
	bool mLoopAnim = true; // czy animacja siê powtarza?

	// Zmienne dla efektu duszka po dash'u
	std::vector<Ghost> mGhosts;
	float mGhostSpawnTimer = 0.0f;

	// Sprawdzamy, jakich wrogów trafi³ gracz aby nie zadaæ im obra¿eñ wiêcej ni¿ raz
	std::vector<Entity*>mHitEntities;

	//Konstruktor
	Player(float startX, float startY) : Entity({ startX, startY }, PLAYER) {
		mPosition = { startX, startY };
		mPrevPosition = mPosition;
		mVelocity = { 0, 0 };
		mTexture = LoadTexture("Assets/player.png");
		mSlashTexture = LoadTexture("Assets/slash.png");
		mFrameRec = { 0.0f, 0.0f, 16.0f, 16.0f };
		mSize = { 10.0f, 14.0f };
	}

	~Player() {
		UnloadTexture(mTexture);
		UnloadTexture(mSlashTexture);
	}

	// Update przyjmuje listê przeszkód
	void update(float deltaTime) override {

		// Zapisujemy prevPosition
		mPrevPosition = mPosition;


		updateTimers(deltaTime);

		handleDash(deltaTime);
		handleMovement(deltaTime);
		handleJump(deltaTime);
		handleAttack(deltaTime);
		applyPhysics(deltaTime);

		handleGhosts(deltaTime);

		// scene_test_room.h to zrobi
		// updateAnimation(deltaTime);
	}

private:

	void updateTimers(float deltaTime) {
		// Zarz¹dzanie buforem skoku
		(IsKeyPressed(KEY_SPACE)) ? mJumpBufferCounter = mJumpBufferTime : mJumpBufferCounter -= deltaTime;

		// Zarz¹dzanie Coyote Time
		(mIsGrounded) ? mCoyoteTimeCounter = mCoyoteTime : mCoyoteTimeCounter -= deltaTime;

		if (mVelocity.y > 0) {
			mIsPogoJump = false;
		}

		if (mRecoilTimer > 0) {
			mRecoilTimer -= deltaTime;
		}
		if (mMomentumTimer > 0) {
			mMomentumTimer -= deltaTime;
		}


		// Mechanika ataku
		// attackTimer
		if (mIsAttacking) {
			mAttackTimer -= deltaTime;

			// koniec ataku
			if (mAttackTimer <= 0) {
				mIsAttacking = false;
				mCurrentState = IDLE;
				mAttackArea = { 0,0,0,0 };

				mHitEntities.clear();
			}
		}
	}

	bool hasHit(Entity* enemy) {
		for (Entity* e : mHitEntities) {
			if (e == enemy) return true;
		}
		return false;
	}

	void handleMovement(float deltaTime) {

		if (mIsDashing) return;

		if (mRecoilTimer > 0) {
			mVelocity.x = approach(mVelocity.x, 0, mRecoilFriction * deltaTime);
			return;
		}

		float moveDir = 0.0f;
		if (IsKeyDown(KEY_RIGHT)) moveDir = 1.0f;
		if (IsKeyDown(KEY_LEFT)) moveDir = -1.0;

		if (mMomentumTimer > 0 && fabsf(mVelocity.x) > mMaxSpeed) {

			if (moveDir != 0) {
				mVelocity.x += moveDir * mAcceleration * 0.5f * deltaTime;
			}

			mVelocity.x = approach(mVelocity.x, 0, (mFriction * 0.70f) * deltaTime);
		}
		else {
			if (moveDir != 0) {
				mVelocity.x = moveDir * mMaxSpeed;
			}
			else {
				mVelocity.x = 0;
			}
		}
	}

	void handleJump(float deltaTime) {
		// skok
		if (mJumpBufferCounter > 0 && mCoyoteTimeCounter > 0)
		{
			mVelocity.y = -mJumpForce;
			mCoyoteTimeCounter = 0; //¿eby nie by³o double jump'u
			mJumpBufferCounter = 0; //¿eby nie by³o double jump'u
			mIsGrounded = false;

			mIsJumping = true;
			mIsPogoJump = false;

			mIsDashing = false;

			if (mMomentumTimer > 0) {
				mMomentumTimer = 0.6f; // Przed³u¿amy pêd w powietrzu
			}
		}

		if (!IsKeyDown(KEY_SPACE) && mVelocity.y < -50.0f && mIsJumping && !mIsPogoJump) {
			mVelocity.y = -50.0f;
		}
	}

	void handleDash(float deltaTime) {
		if (IsKeyPressed(KEY_C) && mCanDash && !mIsDashing) {
			mIsDashing = true;
			mCanDash = false;
			mDashTimer = mDashDuration;
			mCurrentState = DASH;
			mIsJumping = false;

			// Kierunek dash'a
			float dirX = 0.0f;
			float dirY = 0.0f;

			if (IsKeyDown(KEY_RIGHT)) dirX = 1.0f;
			if (IsKeyDown(KEY_LEFT)) dirX = -1.0f;
			if (IsKeyDown(KEY_UP)) dirY = -1.0f;
			if (IsKeyDown(KEY_DOWN)) dirY = 1.0f;

			if (dirX == 0.0f && dirY == 0.0f) {
				dirX = mIsFacingRight ? 1.0f : -1.0f;
			}

			// normalizacja
			if (dirX != 0.0f && dirY != 0.0f) {
				dirX *= 0.7071f;
				dirY *= 0.7071f;
			}

			mVelocity.x = dirX * mDashSpeed;
			mVelocity.y = dirY * mDashSpeed;
		}

		// koniec dash'a
		if (mIsDashing) {
			mDashTimer -= deltaTime;

			if (mDashTimer <= 0) {
				mIsDashing = false;

				if (mVelocity.y != 0) mVelocity.y = 0;
				mVelocity.x = approach(mVelocity.x, 0, mFriction * 0.1f);
			}
		}
	}

	void handleAttack(float deltaTime) {
		if (IsKeyPressed(KEY_Z) && !mIsAttacking) {
			mIsAttacking = true;
			mCurrentState = ATTACK;
			mAttackTimer = mAttackDuration;

			// HITBOX
			float mReach = 25.0f;
			float mThickness = 20.0f;

			if (IsKeyDown(KEY_UP)) {
				mAttackDir = 1;
				// hitbox nad g³ow¹
				mAttackArea = {
					mPosition.x - (mThickness - mSize.x) / 2,
					mPosition.y - mReach,
					mThickness,
					mReach
				};
			}
			else if (IsKeyDown(KEY_DOWN) && !mIsGrounded) {
				mAttackDir = 2;
				// hitbox pod postaci¹
				mAttackArea = {
					mPosition.x - (mThickness - mSize.x) / 2,
					mPosition.y + mSize.y,
					mThickness,
					mReach
				};
			}
			else {
				mAttackDir = 0;
				// hitbox na boki
				if (mIsFacingRight) {
					mAttackArea = { mPosition.x + mSize.x, mPosition.y - 1, mReach, mSize.y };
				}
				else {
					mAttackArea = { mPosition.x - mReach, mPosition.y - 1, mReach, mSize.y };
				}
			}
		}
	}

	void applyPhysics(float deltaTime) {

		if (!mIsDashing) {
			// grawitacja
			mVelocity.y += mGravity * deltaTime;
		}

		// RUCH I KOLIZJE

		// Resetujemy do sprawdzania kolizji
		mIsGrounded = false;

		// Oœ X
		mPosition.x += mVelocity.x * deltaTime;

		// Oœ Y
		mPosition.y += mVelocity.y * deltaTime;
	}

public:

	//potrzebne do kolizji
	Rectangle getRect() override {
		return { mPosition.x, mPosition.y, mSize.x, mSize.y };
	}

	void onCollision(Entity* pOther) override {
		if (pOther->mType == WALL) {

			Rectangle otherRect = pOther->getRect(); // prostok¹t œciany

			// dolna krawêdŸ gracza w poprzedniej klatce
			float prevBottom = mPrevPosition.y + mSize.y;

			// lewa i prawa krawêdŸ w poprzedniej klatce
			float prevRight = mPrevPosition.x + mSize.x;
			float prevLeft = mPrevPosition.x;

			// podloga
			if (prevBottom <= otherRect.y + 2.0f) { // 2.0f toleracji dla floatów 
				mPosition.y = otherRect.y - mSize.y;
				mVelocity.y = 0;
				mIsGrounded = true;
				mCanDash = true;
				mIsJumping = false;

				if (mIsDashing) {
					mIsDashing = false;
					if (fabsf(mVelocity.x) > mMaxSpeed) {
						mMomentumTimer = 0.15f;
					}
				}


			}

			// sufit
			else if (mPrevPosition.y >= otherRect.y + otherRect.height - 2.0f) { // 2.0f tolerancji dla floatów
				mPosition.y = otherRect.y + otherRect.height;
				mVelocity.y = 0;
			}

			// œciany
			else {
				// czy entity by³o z lewej strony
				if (prevRight <= otherRect.x + 5.0f) { // margines b³êdu
					mPosition.x = otherRect.x - mSize.x;
				}
				// czy entity by³o z prawej strony
				else if (prevLeft >= otherRect.x + otherRect.width - 5.0f) { // margines b³êdu
					mPosition.x = otherRect.x + otherRect.width;
				}

				mVelocity.x = 0;
			}

		}
	}

	//funkcja rysuj¹ca
	void draw() override {

		// Rysowanie duszków
		for (const auto& ghost : mGhosts) {
			// obliczamy flip
			float ghostWidth = ghost.facingRight ? ghost.frameRec.width : -ghost.frameRec.width;
			Rectangle source = { ghost.frameRec.x, ghost.frameRec.y, ghostWidth, ghost.frameRec.height };

			// pozycja duszka
			Vector2 drawPos = { (float)floor(ghost.position.x - 3), (float)floor(ghost.position.y - 2) };
			Rectangle dest = { drawPos.x, drawPos.y, 16.0f, 16.0f };

			// kolor duszka
			DrawTexturePro(mTexture, source, dest, { 0,0 }, 0.0f, Fade(SKYBLUE, ghost.alpha));
		}


		float sourceWidth = mIsFacingRight ? mFrameRec.width : -mFrameRec.width;

		Rectangle source = { mFrameRec.x, mFrameRec.y, sourceWidth, mFrameRec.height };

		if (mIsAttacking) {
			int slashFrame = 0;
			if (mAttackTimer < 0.2f) slashFrame = 1;
			if (mAttackTimer < 0.1f) slashFrame = 2;

			Rectangle slashSource = { slashFrame * 32.0f, 0.0f, 32.0f, 32.0f };

			if (!mIsFacingRight) slashSource.width *= -1;

			Vector2 playerCenter = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };

			Vector2 offset = { mIsFacingRight ? 10.0f : -10.0f, 0.0f };
			float rotation = 0.0f;

			if (mAttackDir == 1) {
				offset = { 0.0f, -10.0f };
				rotation = mIsFacingRight ? -90.0f : 90.0f; // góra
			}
			else if (mAttackDir == 2) {
				offset = { 0.0f,10.0f };
				rotation = mIsFacingRight ? 90.0f : -90.0f; // dó³
			}
			
			Rectangle dest = { playerCenter.x + offset.x, playerCenter.y + offset.y, 32.0f, 32.0f };
			Vector2 origin = { 16.0f, 16.0f }; // Œrodek obrotu

			DrawTexturePro(mSlashTexture, slashSource, dest, origin, rotation, WHITE);
		}

		// hitbox 10px
		// texture 16px
		// centrujemy obrazek
		Vector2 drawPos;
		drawPos.x = mPosition.x - 3;
		drawPos.y = mPosition.y - 2;

		// Rysujemy na ekranie (dest)
		Rectangle dest = {
			(float)floor(drawPos.x),
			(float)floor(drawPos.y),
			16.0f, // rozmiar wyœwietlania
			16.0f,
		};

		DrawTexturePro(mTexture, source, dest, { 0,0 }, 0.0f, WHITE);
	}

	void updateAnimation(float deltaTime) {

		float moveInput = 0.0f;
		if (IsKeyDown(KEY_RIGHT)) moveInput = 1.0f;
		if (IsKeyDown(KEY_LEFT)) moveInput = -1.0f;

		bool isPressingMove = (moveInput != 0.0f);

		bool isMovingPhysically = fabsf(mVelocity.x) > 1.0f;
		bool shouldWalk = isPressingMove || isMovingPhysically;


		if (!mIsAttacking && isPressingMove) {
			bool wantsRight = (moveInput > 0.0f);

			if (mIsFacingRight != wantsRight) {
				mIsFacingRight = wantsRight;

				// Uruchamiamy TURN tylko na ziemi
				if (mIsGrounded && mCurrentState != DASH) {
					mCurrentState = TURN;
					mCurrentFrameRelative = 0;
					mFrameTimer = 0.0f;
				}
			}
		}

		AnimState newState = mCurrentState;

		bool isBusyTurning = (mCurrentState == TURN && mCurrentFrameRelative < mLengthFrames - 1);

		// PRIORYTET 1: ATAK / DASH
		if (mIsAttacking) {
			newState = ATTACK;
		}
		else if (mIsDashing) {
			newState = DASH;
		}
		// PRIORYTET 2: POWIETRZE (Przerywa obrót!)
		else if (!mIsGrounded) {
			if (mVelocity.y < -50.0f) newState = JUMP_UP;
			else newState = JUMP_DOWN;
		}
		// PRIORYTET 3: ZIEMIA
		else {
			// Jeœli jesteœmy w trakcie obrotu, wymuœ pozostanie w TURN
			if (isBusyTurning) {
				newState = TURN;
			}
			// Jeœli nie obracamy siê, wybierz WALK lub IDLE
			else {
				if (shouldWalk) newState = WALK;
				else newState = IDLE;
			}
		}

		if (mCurrentState != newState) {
			mCurrentState = newState;
			mCurrentFrameRelative = 0;
			mFrameTimer = 0.0f;
		}

		switch (mCurrentState) {
		case IDLE:
			mStartFrame = 12; mLengthFrames = 6; mLoopAnim = true; mFrameSpeed = 0.15f;
			break;
		case WALK:
			mStartFrame = 0; mLengthFrames = 6; mLoopAnim = true; mFrameSpeed = 0.1f;
			break;
		case JUMP_UP:
			mStartFrame = 6; mLengthFrames = 3; mLoopAnim = false; mFrameSpeed = 0.1f;
			break;
		case JUMP_DOWN:
			mStartFrame = 9; mLengthFrames = 3; mLoopAnim = false; mFrameSpeed = 0.1f;
			break;
		case DASH:
			mStartFrame = 6; mLengthFrames = 1; mLoopAnim = false; mFrameSpeed = 0.5f;
			break;
		case TURN:
			mStartFrame = 18; mLengthFrames = 3; mLoopAnim = false; mFrameSpeed = 0.06f;
			break;
		case ATTACK:
			mLoopAnim = false; mFrameSpeed = 0.1;

			if (mAttackDir == 0) {
				mStartFrame = 22; mLengthFrames = 5;
			}
			else if (mAttackDir == 1) {
				mStartFrame = 28; mLengthFrames = 5;
			}
			else if (mAttackDir == 2) {
				mStartFrame = 34; mLengthFrames = 5;
			}
			break;
		}

		mFrameTimer += deltaTime;

		if (mFrameTimer >= mFrameSpeed) {
			mFrameTimer = 0.0f;
			mCurrentFrameRelative++;

			if (mCurrentFrameRelative >= mLengthFrames) {
				if (mLoopAnim) {
					mCurrentFrameRelative = 0;
				}
				else {
					mCurrentFrameRelative = mLengthFrames - 1;

					// LOGIKA WYJŒCIA Z TURN
					// Gdy animacja obrotu siê skoñczy³a, decydujemy co dalej
					if (mCurrentState == TURN) {
						// Jeœli nadal biegnie lub trzyma klawisz -> WALK, w przeciwnym razie IDLE
						if (shouldWalk) mCurrentState = WALK;
						else mCurrentState = IDLE;

						mCurrentFrameRelative = 0;
					}
				}
			}
		}

		mFrameRec.x = (float)(mStartFrame + mCurrentFrameRelative) * 16.0f;
	}

	void handleGhosts(float deltaTime) {
		if (mIsDashing || mMomentumTimer > 0) {
			mGhostSpawnTimer -= deltaTime;

			if (mGhostSpawnTimer <= 0) {
				mGhostSpawnTimer = 0.05f;

				Ghost newGhost;
				newGhost.position = mPosition;
				newGhost.frameRec = mFrameRec;
				newGhost.facingRight = mIsFacingRight;
				newGhost.alpha = 0.8f; // startowa przeŸroczystoœæ

				mGhosts.push_back(newGhost);
			}
		}
		else {
			mGhostSpawnTimer = 0; // reset, aby po wciœniêciu dasha duch pojawi³ siê od razu
		}

		for (int i = 0; i < mGhosts.size(); i++) {
			mGhosts[i].alpha -= 3.0f * deltaTime;

			/*
			float lerpSpeed = 5.0f * deltaTime;
			mGhosts[i].position.x += (mPosition.x - mGhosts[i].position.x) * lerpSpeed;
			mGhosts[i].position.y += (mPosition.y - mGhosts[i].position.y) * lerpSpeed;
			*/

			// usuwanie martwych duszków
			if (mGhosts[i].alpha <= 0.0f) {
				mGhosts.erase(mGhosts.begin() + i);
			}

		}
	}
};
