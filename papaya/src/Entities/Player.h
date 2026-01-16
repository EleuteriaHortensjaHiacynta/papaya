#pragma once
#include "raylib.h"
#include <vector>
#include "Entity.h"
#include <cmath>

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

    // Fizyka
    float mMaxSpeed = 100.0f;
    float mAcceleration = 810.0f;
    float mFriction = 790.0f;
    float mGravity = 500.0f;
    float mJumpForce = 200.0f;
    float mRecoilFriction = 60.0f;
    float mDashSpeed = 360.0f;

    // Double Jump
    int mJumpCount = 0;
    int mMaxJumps = 2;

    // Dash
    float mDashTimer = 0.0f;
    float mDashDuration = 0.15f;
    float mMomentumTimer = 0.0f;
    bool mIsDashing = false;
    bool mCanDash = true;
    const float WAVE_DASH_BOOST_SPEED = 1.3f;
    float mDashCooldownTimer = 0.0f;
    const float DASH_COOLDOWN = 0.5f;

    // Climb
    bool mTouchingWall = false;
    int mWallDir = 0;
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
            return { 2.0f, 30.0f, 15.0f, 0.25f, 200.0f, -200.0f, PURPLE };
        case SPEAR:
            return { 1.5f, 40.0f, 10.0f, 0.4f, 150.0f, -180.0f, BLUE };
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

    // ============================================
    // COMBO ATTACK SYSTEM
    // ============================================
    float mAttackTimer = 0.0f;
    float mAttackDuration = 0.33f;
    bool mIsAttacking = false;
    int mAttackDir = 0;
    int mAttackDamage = 1;
    Rectangle mAttackArea = { 0, 0, 0, 0 };

    int mComboCount = 0;
    float mComboWindowTimer = 0.0f;
    const float COMBO_WINDOW = 0.45f;
    bool mComboWindowOpen = false;

    // Stan
    bool mIsGrounded = false;
    bool mIsFacingRight = true;
    bool mIsJumping = false;
    bool mIsPogoJump = false;
    bool mIsClimbing = false;

    AnimState mCurrentState = IDLE;

    // GRAFIKA I ANIMACJA
    Rectangle mFrameRec;
    Texture2D mSlashTexture;
    float mSlashTimer = 0.0f;

    int mStartFrame = 0;
    int mLengthFrames = 0;
    int mCurrentFrameRelative = 0;
    float mFrameSpeed = 0.1f;
    float mFrameTimer = 0.0f;
    bool mLoopAnim = true;

    // Skrzydła
    Texture2D mWingsTexture;
    bool mWingsActive = false;
    int mWingsFrame = 0;
    float mWingsTimer = 0.0f;
    const int WINGS_NUM_FRAMES = 6;
    const float WINGS_SPEED = 0.08f;

    // Duszki
    std::vector<Ghost> mGhosts;
    float mGhostSpawnTimer = 0.0f;

    // Lista trafionych wrogów (w jednym ataku)
    std::vector<Entity*> mHitEntities;

    // Konstruktor
    Player(float startX, float startY) : Entity({ startX, startY }, PLAYER) {
        mPosition = { startX, startY };
        mStartPosition = { startX, startY };
        mPrevPosition = mPosition;
        mVelocity = { 0, 0 };
        mSlashTexture = LoadTexture("assets/slash.png");
        mWingsTexture = LoadTexture("assets/jump_wings.png");
        mWingsActive = false;
        mFrameRec = { 0.0f, 0.0f, 16.0f, 16.0f };
        mSize = { 10.0f, 14.0f };
    }

    ~Player() {
        UnloadTexture(mSlashTexture);
        UnloadTexture(mWingsTexture);
    }

    void update(float deltaTime) override {
        if (mHealth <= 0) { respawn(); return; }
        if (mPosition.y > 800.0f) { respawn(); return; }

        mPrevPosition = mPosition;

        // *** RESET FLAG KOLIZJI ***
        mTouchingWall = false;
        mWallDir = 0;

        updateTimers(deltaTime);
        handleDash(deltaTime);
        handleMovement(deltaTime);
        handleJump(deltaTime);
        handleAttack(deltaTime);
        applyPhysics(deltaTime);

        // mTouchingWall będzie ustawione przez onCollision/handleWallCollision

        updateStamina(deltaTime);
        handleGhosts(deltaTime);
        updateWings(deltaTime);
        handleSacrifice();
    }

    void lateUpdate(float deltaTime) {
        handleWallMovementPostCollision(deltaTime);
        updateAnimation(deltaTime);
    }

    void handleWallMovementPostCollision(float deltaTime) {
        if (!mTouchingWall || mStamina <= 0) {
            mIsClimbing = false;
            return;
        }

        // *** NOWE: Jeśli gracz właśnie skoczył, nie nadpisuj velocity ***
        if (mVelocity.y < -100.0f) {
            mIsClimbing = false;
            return;
        }

        bool isClimbingInput = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W) ||
            IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
        bool isGrippedInput = false;

        if (mWallDir == 1 && (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))) {
            isGrippedInput = true;
        }
        else if (mWallDir == -1 && (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))) {
            isGrippedInput = true;
        }

        if (mIsGrounded && !IsKeyDown(KEY_UP) && !IsKeyDown(KEY_W)) {
            mIsClimbing = false;
            return;
        }

        if (isClimbingInput && isGrippedInput) {
            mCurrentState = CLIMB;
            mIsClimbing = true;

            float climbDir = 0.0f;
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) climbDir = -1.0f;
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) climbDir = 1.0f;

            mStamina -= COST_CLIMB * deltaTime;
            mStaminaRegenDelay = STAMINA_DELAY_TIME;

            float climbSpeed = 50.0f;
            mVelocity.y = climbDir * climbSpeed;
            mVelocity.x = 0;
        }
        else if (!mIsGrounded && isGrippedInput) {
            mStamina -= (COST_CLIMB * 0.5f) * deltaTime;
            mStaminaRegenDelay = STAMINA_DELAY_TIME;

            mCurrentState = WALL_SLIDE;
            mIsClimbing = true;
            mVelocity.y = 30.0f;
            mVelocity.x = 0;
        }
        else {
            mIsClimbing = false;
        }
    }

    // Sprawdza czy już trafiliśmy danego wroga w tym ataku
    bool hasHit(Entity* enemy) {
        for (Entity* e : mHitEntities) {
            if (e == enemy) return true;
        }
        return false;
    }

    // Zwraca obrażenia zależne od combo
    int getAttackDamage() {
        switch (mComboCount) {
        case 0: return 1;
        case 1: return 1;
        case 2: return 2;  // Finisher!
        default: return 1;
        }
    }

    Rectangle getRect() override {
        return { mPosition.x, mPosition.y, mSize.x, mSize.y };
    }

    // *** KOLIZJE Z ENTITY (wrogowie, ściany jako Entity) ***
    void onCollision(Entity* pOther) override {
        if (pOther->mType == WALL) {
            Rectangle wallRect = pOther->getRect();
            handleWallCollision(wallRect);
        }
        // Możesz dodać inne typy kolizji tutaj
    }

    void draw() override {
        // Stamina bar
        if (mStamina < mMaxStamina) {
            float barWidth = 20.0f;
            float barHeight = 2.0f;
            float barX = mPosition.x + mSize.x / 2 - barWidth / 2;
            float barY = mPosition.y - 10.0f;

            float percent = mStamina / mMaxStamina;
            Color color = ColorAlpha(RED, percent);

            if (percent < 0.50f) {
                DrawRectangle((int)barX, (int)barY, (int)(barWidth * percent), (int)barHeight, color);
            }
        }

        // Duszki
        for (const auto& ghost : mGhosts) {
            float ghostWidth = ghost.facingRight ? ghost.frameRec.width : -ghost.frameRec.width;
            Rectangle source = { ghost.frameRec.x, ghost.frameRec.y, ghostWidth, ghost.frameRec.height };
            Vector2 drawPos = { std::floor(ghost.position.x - 3), std::floor(ghost.position.y - 2) };
            Rectangle dest = { drawPos.x, drawPos.y, 16.0f, 16.0f };
            DrawTexturePro(mTexture, source, dest, { 0, 0 }, 0.0f, Fade(WHITE, ghost.alpha));
        }

        // Skrzydła
        if (mWingsActive) {
            float wingW = 32.0f;
            float wingH = 16.0f;
            Rectangle source = { mWingsFrame * wingW, 0.0f, mIsFacingRight ? wingW : -wingW, wingH };
            Vector2 playerCenter = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };
            Vector2 wingsPos = { playerCenter.x - (wingW / 2.0f), playerCenter.y - (wingH / 2.0f) + 1.0f };
            DrawTextureRec(mWingsTexture, source, wingsPos, Fade(WHITE, 0.5f));
        }

        float sourceWidth = mIsFacingRight ? mFrameRec.width : -mFrameRec.width;
        Rectangle source = { mFrameRec.x, mFrameRec.y, sourceWidth, mFrameRec.height };

        // Slash z combo
        if (mIsAttacking) {
            WeaponStats stats = getCurrentWeaponStats();
            float attackProgress = 1.0f - (mAttackTimer / (stats.duration * (mComboCount == 2 ? 1.2f : (mComboCount == 1 ? 0.85f : 1.0f))));

            int slashFrame = 0;
            if (attackProgress > 0.3f) slashFrame = 1;
            if (attackProgress > 0.7f) slashFrame = 2;

            float spriteSize = 64.0f;
            if (mComboCount == 2) spriteSize = 80.0f;

            Rectangle slashSource = { slashFrame * 64.0f, 0.0f, 64.0f, 64.0f };
            if (!mIsFacingRight) slashSource.width *= -1;

            Vector2 playerCenter = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };

            // Rotacja zależna od combo
            float comboRotation = 0.0f;
            switch (mComboCount) {
            case 0: comboRotation = 15.0f; break;
            case 1: comboRotation = -25.0f; break;
            case 2: comboRotation = 40.0f; break;
            }
            if (!mIsFacingRight) comboRotation = -comboRotation;

            Vector2 offset = { 0.0f, 0.0f };
            float rotation = 0.0f;

            if (mAttackDir == 0) {
                float offsetDist = 10.0f;
                if (mComboCount == 2) offsetDist = 14.0f;
                offset = { mIsFacingRight ? offsetDist : -offsetDist, 0.0f };
                rotation = comboRotation;
            }
            else if (mAttackDir == 1) {
                offset = { 0.0f, -10.0f };
                rotation = mIsFacingRight ? -90.0f + comboRotation * 0.5f : 90.0f - comboRotation * 0.5f;
            }
            else if (mAttackDir == 2) {
                offset = { 0.0f, 10.0f };
                rotation = mIsFacingRight ? 90.0f + comboRotation * 0.5f : -90.0f - comboRotation * 0.5f;
            }

            Rectangle dest = { playerCenter.x + offset.x, playerCenter.y + offset.y, spriteSize, spriteSize };
            Vector2 origin = { spriteSize / 2.0f, spriteSize / 2.0f };

            // Kolor zależny od combo
            Color slashColor = WHITE;
            switch (mComboCount) {
            case 0: slashColor = WHITE; break;
            case 1: slashColor = { 200, 200, 255, 255 }; break;
            case 2: slashColor = { 255, 180, 50, 255 }; break;
            }

            if (mComboCount == 2 && attackProgress < 0.3f) {
                slashColor = { 255, 255, 180, 255 };
            }

            DrawTexturePro(mSlashTexture, slashSource, dest, origin, rotation, slashColor);

            // Finisher ghost effect
            if (mComboCount == 2) {
                Color ghostSlash = { 255, 150, 50, 80 };
                Rectangle ghostDest = dest;
                ghostDest.width *= 1.3f;
                ghostDest.height *= 1.3f;
                Vector2 ghostOrigin = { ghostDest.width / 2.0f, ghostDest.height / 2.0f };
                DrawTexturePro(mSlashTexture, slashSource, ghostDest, ghostOrigin, rotation - 15.0f, ghostSlash);
            }
        }

        // Gracz
        Vector2 drawPos;
        drawPos.x = mPosition.x - 3;
        drawPos.y = mPosition.y - 2;

        if (mCurrentState == CLIMB || mCurrentState == WALL_SLIDE) {
            float wallOffset = 2.0f;
            drawPos.x += mIsFacingRight ? wallOffset : -wallOffset;
        }

        Rectangle dest = { std::floor(drawPos.x), std::floor(drawPos.y), 16.0f, 16.0f };

        Color drawColor = WHITE;
        if (mInvincibilityTimer > 0.0f) {
            if ((int)(mInvincibilityTimer * 20.0f) % 2 == 0) {
                drawColor = RED;
            }
        }

        DrawTexturePro(mTexture, source, dest, { 0, 0 }, 0.0f, drawColor);

        // Combo indicator
        if (mIsAttacking || mComboWindowOpen) {
            Color indicatorColor = WHITE;
            const char* comboText = "1";
            switch (mComboCount) {
            case 0: comboText = "1"; indicatorColor = WHITE; break;
            case 1: comboText = "2"; indicatorColor = SKYBLUE; break;
            case 2: comboText = "3!"; indicatorColor = GOLD; break;
            }
            DrawText(comboText, (int)mPosition.x + 2, (int)mPosition.y - 18, 8, indicatorColor);

            if (mComboWindowOpen) {
                float progress = mComboWindowTimer / COMBO_WINDOW;
                DrawRectangle((int)mPosition.x - 3, (int)mPosition.y - 12, (int)(16.0f * progress), 2, YELLOW);
            }
        }
    }

