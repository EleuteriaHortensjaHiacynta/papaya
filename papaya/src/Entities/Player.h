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

enum WeaponType {
	SWORD_DEFAULT,
	KATANA,
	SPEAR,
};

struct WeaponStats {
	float damage;
	float reachX;
	float reachY;
	float duration;
	float recoilSelf;
	float pogoForce;
	Color debugColor;
};

enum AnimState {
	IDLE, WALK, JUMP_UP, JUMP_DOWN, TURN, ATTACK, DASH, CLIMB, WALL_SLIDE
};

class Player : public Entity {
public:
	Vector2 mPosition;
	Vector2 mPrevPosition;
	Vector2 mVelocity;
	Vector2 mSize;

	Vector2 mStartPosition;

	// Fizyka
	float mMaxSpeed = 100.0f;
	float mAcceleration = 1110.0f;
	float mFriction = 990.0f;
	float mGravity = 500.0f;
	float mJumpForce = 250.0f;
	float mRecoilFriction = 20.0f;

	// Double Jump
	int mJumpCount = 0;
	int mMaxJumps = 2;

	// Dash
	float mDashTimer = 0.0f;
	float mDashDuration = 0.15f;
	float mMomentumTimer = 0.0f;
	float mDashSpeed = 280.0f;
	bool mIsDashing = false;
	bool mCanDash = true;
	const float WAVE_DASH_BOOST_SPEED = 1.3f;
	float mDashCooldownTimer = 0.0f;
	const float DASH_COOLDOWN = 0.5f;

	// Climb
	bool mTouchingWall = false;
	int mWallDir = 0; // 1 - œciana po prawej ; -1 - œciana po lewej
	float mStamina = 100.0f;
	float mMaxStamina = 100.0f;
	float mStaminaRegenDelay = 0.0f;

	const float STAMINA_REGEN_SPEED = 50.0f;
	const float STAMINA_DELAY_TIME= 0.5f;
	const float COST_CLIMB = 30.0f;

	// Bronie
	WeaponType mCurrentSpecialWeapon = SWORD_DEFAULT;

	// baza danych broni
	WeaponStats getCurrentWeaponStats() {
		switch (mCurrentSpecialWeapon) {
		case KATANA:
			break;
		case SPEAR:
			break;
		default:
			return { 1.0f, 25.0f, 20.0f, 0.33f, 300.0f, -260.0f, WHITE };
		}
	}

	// Zdrowie
	int mHealth = 3;
	int mMaxHealth = 5;
	float mInvincibilityTimer = 0.0f;

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
	bool mIsClimbing = false;

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

	// Animacja skrzyde³ (dla double jump)
	Texture2D mWingsTexture;
	bool mWingsActive;
	int mWingsFrame = 0;
	float mWingsTimer = 0.0f;
	const int WINGS_NUM_FRAMES = 6;
	const float WINGS_SPEED = 0.08f;

	// Zmienne dla efektu duszka po dash'u
	std::vector<Ghost> mGhosts;
	float mGhostSpawnTimer = 0.0f;

	// Sprawdzamy, jakich wrogów trafi³ gracz aby nie zadaæ im obra¿eñ wiêcej ni¿ raz
	std::vector<Entity*>mHitEntities;

	//Konstruktor
	Player(float startX, float startY) : Entity({ startX, startY }, PLAYER) {
		mPosition = { startX, startY };
		mStartPosition = { startX, startY };
		mPrevPosition = mPosition;
		mVelocity = { 0, 0 };
		mTexture = LoadTexture("Assets/player.png");
		mSlashTexture = LoadTexture("Assets/slash.png");
		mWingsTexture = LoadTexture("Assets/jump_wings.png");
		mWingsActive = false;
		mFrameRec = { 0.0f, 0.0f, 16.0f, 16.0f };
		mSize = { 10.0f, 14.0f };
	}

	~Player() {
		UnloadTexture(mTexture);
		UnloadTexture(mSlashTexture);
		UnloadTexture(mWingsTexture);
	}

