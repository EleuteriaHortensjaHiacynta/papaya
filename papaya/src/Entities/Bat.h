#pragma once
#include "Entity.h"
#include "raylib.h"
#include <cmath>

class Bat : public Entity {
public:
    enum BatState {
        FLYING,         
        TARGETING,      
        CHARGING,       
        RECOVERING,     
        STUNNED         
    };

    BatState mState = FLYING;
    Texture2D mTexture;
    Entity* pTarget = nullptr;

    // Statystyki
    int mHealth = 2;
    float mFlySpeed = 30.0f;
    float mChargeSpeed = 250.0f;

    // Pozycja bazowa
    Vector2 mHomePosition;
    float mFlyRadius = 40.0f;
    float mFlyAngle = 0.0f;
    float mFlyAngleSpeed = 2.0f;

    // Wysokoœæ lotu
    float mBaseHeight;
    float mCurrentHeight;
    float mHoverAmplitude = 8.0f;
    float mHoverSpeed = 3.0f;
    float mHoverOffset = 0.0f;

    // Timery stanów
    float mStateTimer = 0.0f;
    float mTargetingTime = 0.8f;
    float mRecoveryTime = 1.5f;
    float mStunTime = 1.0f;

    // Szar¿a
    Vector2 mChargeDirection;
    Vector2 mChargeStartPos;
    float mMaxChargeDistance = 300.0f;
    float mChargedDistance = 0.0f;

    // Detekcja gracza
    float mDetectionRange = 150.0f;
    float mAttackCooldown = 0.0f;
    float mAttackCooldownTime = 2.0f;

    // Animacja
    int mCurrentFrame = 0;
    float mFrameTimer = 0.0f;
    float mFrameSpeed = 0.1f;
    const float SPRITE_SIZE = 16.0f;

    // Visual
    float mHurtTimer = 0.0f;
    bool mIsFacingRight = true;
    float mWarningAlpha = 0.0f;

    Bat(float x, float y, Texture2D tex, Entity* target)
        : Entity({ x, y }, ENEMY) {
        mTexture = tex;
        pTarget = target;
        mSize = { 14.0f, 12.0f };
        mHomePosition = { x, y };
        mBaseHeight = y;
        mCurrentHeight = y;

        // losowy pocz¹tkowy k¹t lotu
        mFlyAngle = GetRandomValue(0, 628) / 100.0f;
    }

    void update(float deltaTime) override {
        if (mHealth <= 0) return;

        if (mHurtTimer > 0) mHurtTimer -= deltaTime;
        if (mAttackCooldown > 0) mAttackCooldown -= deltaTime;

        switch (mState) {
        case FLYING:
            updateFlying(deltaTime);
            break;
        case TARGETING:
            updateTargeting(deltaTime);
            break;
        case CHARGING:
            updateCharging(deltaTime);
            break;
        case RECOVERING:
            updateRecovering(deltaTime);
            break;
        case STUNNED:
            updateStunned(deltaTime);
            break;
        }

        updateAnimation(deltaTime);
    }

    void updateFlying(float deltaTime) {
        // ruch ko³owy wokó³ punktu bazowego
        mFlyAngle += mFlyAngleSpeed * deltaTime;
        if (mFlyAngle > 2 * PI) mFlyAngle -= 2 * PI;

        float targetX = mHomePosition.x + cosf(mFlyAngle) * mFlyRadius;
        mPosition.x += (targetX - mPosition.x) * 2.0f * deltaTime;

		// hovering w pionie
        mHoverOffset += mHoverSpeed * deltaTime;
        float targetY = mBaseHeight + sinf(mHoverOffset) * mHoverAmplitude;
        mPosition.y += (targetY - mPosition.y) * 3.0f * deltaTime;

        // sprawdzamy obecnoœæ gracza w zasiêgu
        if (pTarget && mAttackCooldown <= 0) {
            float distToPlayer = getDistanceToPlayer();
            if (distToPlayer < mDetectionRange) {
                startTargeting();
            }
        }

        if (pTarget) {
            mIsFacingRight = (pTarget->mPosition.x > mPosition.x);
        }
    }

    void startTargeting() {
        mState = TARGETING;
        mStateTimer = 0.0f;
        mWarningAlpha = 0.0f;

        mChargeStartPos = mPosition;
    }

    void updateTargeting(float deltaTime) {
        mStateTimer += deltaTime;

        // pulsuj¹cy warning
        mWarningAlpha = (sinf(mStateTimer * 15.0f) + 1.0f) / 2.0f;

		// aktualizacja kierunku szar¿y w stronê gracza
        if (pTarget) {
            Vector2 toPlayer = {
                (pTarget->mPosition.x + pTarget->mSize.x / 2) - (mPosition.x + mSize.x / 2),
                (pTarget->mPosition.y + pTarget->mSize.y / 2) - (mPosition.y + mSize.y / 2)
            };
            float len = sqrtf(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y);
            if (len > 0) {
                mChargeDirection = { toPlayer.x / len, toPlayer.y / len };
            }
            mIsFacingRight = (toPlayer.x > 0);
        }

        // lekkie dr¿enie
        mPosition.x = mChargeStartPos.x + (GetRandomValue(-10, 10) / 10.0f) * (mStateTimer / mTargetingTime);
        mPosition.y = mChargeStartPos.y + (GetRandomValue(-10, 10) / 10.0f) * (mStateTimer / mTargetingTime);

        if (mStateTimer >= mTargetingTime) {
            startCharge();
        }
    }