private:

    void respawn() {
        mPosition = mStartPosition;
        mHealth = mMaxHealth;
        mVelocity = { 0, 0 };
        mStamina = mMaxStamina;
        mRecoilTimer = 0;
        mInvincibilityTimer = 0;
        mIsDashing = false;
        mIsAttacking = false;
        mComboCount = 0;
        mComboWindowOpen = false;
        mCurrentState = IDLE;
        mTouchingWall = false;
        mWallDir = 0;
        mIsClimbing = false;
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

        if (mVelocity.y > 0) {
            mIsPogoJump = false;
        }

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

        // Combo window
        if (mComboWindowOpen) {
            mComboWindowTimer -= deltaTime;
            if (mComboWindowTimer <= 0) {
                mComboWindowOpen = false;
                mComboCount = 0;
            }
        }

        // Attack timer
        if (mIsAttacking) {
            mAttackTimer -= deltaTime;

            if (mAttackTimer <= 0) {
                mIsAttacking = false;
                mCurrentState = IDLE;
                mAttackArea = { 0, 0, 0, 0 };
                mHitEntities.clear();

                // Otwórz okno combo (chyba że finisher)
                if (mComboCount < 2) {
                    mComboWindowOpen = true;
                    mComboWindowTimer = COMBO_WINDOW;
                }
                else {
                    mComboCount = 0;
                    mComboWindowOpen = false;
                }
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

    void handleMovement(float deltaTime) {
        if (mIsDashing || mIsClimbing) return;  // *** DODANO: || mIsClimbing ***

        float moveDir = 0.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) moveDir = 1.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) moveDir = -1.0f;

        float currentMaxSpeed = mMaxSpeed;
        float groundAccel = 2000.0f;
        float groundFriction = 3000.0f;
        float momentumDragGround = 1000.0f;
        float airAccel = 1200.0f;
        float airTurnAccel = 3000.0f;
        float airDrag = 500.0f;

        bool isSuperSpeed = fabsf(mVelocity.x) > currentMaxSpeed;

        if (mIsGrounded) {
            if (isSuperSpeed) {
                float drag = momentumDragGround;
                if ((mVelocity.x > 0 && moveDir > 0) || (mVelocity.x < 0 && moveDir < 0)) {
                    drag *= 0.5f;
                }
                mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, drag * deltaTime);
            }
            else {
                if (moveDir != 0) {
                    mVelocity.x = approach(mVelocity.x, moveDir * currentMaxSpeed, groundAccel * deltaTime);
                }
                else {
                    mVelocity.x = approach(mVelocity.x, 0, groundFriction * deltaTime);
                }
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
                mVelocity.y = -mJumpForce;
                float wallJumpKick = 130.0f;
                mVelocity.x = -mWallDir * wallJumpKick;
                mIsFacingRight = (mWallDir == -1);
                mIsClimbing = false;
                mJumpCount = 1;
                jumped = true;
            }
            else if (mCoyoteTimeCounter > 0) {
                mVelocity.y = -mJumpForce;
                mJumpCount = 1;
                jumped = true;
            }
            else if (mJumpCount < mMaxJumps) {
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

            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dirX = 1.0f;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dirX = -1.0f;
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) dirY = -1.0f;
            if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) dirY = 1.0f;

            if (dirX == 0.0f && dirY == 0.0f) {
                dirX = mIsFacingRight ? 1.0f : -1.0f;
            }

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
                if (mWingsFrame >= WINGS_NUM_FRAMES) {
                    mWingsActive = false;
                    mWingsFrame = 0;
                }
            }
        }
    }

    void handleAttack(float deltaTime) {
        bool canStartAttack = !mIsAttacking;

        if (IsKeyPressed(KEY_Z) && canStartAttack) {
            // Combo logic
            if (mComboWindowOpen) {
                mComboCount++;
                if (mComboCount > 2) mComboCount = 2;
            }
            else {
                mComboCount = 0;
            }

            mComboWindowOpen = false;

            WeaponStats stats = getCurrentWeaponStats();

            mIsAttacking = true;
            mCurrentState = ATTACK;

            // Czas trwania zależny od combo
            float durationMultiplier = 1.0f;
            switch (mComboCount) {
            case 0: durationMultiplier = 1.0f; break;
            case 1: durationMultiplier = 0.85f; break;
            case 2: durationMultiplier = 1.2f; break;
            }
            mAttackTimer = stats.duration * durationMultiplier;

            // Zasięg zależny od combo
            float reachX = stats.reachX;
            float reachY = stats.reachY;

            if (mComboCount == 2) {
                reachX *= 1.4f;
                reachY *= 1.3f;
            }

            // Hitbox
            if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
                mAttackDir = 1;
                mAttackArea = {
                    mPosition.x - (reachY - mSize.x) / 2,
                    mPosition.y - reachX,
                    reachY,
                    reachX
                };
            }
            else if ((IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) && !mIsGrounded) {
                mAttackDir = 2;
                mAttackArea = {
                    mPosition.x - (reachY - mSize.x) / 2,
                    mPosition.y + mSize.y,
                    reachY,
                    reachX
                };
            }
            else {
                mAttackDir = 0;
                float yOffset = (mSize.y - reachY) / 2;

                if (mIsFacingRight) {
                    mAttackArea = { mPosition.x + mSize.x, mPosition.y + yOffset, reachX, reachY };
                }
                else {
                    mAttackArea = { mPosition.x - reachX, mPosition.y + yOffset, reachX, reachY };
                }
            }

            // Impuls do przodu
            if (mAttackDir == 0 && mIsGrounded) {
                float pushForce = 25.0f;
                if (mComboCount == 2) pushForce = 50.0f;
                mVelocity.x += mIsFacingRight ? pushForce : -pushForce;
            }
        }
    }

    void applyPhysics(float deltaTime) {
        if (!mIsDashing && !mIsClimbing) {
            mVelocity.y += mGravity * deltaTime;
        }

        mIsGrounded = false;
        mPosition.x += mVelocity.x * deltaTime;
        mPosition.y += mVelocity.y * deltaTime;
    }

    // *** WSPÓLNA LOGIKA KOLIZJI ZE ŚCIANAMI ***
    void handleWallCollision(Rectangle wallRect) {
        float prevBottom = mPrevPosition.y + mSize.y;
        float prevRight = mPrevPosition.x + mSize.x;
        float prevLeft = mPrevPosition.x;

        // Kolizja od góry (stanie na podłodze)
        if (prevBottom <= wallRect.y + 2.0f) {
            mPosition.y = wallRect.y - mSize.y;
            mVelocity.y = 0;
            mIsGrounded = true;
            mCanDash = true;
            mIsJumping = false;
            mJumpCount = 0;

            // Wave-dash
            if (mIsDashing) {
                mIsDashing = false;
                float dashDir = 0.0f;
                if (mVelocity.x > 0.1f) dashDir = 1.0f;
                else if (mVelocity.x < -0.1f) dashDir = -1.0f;
                else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dashDir = 1.0f;
                else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dashDir = -1.0f;
                else dashDir = mIsFacingRight ? 1.0f : -1.0f;
                mVelocity.x = dashDir * mDashSpeed * WAVE_DASH_BOOST_SPEED;
            }
        }
        // Kolizja od dołu (uderzenie głową w sufit)
        else if (mPrevPosition.y >= wallRect.y + wallRect.height - 2.0f) {
            mPosition.y = wallRect.y + wallRect.height;
            mVelocity.y = 0;
        }
        // Kolizja boczna (ŚCIANY - dla wspinaczki!)
        else {
            if (prevRight <= wallRect.x + 5.0f) {
                // Ściana po prawej
                mPosition.x = wallRect.x - mSize.x;
                mTouchingWall = true;
                mWallDir = 1;
            }
            else if (prevLeft >= wallRect.x + wallRect.width - 5.0f) {
                // Ściana po lewej
                mPosition.x = wallRect.x + wallRect.width;
                mTouchingWall = true;
                mWallDir = -1;
            }
            mVelocity.x = 0;
        }
    }

    void updateAnimation(float deltaTime) {
        float moveInput = 0.0f;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) moveInput = 1.0f;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) moveInput = -1.0f;

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

        if (mIsAttacking) {
            newState = ATTACK;
        }
        else if (mIsDashing) {
            newState = DASH;
        }
        else if (mIsClimbing) {
            newState = mCurrentState;
        }
        else if (!mIsGrounded) {
            if (mVelocity.y < -50.0f) newState = JUMP_UP;
            else newState = JUMP_DOWN;
        }
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
            mLoopAnim = true; mFrameSpeed = 0.15f;
            if (mVelocity.y < 0) { mStartFrame = 39; mLengthFrames = 2; }
            else if (mVelocity.y > 0) { mStartFrame = 41; mLengthFrames = 2; }
            else { mStartFrame = 39; mLengthFrames = 1; }
            break;
        case WALL_SLIDE:
            mStartFrame = 43; mLengthFrames = 2; mLoopAnim = true; mFrameSpeed = 0.15f;
            break;
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
                if (mLoopAnim) {
                    mCurrentFrameRelative = 0;
                }
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

        if (mIsDashing || (isSuperSpeed && !isInRecoil)) {
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

        for (int i = 0; i < (int)mGhosts.size(); i++) {
            mGhosts[i].alpha -= 3.0f * deltaTime;
            if (mGhosts[i].alpha <= 0.0f) {
                mGhosts.erase(mGhosts.begin() + i);
                i--;
            }
        }
    }
};