	// Update przyjmuje listê przeszkód
	void update(float deltaTime) override {

		if (mHealth <= 0) {
			respawn();
			return;
		}

		if (mPosition.y > 500.0f) {
			respawn();
			return;
		}
		
		// Zapisujemy prevPosition
		mPrevPosition = mPosition;



		updateTimers(deltaTime);

		handleWallMovement(deltaTime);

		handleDash(deltaTime);
		handleMovement(deltaTime);

		handleJump(deltaTime);

		handleAttack(deltaTime);
		
		applyPhysics(deltaTime);

		mTouchingWall = false;

		updateStamina(deltaTime);
		handleGhosts(deltaTime);

		updateWings(deltaTime);

		handleSacrifice();



		// scene_test_room.h to zrobi
		// updateAnimation(deltaTime);
	}

private:

	void respawn() {
		mPosition = mStartPosition;
		mHealth = mMaxHealth;
		mVelocity = { 0,0 };
		mStamina = mMaxStamina;
		mRecoilTimer = 0;
		mInvincibilityTimer = 0;
		mIsDashing = false;
		mCurrentState = IDLE;
	}

	void handleSacrifice() {
		// Mechanika ta sprawia, ¿e broñ specjaln¹, któr¹ trzyma gracz zostaje zamieniona na jego punkty zdrowia
		if (IsKeyPressed(KEY_H) && mCurrentSpecialWeapon != SWORD_DEFAULT) {
			
			if (mHealth < mMaxHealth) {
					mHealth++;
			}
			
			mCurrentSpecialWeapon = SWORD_DEFAULT;
		}
	}

