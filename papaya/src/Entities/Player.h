#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm> // potrzebne do std::remove_if
#include "Entities/Entity.h"
#include "Saves/map.hpp"

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
    Vector2 mPrevPosition;
    Vector2 mVelocity;
    Vector2 mStartPosition;

    // SFX
    Sound mSfxAttack;
    Sound mSfxJump;
    Sound mSfxHurt;
    Sound mSfxDeath;

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

    // Dash & Unlockables
    bool mCanDoubleJump = false;
    bool mCanWavedash = false;

    float mDashTimer = 0.0f;
    float mDashDuration = 0.15f;
    float mMomentumTimer = 0.0f;
    float mDashSpeed = 280.0f;
    bool mIsDashing = false;
    bool mCanDash = true;
    float mDashCooldownTimer = 0.0f;
    const float WAVE_DASH_BOOST_SPEED = 1.3f;
    const float DASH_COOLDOWN = 0.5f;

    // Climb
    bool mTouchingWall = false;
    int mWallDir = 0; // 1 - sciana po prawej ; -1 - sciana po lewej
    float mStamina = 100.0f;
    float mMaxStamina = 100.0f;
    float mStaminaRegenDelay = 0.0f;

    const float STAMINA_REGEN_SPEED = 50.0f;
    const float STAMINA_DELAY_TIME = 0.5f;
    const float COST_CLIMB = 30.0f;

    // Bronie
    WeaponType mCurrentSpecialWeapon = SWORD_DEFAULT;

    WeaponStats getCurrentWeaponStats() {
        switch (mCurrentSpecialWeapon) {
        case KATANA:
            return { 1.5f, 30.0f, 25.0f, 0.25f, 300.0f, -260.0f, RED }; // Przykładowe staty
        case SPEAR:
            return { 1.0f, 40.0f, 15.0f, 0.4f, 300.0f, -260.0f, BLUE };
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
    int mAttackDir = 0; // 0 - boki, 1 - gora, 2 - dol
    int mAttackDamage = 1;
    Rectangle mAttackArea = { 0,0,0,0 };
    int mComboHit = 0;
    float mComboTimer = 0.0f;
    const float COMBO_WINDOW = 0.5f;

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

    Texture2D mSlashTexture;
    float mSlashTimer = 0.0f;

    int mStartFrame = 0;
    int mLengthFrames = 0;
    int mCurrentFrameRelative = 0;
    float mFrameSpeed = 0.1f;
    float mFrameTimer = 0.0f;
    bool mLoopAnim = true;

    // Animacja skrzydel
    Texture2D mWingsTexture;
    bool mWingsActive;
    int mWingsFrame = 0;
    float mWingsTimer = 0.0f;
    const int WINGS_NUM_FRAMES = 6;
    const float WINGS_SPEED = 0.08f;

    std::vector<Ghost> mGhosts;
    float mGhostSpawnTimer = 0.0f;

    std::vector<Entity*>mHitEntities;

    // Konstruktor
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
        mSize = { 10.0f, 14.0f }; // Hitbox gracza

        // SFX
        mSfxAttack = LoadSound("Assets/sfx/attack.wav");
        mSfxJump = LoadSound("Assets/sfx/jump.wav");
        mSfxHurt = LoadSound("Assets/sfx/hurt.wav");
        mSfxDeath = LoadSound("Assets/sfx/death.wav");

        // Debugowo odblokowane
        mCanDoubleJump = true;
        mCanWavedash = true;
    }

    ~Player() {
        UnloadTexture(mTexture);
        UnloadTexture(mSlashTexture);
        UnloadTexture(mWingsTexture);

        UnloadSound(mSfxAttack);
        UnloadSound(mSfxJump);
        UnloadSound(mSfxHurt);
        UnloadSound(mSfxDeath);
    }

    void update(float deltaTime) override {

        if (mHealth <= 0 || mPosition.y > 600.0f) {
            respawn();
            return;
        }

        mPrevPosition = mPosition;

        updateTimers(deltaTime);
        handleWallMovement(deltaTime);
        handleDash(deltaTime);
        handleMovement(deltaTime);
        handleJump(deltaTime);
        handleAttack(deltaTime);

        applyPhysics(deltaTime);
        // UWAGA: applyPhysics() teraz tylko liczy grawitację. 
        // Faktyczne przesuwanie (x += vel * dt) robi funkcja ResolveMapCollision w Scene!

        mTouchingWall = false; // Reset flagi (ResolveMapCollision ustawi ją ponownie jeśli dotkniemy ściany)

        updateStamina(deltaTime);
        handleGhosts(deltaTime);
        updateWings(deltaTime);
        handleSacrifice();

        updateAnimation(deltaTime);
    }

    void playSoundRandom(Sound sound, float minPitch = 0.9f, float maxPitch = 1.1f) {
        float pitch = minPitch + (float)GetRandomValue(0, 100) / 100.0f * (maxPitch - minPitch);
        SetSoundPitch(sound, pitch);
        PlaySound(sound);
    }

    // Nowa metoda pomocnicza dla Scene, żeby zresetować skoki przy dotknięciu ziemi
    void onGroundHit() {
        mIsGrounded = true;
        mCanDash = true;
        mIsJumping = false;
        mJumpCount = 0;

        // Logika Wavedash lądowania
        if (mIsDashing) {
            mIsDashing = false;
            if (!mCanWavedash) {
                mVelocity.x = 0;
                return;
            }
            // Boost przy lądowaniu
            float boost = (mVelocity.x > 0 ? 1.0f : -1.0f) * WAVE_DASH_BOOST_SPEED;
            if (fabs(mVelocity.x) > 10.0f) mVelocity.x = boost * mDashSpeed;
        }
    }
 
    void takeDamage(int amount) {
        if (mInvincibilityTimer > 0) return;
        mHealth -= amount;
        mInvincibilityTimer = 1.0f;
        playSoundRandom(mSfxHurt);
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
        playSoundRandom(mSfxDeath);
    }

    void handleSacrifice() {
        if (IsKeyPressed(KEY_H) && mCurrentSpecialWeapon != SWORD_DEFAULT) {
            if (mHealth < mMaxHealth) {
                mHealth++;
            }
            mCurrentSpecialWeapon = SWORD_DEFAULT;
        }
    }

    void updateTimers(float deltaTime) {
        (IsKeyPressed(KEY_SPACE)) ? mJumpBufferCounter = mJumpBufferTime : mJumpBufferCounter -= deltaTime;
        (mIsGrounded) ? mCoyoteTimeCounter = mCoyoteTime : mCoyoteTimeCounter -= deltaTime;

        if (mVelocity.y > 0) mIsPogoJump = false;
        if (mDashCooldownTimer > 0) mDashCooldownTimer -= deltaTime;
        if (mInvincibilityTimer > 0) mInvincibilityTimer -= deltaTime;

        if (mRecoilTimer > 0) {
            mRecoilTimer -= deltaTime;
            if (fabsf(mVelocity.x) > mMaxSpeed) {
                float dir = (mVelocity.x > 0) ? 1.0f : -1.0f;
                mVelocity.x = dir * mMaxSpeed;
            }
        }

        if (mMomentumTimer > 0) mMomentumTimer -= deltaTime;

        if (mIsAttacking) {
            mAttackTimer -= deltaTime;
            if (mAttackTimer <= 0) {
                mIsAttacking = false;
                mCurrentState = IDLE;
                mAttackArea = { 0,0,0,0 };
                mHitEntities.clear();
            }
        }

        if (mComboTimer > 0) {
            mComboTimer -= deltaTime;
            if (mComboTimer <= 0) mComboHit = 0;
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

    void handleWallMovement(float deltaTime) {
        if (mTouchingWall && mStamina > 0) {
            bool isClimbingInput = IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN);
            bool isGrippedInput = (mWallDir == 1 && IsKeyDown(KEY_RIGHT)) || (mWallDir == -1 && IsKeyDown(KEY_LEFT));

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

                mStamina -= COST_CLIMB * deltaTime;
                mStaminaRegenDelay = STAMINA_DELAY_TIME;
                mVelocity.y = climbDir * 50.0f;
            }
            else if (!mIsGrounded) {
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

        float moveDir = 0.0f;
        if (IsKeyDown(KEY_RIGHT)) moveDir = 1.0f;
        if (IsKeyDown(KEY_LEFT)) moveDir = -1.0;

        float currentMaxSpeed = mMaxSpeed;
        float groundAccel = 2000.0f;
        float groundFriction = 3000.0f;
        float momentumDragGround = 1000.0f;
        float airAccel = 1200.0f;
        float airTurnAccel = 3000.0f;
        float airDrag = 500.0f;

        bool isSuperSpeed = fabsf(mVelocity.x) > currentMaxSpeed;

        if (mIsGrounded) {
            if (isSuperSpeed && mCanWavedash) {
                float drag = momentumDragGround;
                if ((mVelocity.x > 0 && moveDir > 0) || (mVelocity.x < 0 && moveDir < 0)) {
                    drag *= 0.5f;
                }
                mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, drag * deltaTime);
            }
            else {
                if (moveDir != 0) mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, groundAccel * deltaTime);
                else mVelocity.x = approach(mVelocity.x, 0, groundFriction * deltaTime);
            }
        }
        else {
            if (isSuperSpeed) {
                mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, airDrag * deltaTime);
            }
            else {
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
        if (IsKeyPressed(KEY_SPACE) || mJumpBufferCounter > 0) {
            bool jumped = false;

            if (mTouchingWall && !mIsGrounded) {
                playSoundRandom(mSfxJump, 0.95f, 1.05f);
                mVelocity.y = -mJumpForce;
                float wallJumpKick = 180.0f;
                mVelocity.x = -mWallDir * wallJumpKick;
                mIsFacingRight = (mWallDir == -1);
                mIsClimbing = false;
                mJumpCount = 1;
                jumped = true;
            }
            else if (mCoyoteTimeCounter > 0) {
                playSoundRandom(mSfxJump, 0.95f, 1.05f);
                mVelocity.y = -mJumpForce;
                mJumpCount = 1;
                jumped = true;
            }
            else if (mJumpCount < mMaxJumps && mCanDoubleJump) {
                playSoundRandom(mSfxJump, 1.1f, 1.25f);
                mVelocity.y = -mJumpForce;
                mJumpCount++;
                jumped = true;
                mMomentumTimer = 0.0f;
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
                mIsDashing = false;
                if (mMomentumTimer > 0) mMomentumTimer = 0.6f;
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

            float dirX = 0.0f;
            float dirY = 0.0f;

            if (IsKeyDown(KEY_RIGHT)) dirX = 1.0f;
            if (IsKeyDown(KEY_LEFT)) dirX = -1.0f;
            if (IsKeyDown(KEY_UP)) dirY = -1.0f;
            if (IsKeyDown(KEY_DOWN)) dirY = 1.0f;

            if (dirX == 0.0f && dirY == 0.0f) dirX = mIsFacingRight ? 1.0f : -1.0f;

            if (dirX != 0.0f && dirY != 0.0f) {
                dirX *= 0.7071f;
                dirY *= 0.7071f;
            }

            mVelocity.x = dirX * mDashSpeed;
            mVelocity.y = dirY * mDashSpeed;
        }

        if (mIsDashing) {
            mDashTimer -= deltaTime;
            if (mDashTimer <= 0) {
                mIsDashing = false;
                if (fabs(mVelocity.y) > mMaxSpeed) mVelocity.y *= 0.6f;
            }
        }
    }

    void updateWings(float deltaTime) {
        if (mWingsActive) {
            mWingsTimer += deltaTime;
            if (mWingsTimer >= WINGS_SPEED) {
                mWingsTimer = 0.0f;
                mWingsFrame++;
                if (mWingsFrame >= WINGS_NUM_FRAMES) {
                    mWingsActive = false;
                    mWingsFrame = 0;
                }
            }
        }
    }

    void handleAttack(float deltaTime) {
        if (IsKeyPressed(KEY_Z) && !mIsAttacking) {
            playSoundRandom(mSfxAttack, 0.85f, 1.15f);
            WeaponStats stats = getCurrentWeaponStats();

            mIsAttacking = true;
            mCurrentState = ATTACK;
            mAttackTimer = stats.duration;

            float reachX = stats.reachX;
            float reachY = stats.reachY;

            switch (mComboHit) {
            case 1: reachX *= 1.15f; break;
            case 2: reachX *= 1.3f; reachY *= 1.2f; break;
            }

            if (IsKeyDown(KEY_UP)) {
                mAttackDir = 1;
                mAttackArea = { mPosition.x - (reachY - mSize.x) / 2, mPosition.y - reachX, reachY, reachX };
            }
            else if (IsKeyDown(KEY_DOWN) && !mIsGrounded) {
                mAttackDir = 2;
                mAttackArea = { mPosition.x - (reachY - mSize.x) / 2, mPosition.y + mSize.y, reachY, reachX };
            }
            else {
                mAttackDir = 0;
                float yOffset = (mSize.y - reachY) / 2;
                if (mIsFacingRight) mAttackArea = { mPosition.x + mSize.x, mPosition.y + yOffset, reachX, reachY };
                else mAttackArea = { mPosition.x - reachX, mPosition.y + yOffset, reachX, reachY };
            }

            mComboHit++;
            if (mComboHit > 2) mComboHit = 0;
            mComboTimer = COMBO_WINDOW;
        }
    }

    void applyPhysics(float deltaTime) {
        if (!mIsDashing && !mIsClimbing) {
            mVelocity.y += mGravity * deltaTime;
        }

        // WAŻNE: Nie ruszamy tu mPosition.x/y!
        // Robi to funkcja ResolveMapCollision w scene_test_room.h
        // Dzięki temu nie ma konfliktu i podwójnego ruchu.
    }

public:

    void onCollision(Entity* pOther) override {
        // Kolizje ze ścianami obsługuje teraz ResolveMapCollision (Axis Separating)
        // Ta funkcja służy tylko do interakcji z innymi Entity (np. przeciwnicy, pociski)

        /* if (pOther->mType == ENEMY) {
             takeDamage(1);
        }
        */
    }

    //funkcja rysujaca - DODANO CONST
    void draw() const override {
        if (mStamina < mMaxStamina) {
            float barWidth = 20.0f;
            float barHeight = 2.0f;
            float barX = mPosition.x + mSize.x / 2 - barWidth / 2;
            float barY = mPosition.y - 10.0f;
            float percent = mStamina / mMaxStamina;
            Color color = ColorAlpha(RED, percent);
            if (percent < 0.50f) DrawRectangle(barX, barY, barWidth * percent, barHeight, color);
        }

        for (const auto& ghost : mGhosts) {
            float ghostWidth = ghost.facingRight ? ghost.frameRec.width : -ghost.frameRec.width;
            Rectangle source = { ghost.frameRec.x, ghost.frameRec.y, ghostWidth, ghost.frameRec.height };
            Vector2 drawPos = { (float)floor(ghost.position.x - 3), (float)floor(ghost.position.y - 2) };
            Rectangle dest = { drawPos.x, drawPos.y, 16.0f, 16.0f };
            DrawTexturePro(mTexture, source, dest, { 0,0 }, 0.0f, Fade(WHITE, ghost.alpha));
        }

        if (mWingsActive) {
            float wingW = 32.0f;
            float wingH = 16.0f;
            Rectangle source = { mWingsFrame * wingW, 0.0f, mIsFacingRight ? wingW : -wingW, wingH };
            Vector2 playerCenter = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };
            Vector2 wingsPos = { playerCenter.x - (wingW / 2.0f), playerCenter.y - (wingH / 2.0f) };
            wingsPos.y += 1.0f;
            DrawTextureRec(mWingsTexture, source, wingsPos, Fade(WHITE, 0.5f));
        }

        float sourceWidth = mIsFacingRight ? mFrameRec.width : -mFrameRec.width;
        Rectangle source = { mFrameRec.x, mFrameRec.y, sourceWidth, mFrameRec.height };

        if (mIsAttacking) {
            int slashFrame = 0;
            if (mAttackTimer < 0.2f) slashFrame = 1;
            if (mAttackTimer < 0.1f) slashFrame = 2;

            float spriteSize = 64.0f;
            float rotation = 0.0f;
            float scale = 1.0f;
            Vector2 offset = { 0.0f,0.0f };
            Color slashColor = WHITE;

            Rectangle slashSource = { slashFrame * spriteSize, 0.0f, spriteSize, spriteSize };
            if (!mIsFacingRight) slashSource.width *= -1;

            Vector2 playerCenter = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };

            if (mAttackDir == 1) {
                offset = { 0.0f, -10.0f };
                rotation = mIsFacingRight ? -90.0f : 90.0f;
            }
            else if (mAttackDir == 2) {
                offset = { 0.0f,10.0f };
                rotation = mIsFacingRight ? 90.0f : -90.0f;
            }
            else {
                offset = { mIsFacingRight ? 10.0f : -10.0f, 0.0f };
                switch (mComboHit) {
                case 1: rotation = 20.0f; break;
                case 2: rotation = -25.0f; scale = 1.2f; slashColor = LIGHTGRAY; break;
                default: rotation = 0.0f; break;
                }
                if (!mIsFacingRight) rotation *= -1;
            }

            float finalSize = spriteSize * scale;
            Rectangle dest = { playerCenter.x + offset.x, playerCenter.y + offset.y, finalSize, finalSize };
            Vector2 origin = { spriteSize / 2.0f, spriteSize / 2.0f };
            DrawTexturePro(mSlashTexture, slashSource, dest, origin, rotation, slashColor);
        }

        Vector2 drawPos;
        drawPos.x = mPosition.x - 3;
        drawPos.y = mPosition.y - 2;

        if (mCurrentState == CLIMB || mCurrentState == WALL_SLIDE) {
            float wallOffset = 2.0f;
            mIsFacingRight ? drawPos.x += wallOffset : drawPos.x -= wallOffset;
        }

        Rectangle dest = { (float)floor(drawPos.x), (float)floor(drawPos.y), 16.0f, 16.0f };
        Color drawColor = WHITE;
        if (mInvincibilityTimer > 0.0f) {
            if ((int)(mInvincibilityTimer * 20.0f) % 2 == 0) drawColor = RED;
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
                if (mIsGrounded && mCurrentState != DASH) {
                    mCurrentState = TURN;
                    mCurrentFrameRelative = 0;
                    mFrameTimer = 0.0f;
                }
            }
        }

        AnimState newState = mCurrentState;
        bool isBusyTurning = (mCurrentState == TURN && mCurrentFrameRelative < mLengthFrames - 1);

        if (mIsAttacking) newState = ATTACK;
        else if (mIsDashing) newState = DASH;
        else if (mIsClimbing) newState = mCurrentState;
        else if (!mIsGrounded) {
            if (mVelocity.y < -50.0f) newState = JUMP_UP;
            else newState = JUMP_DOWN;
        }
        else {
            if (isBusyTurning) newState = TURN;
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
        case IDLE: mStartFrame = 12; mLengthFrames = 6; mLoopAnim = true; mFrameSpeed = 0.15f; break;
        case WALK: mStartFrame = 0; mLengthFrames = 6; mLoopAnim = true; mFrameSpeed = 0.1f; break;
        case JUMP_UP: mStartFrame = 6; mLengthFrames = 3; mLoopAnim = false; mFrameSpeed = 0.1f; break;
        case JUMP_DOWN: mStartFrame = 9; mLengthFrames = 3; mLoopAnim = false; mFrameSpeed = 0.1f; break;
        case DASH: mStartFrame = 6; mLengthFrames = 1; mLoopAnim = false; mFrameSpeed = 0.5f; break;
        case TURN: mStartFrame = 18; mLengthFrames = 3; mLoopAnim = false; mFrameSpeed = 0.06f; break;
        case CLIMB:
            mLoopAnim = true; mFrameSpeed = 0.15f;
            if (mVelocity.y < 0) { mStartFrame = 39; mLengthFrames = 2; }
            else if (mVelocity.y > 0) { mStartFrame = 41; mLengthFrames = 2; }
            else { mStartFrame = 39; mLengthFrames = 1; }
            break;
        case WALL_SLIDE: mStartFrame = 43; mLengthFrames = 2; mLoopAnim = true; mFrameSpeed = 0.15f; break;
        case ATTACK:
            mLoopAnim = false; mFrameSpeed = 0.06f;
            if (mAttackDir == 0) { mStartFrame = 22; mLengthFrames = 5; }
            else if (mAttackDir == 1) { mStartFrame = 28; mLengthFrames = 5; }
            else if (mAttackDir == 2) { mStartFrame = 34; mLengthFrames = 5; }
            break;
        }

        mFrameTimer += deltaTime;
        if (mFrameTimer >= mFrameSpeed) {
            mFrameTimer = 0.0f;
            mCurrentFrameRelative++;
            if (mCurrentFrameRelative >= mLengthFrames) {
                if (mLoopAnim) mCurrentFrameRelative = 0;
                else {
                    mCurrentFrameRelative = mLengthFrames - 1;
                    if (mCurrentState == TURN) {
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
                newGhost.alpha = 0.5f;
                mGhosts.push_back(newGhost);
            }
        }
        else {
            mGhostSpawnTimer = 0;
        }

        mGhosts.erase(std::remove_if(mGhosts.begin(), mGhosts.end(), [&](Ghost& g) {
            g.alpha -= 3.0f * deltaTime;
            return g.alpha <= 0.0f;
            }), mGhosts.end());
    }
};