    void startCharge() {
        mState = CHARGING;
        mChargedDistance = 0.0f;
        mChargeStartPos = mPosition;
        // kierunek ju¿ jest obliczony w targeting
    }

    void updateCharging(float deltaTime) {
        float moveDistance = mChargeSpeed * deltaTime;
        mPosition.x += mChargeDirection.x * moveDistance;
        mPosition.y += mChargeDirection.y * moveDistance;
        mChargedDistance += moveDistance;

        // sprawdzamy czy przelecia³ maksymalny dystans
        if (mChargedDistance >= mMaxChargeDistance) {
            startRecovery();
        }
    }

    void startRecovery() {
        mState = RECOVERING;
        mStateTimer = 0.0f;
        mAttackCooldown = mAttackCooldownTime;

        // nowa pozycja bazowa (miejsce koñca szar¿y)
        mHomePosition = mPosition;
        mBaseHeight = mPosition.y - 30.0f;
    }

    void updateRecovering(float deltaTime) {
        mStateTimer += deltaTime;

        // wzlot w górê
        float targetY = mBaseHeight;
        mPosition.y += (targetY - mPosition.y) * 3.0f * deltaTime;

        if (mStateTimer >= mRecoveryTime) {
            mState = FLYING;
            mBaseHeight = mPosition.y;
            mHomePosition = mPosition;
        }
    }

    void updateStunned(float deltaTime) {
        mStateTimer += deltaTime;

        // spadek w dó³
        mPosition.y += 50.0f * deltaTime;

        if (mStateTimer >= mStunTime) {
            mState = RECOVERING;
            mStateTimer = 0.0f;
        }
    }

    void updateAnimation(float deltaTime) {
        mFrameTimer += deltaTime;

        float speed = mFrameSpeed;
        if (mState == CHARGING) speed = 0.05f;
        if (mState == STUNNED) speed = 0.3f;

        if (mFrameTimer >= speed) {
            mFrameTimer = 0.0f;
            mCurrentFrame++;
            if (mCurrentFrame > 3) mCurrentFrame = 0;
        }
    }

    void onCollision(Entity* other) override {
        if (other->mType == WALL) {
            if (mState == CHARGING) {
				// przeciwnik uderzy³ w œcianê podczas szar¿y -> og³uszenie
                mState = STUNNED;
                mStateTimer = 0.0f;

                // lekkie odbicie
                mPosition.x -= mChargeDirection.x * 5.0f;
                mPosition.y -= mChargeDirection.y * 5.0f;
            }
        }
    }

    void takeDamage(int amount) {
        mHealth -= amount;
        mHurtTimer = 0.2f;

        if (mState == CHARGING) {
            // przerwana szar¿a
            startRecovery();
        }
    }

    float getDistanceToPlayer() {
        if (!pTarget) return 9999.0f;
        float dx = pTarget->mPosition.x - mPosition.x;
        float dy = pTarget->mPosition.y - mPosition.y;
        return sqrtf(dx * dx + dy * dy);
    }

    Rectangle getRect() const override {
        return { mPosition.x, mPosition.y, mSize.x, mSize.y };
    }

    void draw() const override {
        if (mHealth <= 0) return;

        // warning line podczas targetowania
        if (mState == TARGETING && pTarget) {
            Vector2 start = { mPosition.x + mSize.x / 2, mPosition.y + mSize.y / 2 };
            Vector2 end = {
                pTarget->mPosition.x + pTarget->mSize.x / 2,
                pTarget->mPosition.y + pTarget->mSize.y / 2
            };

            Color warningColor = Fade(RED, mWarningAlpha * 0.5f);
            DrawLineEx(start, end, 2.0f, warningColor);

            // warning indicator
            DrawText("!", mPosition.x + mSize.x / 2 - 3, mPosition.y - 15, 16,
                Fade(RED, mWarningAlpha));
        }

        // rysujemy nietoperza
        Rectangle source = {
            (float)mCurrentFrame * SPRITE_SIZE,
            0.0f,
            mIsFacingRight ? SPRITE_SIZE : -SPRITE_SIZE,
            SPRITE_SIZE
        };

        Color batColor = WHITE;
        if (mState == STUNNED) batColor = GRAY;
        if (mState == TARGETING) batColor = ORANGE;
        if (mState == CHARGING) batColor = RED;
        if (mHurtTimer > 0) batColor = RED;

        Vector2 drawPos = {
            mPosition.x - (SPRITE_SIZE - mSize.x) / 2,
            mPosition.y - (SPRITE_SIZE - mSize.y) / 2
        };

        DrawTextureRec(mTexture, source, drawPos, batColor);

        // Debug hitbox
        DrawRectangleLines(getRect().x, getRect().y, getRect().width, getRect().height, PURPLE);
    }
};