	void updateTimers(float deltaTime) {
		// Zarz¹dzanie buforem skoku
		(IsKeyPressed(KEY_SPACE)) ? mJumpBufferCounter = mJumpBufferTime : mJumpBufferCounter -= deltaTime;

		// Zarz¹dzanie Coyote Time
		(mIsGrounded) ? mCoyoteTimeCounter = mCoyoteTime : mCoyoteTimeCounter -= deltaTime;

		if (mVelocity.y > 0) {
			mIsPogoJump = false;
		}

		if (mDashCooldownTimer > 0) {
			mDashCooldownTimer -= deltaTime;
		}

		if (mInvincibilityTimer > 0) {
			mInvincibilityTimer -= deltaTime;
		}

		if (mRecoilTimer > 0) {
			mRecoilTimer -= deltaTime;

			if (fabsf(mVelocity.x) > mMaxSpeed) {
				float dir = (mVelocity.x > 0) ? 1.0f : -1.0f;
				mVelocity.x = dir * mMaxSpeed;
			}
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

	void updateStamina(float deltaTime) {
		if (mIsClimbing) return;

		if (mStaminaRegenDelay > 0) {
			mStaminaRegenDelay -= deltaTime;
		}
		else {
			if (mStamina < mMaxStamina) {
				mStamina += STAMINA_REGEN_SPEED * deltaTime;
				if (mStamina > mMaxStamina) mStamina = mMaxStamina;
			}
		}
	}

	bool hasHit(Entity* enemy) {
		for (Entity* e : mHitEntities) {
			if (e == enemy) return true;
		}
		return false;
	}

	void handleWallMovement(float deltaTime) {
		
		if (mTouchingWall && mStamina > 0) {

			bool isClimbingInput = IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN);
			bool isGrippedInput = false;

			if (mWallDir == 1 && IsKeyDown(KEY_RIGHT)) isGrippedInput = true;
			else if (mWallDir == -1 && IsKeyDown(KEY_LEFT)) isGrippedInput = true;

			// Gdy stoimy przy œcianie, nie chcemy siê do niej przyklejaæ
			if (mIsGrounded && !IsKeyDown(KEY_UP)) {
				mIsClimbing = false;
				return;
			}
			
			if (isClimbingInput && isGrippedInput) {
				
				mCurrentState = CLIMB;
				mIsClimbing = true;

				float climbDir = 0.0f;
				if (IsKeyDown(KEY_UP)) climbDir = -1.0f;
				if (IsKeyDown(KEY_DOWN)) climbDir = 1.0f;
			
				// Zu¿ycie staminy
				mStamina -= COST_CLIMB * deltaTime;
				mStaminaRegenDelay = STAMINA_DELAY_TIME;

				float climbSpeed = 50.0f;
				mVelocity.y = climbDir *climbSpeed;
			}
			else if (!mIsGrounded){
				// zeœlizgujemy siê ze œciany
				mStamina -= (COST_CLIMB * 0.5f) * deltaTime;
				mStaminaRegenDelay = STAMINA_DELAY_TIME;

				mCurrentState = WALL_SLIDE;
				mIsClimbing = true;
				mVelocity.y = 25.0f;
				mVelocity.x = 0.0f;
			}
			else {
				mIsClimbing = false;
			}
		}
		else {
			mIsClimbing = false;
		}
	}

	void handleMovement(float deltaTime) {

		if (mIsDashing) return;

		// inputy
		float moveDir = 0.0f;
		if (IsKeyDown(KEY_RIGHT)) moveDir = 1.0f;
		if (IsKeyDown(KEY_LEFT)) moveDir = -1.0;

		// Konfiguracja fizyki
		float currentMaxSpeed = mMaxSpeed;

		// ziemia - normalny ruch
		float groundAccel = 2000.0f;
		float groundFriction = 3000.0f;

		// ziemia - Wavedash
		float momentumDragGround = 1000.0f;
		
		// powietrze - normalny ruch
		float airAccel = 1200.0f; // normalne przyœpieszenie w powietrzu
		float airTurnAccel = 3000.0f; // szybkie hamowanie/nawracanie
		float airDrag = 500.0f; // opór powietrza

		// (soft cap) dla zachowania momentu dla dash'y i wavedash'y
		bool isSuperSpeed = fabsf(mVelocity.x) > currentMaxSpeed;
		
		if (mIsGrounded) {
			// ZIEMIA
			if (isSuperSpeed) {
				// Wavedash po ziemii <=> ma³e tarcie
				
				float drag = momentumDragGround;
				if ((mVelocity.x > 0 && moveDir > 0) || (mVelocity.x < 0 && moveDir < 0)) {
					drag *= 0.5f; // przytrzymuj¹c przycisk wyd³u¿asz "œlizg"
				}
				
				mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, drag * deltaTime);
			}
			else {
				// normalne bieganie
				if (moveDir != 0) {
					mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, groundAccel * deltaTime);
				}
				else {
					mVelocity.x = approach(mVelocity.x, 0, groundFriction * deltaTime);
				}
			}
		}
		else {
			// POWIETRZE
			if (isSuperSpeed) {
				// powrót do normalnej prêdkoœci
				mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, airDrag * deltaTime);
			}
			else {
				// normalna kontrola w powietrzu
				if (moveDir != 0) {
					bool isTurning = (mVelocity.x > 0 && moveDir < 0) || (mVelocity.x < 0 && moveDir > 0);
					float accel = isTurning ? airTurnAccel : airAccel;
					mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, accel * deltaTime);
				}
				else {
					mVelocity.x = approach(mVelocity.x, 0, airDrag * deltaTime);
				}
			}
		}
	}

	void handleJump(float deltaTime) {
		

		// skok
		if (IsKeyPressed(KEY_SPACE) || mJumpBufferCounter > 0) {
			
			bool jumped = false;
		
			// wall jump (odbicie siê od œciany w skoku)
			if (mTouchingWall && !mIsGrounded) {
				mVelocity.y = -mJumpForce;

				float wallJumpKick = 180.0f;
				mVelocity.x = -mWallDir * wallJumpKick;

				// œciana po prawej, skaczemy w lewo -> facingRight = false
				// œciana po lewej, skaczemy w prawo -> facingRight = true
				mIsFacingRight = (mWallDir == -1);
				
				mIsClimbing = false;
				mJumpCount = 1; // wall jump mo¿e resetowaæ skok
				jumped = true;
			}
			
			// pierwszy skok
			else if (mCoyoteTimeCounter > 0) {
				mVelocity.y = -mJumpForce;
				mJumpCount = 1;
				jumped = true;
			}
			//  drugi skok
			else if (mJumpCount < mMaxJumps) {
				mVelocity.y = -mJumpForce;
				mJumpCount++;
				jumped = true;

				// odzyskanie kontroli nad sterowaniem
				mMomentumTimer = 0.0f;

				// aktywacja animacji skrzyde³
				mWingsActive = true;
				mWingsFrame = 0;
				mWingsTimer = 0.0f;
			}

			if (jumped) {
				mCoyoteTimeCounter = 0;
				mJumpBufferCounter = 0;
				mIsGrounded = false;
				mIsJumping = true;
				mIsPogoJump = false;
				mIsDashing = false; // skok przerywa dash
				if (mMomentumTimer > 0) {
					mMomentumTimer = 0.6f;
				}

			}
		}
		
		if (!IsKeyDown(KEY_SPACE) && mVelocity.y < -50.0f && mIsJumping && !mIsPogoJump) {
			mVelocity.y = -50.0f;
		}
	}

	void handleDash(float deltaTime) {
		if (IsKeyPressed(KEY_C) && mCanDash && !mIsDashing && mDashCooldownTimer <= 0) {
			mIsDashing = true;
			mCanDash = false;

			mDashTimer = mDashDuration;
			mDashCooldownTimer = DASH_COOLDOWN;
			
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

				if (fabs(mVelocity.y) > mMaxSpeed) {
					mVelocity.y *= 0.6f;
				}
			}
		}
	}

	void updateWings(float deltaTime) {
		if (mWingsActive) {
			mWingsTimer += deltaTime;
			if (mWingsTimer >= WINGS_SPEED) {
				mWingsTimer = 0.0f;
				mWingsFrame++;

				// gdy animacja dobie³ka koñca
				if (mWingsFrame >= WINGS_NUM_FRAMES) {
					mWingsActive = false;
					mWingsFrame = 0;
				}
			}
		}
	}

	void handleAttack(float deltaTime) {
		if (IsKeyPressed(KEY_Z) && !mIsAttacking) {

			WeaponStats stats = getCurrentWeaponStats();

			mIsAttacking = true;
			mCurrentState = ATTACK;
			mAttackTimer = stats.duration;

			// HITBOX
			float reachX = stats.reachX;
			float reachY = stats.reachY;

			if (IsKeyDown(KEY_UP)) {
				mAttackDir = 1;
				// hitbox nad g³ow¹
				mAttackArea = {
					mPosition.x - (reachY - mSize.x) / 2,
					mPosition.y - reachX,
					reachY,
					reachX
				};
			}
			else if (IsKeyDown(KEY_DOWN) && !mIsGrounded) {
				mAttackDir = 2;
				// hitbox pod postaci¹
				mAttackArea = {
					mPosition.x - (reachY - mSize.x) / 2,
					mPosition.y + mSize.y,
					reachY,
					reachX
				};
			}
			else {
				mAttackDir = 0;

				// wycentrowanie w pionie
				float yOffset = (mSize.y - reachY) / 2;

				// hitbox na boki
				if (mIsFacingRight) {
					mAttackArea = { mPosition.x + mSize.x, mPosition.y + yOffset, reachX, reachY };
				}
				else {
					mAttackArea = { mPosition.x - reachX, mPosition.y + yOffset, reachX, reachY };
				}
			}
		}
	}

	void applyPhysics(float deltaTime) {

		if (!mIsDashing && !mIsClimbing) {
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

				mJumpCount = 0;

				// (WAVEDASH LOGIC) jeœli dotkneliœmy ziemi podczas dash'a
				if (mIsDashing) {
					mIsDashing = false;

					float dashDir = 0.0f;

					if (mVelocity.x > 0.1f) {
						dashDir = 1.0f;
					}
					else if(mVelocity.x < -0.1f){
						dashDir = -1.0f;
					}

					else if (IsKeyDown(KEY_RIGHT)) {
						dashDir = 1.0f;
					}
					else if (IsKeyDown(KEY_LEFT)) {
						dashDir = -1.0f;
					}

					else {
						dashDir = mIsFacingRight ? 1.0f : -1.0f;
					}

					mVelocity.x = dashDir * mDashSpeed * WAVE_DASH_BOOST_SPEED; //1.3f leciutki boost
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
					mTouchingWall = true;
					mWallDir = 1.0f; // œciana po prawej
				}
				// czy entity by³o z prawej strony
				else if (prevLeft >= otherRect.x + otherRect.width - 5.0f) { // margines b³êdu
					mPosition.x = otherRect.x + otherRect.width;
					mTouchingWall = true;
					mWallDir = -1.0f; // œciana po lewej
				}

				mVelocity.x = 0;
			}

		}
	}

	//funkcja rysuj¹ca
	void draw() override {


		if (mStamina < mMaxStamina) {
			float barWidth = 20.0f;
			float barHeight = 2.0f;
			float barX = mPosition.x + mSize.x / 2 - barWidth / 2;
			float barY = mPosition.y - 10.0f;

			//// T³o
			//DrawRectangle(barX, barY, barWidth, barHeight, Fade(BLACK, 0.6f));

			// Wype³nienie
			float percent = mStamina / mMaxStamina;
			Color color = ColorAlpha(RED,percent); // Czerwony zanika gdy koñcówka si³
			
			if (percent < 0.50f) {
				DrawRectangle(barX, barY, barWidth * percent, barHeight, color);
			}
		}

		// Rysowanie duszków
		for (const auto& ghost : mGhosts) {
			// obliczamy flip
			float ghostWidth = ghost.facingRight ? ghost.frameRec.width : -ghost.frameRec.width;
			Rectangle source = { ghost.frameRec.x, ghost.frameRec.y, ghostWidth, ghost.frameRec.height };

			// pozycja duszka
			Vector2 drawPos = { (float)floor(ghost.position.x - 3), (float)floor(ghost.position.y - 2) };
			Rectangle dest = { drawPos.x, drawPos.y, 16.0f, 16.0f };

			// kolor duszka
			DrawTexturePro(mTexture, source, dest, { 0,0 }, 0.0f, Fade(WHITE, ghost.alpha));
		}

		if (mWingsActive) {
			float wingW = 32.0f;
			float wingH = 16.0f;

			Rectangle source = { mWingsFrame * wingW, 0.0f, mIsFacingRight ? wingW : -wingW, wingH };
			
			Vector2 playerCenter = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };

			Vector2 wingsPos = {
				playerCenter.x - (wingW / 2.0f),
				playerCenter.y - (wingH / 2.0f)
			};

			wingsPos.y += 1.0f;

			DrawTextureRec(mWingsTexture, source, wingsPos, Fade(WHITE,0.5f));
		}

		float sourceWidth = mIsFacingRight ? mFrameRec.width : -mFrameRec.width;

		Rectangle source = { mFrameRec.x, mFrameRec.y, sourceWidth, mFrameRec.height };

		if (mIsAttacking) {
			int slashFrame = 0;
			if (mAttackTimer < 0.2f) slashFrame = 1;
			if (mAttackTimer < 0.1f) slashFrame = 2;

			float spriteSize = 64.0f;

			Rectangle slashSource = { slashFrame * spriteSize, 0.0f, spriteSize, spriteSize };

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
			
			Rectangle dest = { playerCenter.x + offset.x, playerCenter.y + offset.y, spriteSize, spriteSize};
			Vector2 origin = { spriteSize/2.0f, spriteSize/2.0f }; // Œrodek obrotu

			DrawTexturePro(mSlashTexture, slashSource, dest, origin, rotation, WHITE);
		}

		// hitbox 10px
		// texture 16px
		// centrujemy obrazek
		Vector2 drawPos;
		drawPos.x = mPosition.x - 3;
		drawPos.y = mPosition.y - 2;

		if (mCurrentState == CLIMB || mCurrentState == WALL_SLIDE) {
			float wallOffset = 2.0f;
			if (mIsFacingRight) {
				drawPos.x += wallOffset;
			}
			else {
				drawPos.x -= wallOffset;
			}
		}

		// Rysujemy na ekranie (dest)
		Rectangle dest = {
			(float)floor(drawPos.x),
			(float)floor(drawPos.y),
			16.0f, // rozmiar wyœwietlania
			16.0f,
		};

		// MRUGANIE PO OTRZYMANIU OBRA¯EÑ
		Color drawColor = WHITE;
		if (mInvincibilityTimer > 0.0f) {
			if ((int)(mInvincibilityTimer * 20.0f) % 2 == 0) {
				drawColor = RED;
			}
		}

		DrawTexturePro(mTexture, source, dest, { 0,0 }, 0.0f, drawColor);
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

		// PRIORYTET 1: ATAK / DASH / CLIMB / SLIDE
		if (mIsAttacking) {
			newState = ATTACK;
		}
		else if (mIsDashing) {
			newState = DASH;
		}
		else if (mIsClimbing) {
			newState = mCurrentState;
		}
		// PRIORYTET 2: POWIETRZE
		else if (!mIsGrounded) {
			if (mVelocity.y < -50.0f) newState = JUMP_UP;
			else newState = JUMP_DOWN;
		}
		// PRIORYTET 3: ZIEMIA
		else {
			if (isBusyTurning) {
				newState = TURN;
			}
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
		case CLIMB:
			mLoopAnim = true;
			mFrameSpeed = 0.15f;
			
			if (mVelocity.y < 0) {
				mStartFrame = 39; mLengthFrames = 2;
			}
			else if (mVelocity.y > 0) {
				mStartFrame = 41; mLengthFrames = 2;
			}
			else {
				mStartFrame = 39; mLengthFrames = 1;
			}
			break;
		case WALL_SLIDE:
			mStartFrame = 43; mLengthFrames = 2; mLoopAnim = true; mFrameSpeed = 0.15f;
			break;
		case ATTACK:
			mLoopAnim = false; mFrameSpeed = 0.06f;

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

		float speedThreshold = mMaxSpeed * WAVE_DASH_BOOST_SPEED;
		bool isSuperSpeed = fabsf(mVelocity.x) > speedThreshold;

		bool isInRecoil = mRecoilTimer > 0.0f;

		if (mIsDashing || isSuperSpeed && !isInRecoil) {
			mGhostSpawnTimer -= deltaTime;

			if (mGhostSpawnTimer <= 0) {
				mGhostSpawnTimer = 0.02f;

				Ghost newGhost;
				newGhost.position = mPosition;
				newGhost.frameRec = mFrameRec;
				newGhost.facingRight = mIsFacingRight;
				newGhost.alpha = 0.5f; // startowa przeŸroczystoœæ

				mGhosts.push_back(newGhost);
			}
		}
		else {
			mGhostSpawnTimer = 0; // reset, aby po wciœniêciu dasha duch pojawi³ siê od razu
		}

		for (int i = 0; i < mGhosts.size(); i++) {
			mGhosts[i].alpha -= 3.0f * deltaTime;

			/*float lerpSpeed = 5.0f * deltaTime;
			mGhosts[i].position.x += (mPosition.x - mGhosts[i].position.x) * lerpSpeed;
			mGhosts[i].position.y += (mPosition.y - mGhosts[i].position.y) * lerpSpeed;*/

			// usuwanie martwych duszków
			if (mGhosts[i].alpha <= 0.0f) {
				mGhosts.erase(mGhosts.begin() + i);
				i--;
			}

		}
	}
};