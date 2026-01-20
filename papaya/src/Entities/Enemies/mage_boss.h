#pragma once
#include "../Entity.h"
#include "../Player/Player.h"
#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

struct Fireball {
    Vector2 position = { 0, 0 };
    Vector2 velocity = { 0, 0 };
    bool active = false;
    float radius = 6.0f;
    float rotation = 0.0f;
    int animFrame = 0;
    float animTimer = 0.0f;
};

class MageBoss : public Entity {
public:
    enum BossState {
        INACTIVE,
        APPEARING,
        IDLE,
        CAST_FIREBALL,
        LASER_CHARGE,
        LASER_FIRE,
        VULNERABLE,
        TELEPORT_OUT,
        TELEPORT_IN,
        AOE_ATTACK,
        DYING
    };

    BossState mState = INACTIVE;
    Texture2D mTexture;
    Vector2 mVelocity = { 0, 0 };
    Entity* pTarget = nullptr;

    Rectangle mArenaBounds = { 0, 0, 0, 0 };

    // Stats
    int mHealth = 70;
    int mMaxHealth = 70;
    float mFloatSpeed = 2.5f;
    float mFloatOffset = 0.0f;
    float mStartY = 0.0f;
    float mMoveSpeed = 70.0f;

    // Timers
    float mStateTimer = 0.0f;
    float mIdleTimeBase = 1.5f;
    float mLaserChargeTimeBase = 2.0f;
    float mLaserFireTimeBase = 1.8f;
    float mVulnerableTime = 2.5f;

    float mIdleTime = 1.5f;
    float mLaserChargeTime = 2.0f;
    float mLaserFireTime = 1.8f;

    // Laser
    float mLaserAngle = 0.0f;
    float mLaserTrackSpeedChargeBase = 3.0f;
    float mLaserTrackSpeedFireBase = 0.6f;
    float mLaserTrackSpeedCharge = 3.0f;
    float mLaserTrackSpeedFire = 0.6f;
    bool mLaserInitialized = false;
    Vector2 mLaserStart = { 0, 0 };
    Vector2 mLaserEnd = { 0, 0 };
    float mLaserWidth = 8.0f;

    bool mDualLaser = false;
    float mLaserAngle2 = 0.0f;
    Vector2 mLaserEnd2 = { 0, 0 };

    // Fireballs
    std::vector<Fireball> mFireballs;
    static const int MAX_FIREBALLS = 32;
    float mFireballSpeed = 180.0f;
    int mFireballCountBase = 2;
    int mFireballCount = 2;
    float mFireballSpread = 25.0f;
    bool mHasFireballCast = false;
    bool mChooseFireball = false;

    int mFireballBurstCount = 1;
    int mFireballBurstCountEnraged = 3;
    float mFireballBurstDelay = 0.25f;
    int mCurrentBurst = 0;
    float mBurstTimer = 0.0f;

    // Teleport
    float mTeleportationDuration = 0.4f;
    Vector2 mTeleportTarget = { 0, 0 };
    float mTeleportAlpha = 1.0f;

    // AoE
    float mAoeRadius = 0.0f;
    float mAoeMaxRadius = 80.0f;
    float mAoeDuration = 0.6f;
    float mAoeWarningTime = 1.1f;
    bool mAoeDealtDamage = false;

    // Enrage
    bool mIsEnraged = false;
    float mEnrageThreshold = 0.6f;
    float mEnrageSpeedMultiplier = 1.8f;
    Color mEnrageColor = { 255, 80, 80, 255 };

    // Animation
    int mCurrentFrame = 0;
    float mFrameTimer = 0.0f;
    static constexpr float SPRITE_SIZE = 32.0f;

    bool mIsFacingRight = false;
    float mHurtTimer = 0.0f;

    int mDeathFrameStart = 17;
    int mDeathFrameEnd = 20;
    float mActivationDistance = 150.0f;

    Vector2 mPrevPosition = { 0, 0 };
    int mAttackCounter = 0;
    bool mTeleportAttack = false;

    // Optimization helpers
    Vector2 mCachedCenter = { 0, 0 };
    float mCachedSinTime = 0.0f;
    int mUpdateCounter = 0;

    MageBoss(float x, float y, Texture tex, Entity* target)
        : Entity({ x, y }, MAGE_BOSS) {
        mPosition = { x, y };
        mTexture = tex;
        pTarget = target;
        mSize = { 32.0f, 38.0f };
        mState = INACTIVE;
        mPrevPosition = mPosition;
        mStartY = y;
        mCachedCenter = { x + mSize.x / 2, y + mSize.y / 2 };

        mFireballs.reserve(MAX_FIREBALLS);
    }

    void setArenaBounds(float x, float y, float w, float h) {
        mArenaBounds = { x, y, w, h };
    }

    void checkEnrage() {
        if (mIsEnraged) return;

        float hpPercent = (float)mHealth / (float)mMaxHealth;

        if (hpPercent < mEnrageThreshold) {
            mIsEnraged = true;
            std::cout << "[BOSS] ENRAGED!" << std::endl;

            mIdleTime = mIdleTimeBase / mEnrageSpeedMultiplier;
            mLaserChargeTime = mLaserChargeTimeBase / mEnrageSpeedMultiplier;
            mLaserFireTime = mLaserFireTimeBase * 1.2f;
            mLaserTrackSpeedCharge = mLaserTrackSpeedChargeBase * mEnrageSpeedMultiplier;
            mLaserTrackSpeedFire = mLaserTrackSpeedFireBase * mEnrageSpeedMultiplier;

            mDualLaser = true;
            mFireballCount = 4;
            mFireballSpeed *= 1.3f;
            mFireballBurstCount = mFireballBurstCountEnraged;
            mMoveSpeed *= 1.3f;
            mAoeMaxRadius *= 1.2f;
            mAoeWarningTime *= 0.8f;
            mTeleportAttack = true;
        }
    }

    void takeDamage(int amount) {
        if (mState == DYING || mState == INACTIVE || !mActive) return;
        if (mState == TELEPORT_OUT || mState == TELEPORT_IN) return;

        mHealth -= amount;
        mHurtTimer = 0.2f;

        checkEnrage();

        if (mHealth <= 0) {
            mHealth = 0;
            mState = DYING;
            mCurrentFrame = mDeathFrameStart;
            mFrameTimer = 0.0f;
            mFireballs.clear();
        }
    }

    Vector2 chooseTeleportDestination(const std::vector<Entity*>& walls) {
        for (int attempt = 0; attempt < 8; attempt++) {
            Vector2 candidate = mPosition;

            float dirX = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
            float distX = (float)GetRandomValue(80, 180);
            float distY = (float)GetRandomValue(-50, 50);

            candidate.x += dirX * distX;
            candidate.y += distY;

            if (mArenaBounds.width > 0) {
                float margin = 12.0f;
                candidate.x = std::clamp(candidate.x,
                    mArenaBounds.x + margin,
                    mArenaBounds.x + mArenaBounds.width - margin - mSize.x);
                candidate.y = std::clamp(candidate.y,
                    mArenaBounds.y + margin,
                    mArenaBounds.y + mArenaBounds.height - margin - mSize.y);
            }

            Rectangle candidateRect = { candidate.x, candidate.y, mSize.x, mSize.y };
            bool valid = true;

            for (const auto* wall : walls) {
                Rectangle wallRect = { wall->mPosition.x, wall->mPosition.y, wall->mSize.x, wall->mSize.y };
                if (CheckCollisionRecs(candidateRect, wallRect)) {
                    valid = false;
                    break;
                }
            }

            if (valid && pTarget) {
                float dx = candidate.x - pTarget->mPosition.x;
                float dy = candidate.y - pTarget->mPosition.y;
                if (dx * dx + dy * dy < 2500.0f) valid = false;
            }

            if (valid) return candidate;
        }

        return mPosition;
    }

    void updateBossLogic(float deltaTime, std::vector<Entity*>& walls) {
        if (!mActive) return;

        mUpdateCounter++;
        mCachedCenter = { mPosition.x + mSize.x / 2, mPosition.y + mSize.y / 2 };

        if (mUpdateCounter % 3 == 0) {
            mCachedSinTime = sinf(mFloatOffset);
        }

        if (mState == DYING) {
            updateAnimation(deltaTime);
            return;
        }

        mPrevPosition = mPosition;

        // Levitation
        if (mState != TELEPORT_OUT && mState != TELEPORT_IN) {
            mFloatOffset += deltaTime * mFloatSpeed;

            if (mState != VULNERABLE && mState != LASER_FIRE && mState != AOE_ATTACK) {
                float targetY = mStartY + mCachedSinTime * 12.0f;
                mPosition.y += (targetY - mPosition.y) * 2.0f * deltaTime;
            }
        }

        mPosition.x += mVelocity.x * deltaTime;
        if (mHurtTimer > 0) mHurtTimer -= deltaTime;

        updateFireballs(deltaTime, walls);
        updateStateMachine(deltaTime, walls);
        updateAnimation(deltaTime);
    }

    void updateFireballs(float deltaTime, std::vector<Entity*>& walls) {
        for (int i = (int)mFireballs.size() - 1; i >= 0; i--) {
            Fireball& fb = mFireballs[i];
            if (!fb.active) {
                std::swap(mFireballs[i], mFireballs.back());
                mFireballs.pop_back();
                continue;
            }

            fb.position.x += fb.velocity.x * deltaTime;
            fb.position.y += fb.velocity.y * deltaTime;

            fb.animTimer += deltaTime;
            if (fb.animTimer > 0.1f) {
                fb.animTimer = 0.0f;
                fb.animFrame = (fb.animFrame + 1) & 3;
            }

            // Optimize collisions for distant fireballs
            bool checkCollision = true;
            float dxFromBoss = fb.position.x - mPosition.x;
            float dyFromBoss = fb.position.y - mPosition.y;
            float distSqFromBoss = dxFromBoss * dxFromBoss + dyFromBoss * dyFromBoss;

            if (distSqFromBoss > 250000.0f && (mUpdateCounter & 1)) {
                checkCollision = false;
            }

            if (checkCollision) {
                for (auto* wall : walls) {
                    Rectangle wallRect = { wall->mPosition.x, wall->mPosition.y, wall->mSize.x, wall->mSize.y };
                    if (CheckCollisionCircleRec(fb.position, fb.radius, wallRect)) {
                        fb.active = false;
                        break;
                    }
                }
            }

            if (distSqFromBoss > 1000000.0f) {
                fb.active = false;
            }
        }
    }

    void updateStateMachine(float deltaTime, std::vector<Entity*>& walls) {
        switch (mState) {
        case INACTIVE:
            mVelocity = { 0, 0 };
            if (pTarget) {
                float dx = mPosition.x - pTarget->mPosition.x;
                float dy = mPosition.y - pTarget->mPosition.y;
                if (dx * dx + dy * dy < mActivationDistance * mActivationDistance) {
                    mState = APPEARING;
                    mCurrentFrame = mDeathFrameEnd;
                    mFrameTimer = 0.0f;
                    mIsFacingRight = (pTarget->mPosition.x > mPosition.x);
                }
            }
            break;

        case APPEARING:
            mVelocity = { 0, 0 };
            break;

        case IDLE:
            updateIdleState(deltaTime);
            break;

        case CAST_FIREBALL:
            updateFireballState(deltaTime);
            break;

        case LASER_CHARGE:
            mStateTimer += deltaTime;
            mVelocity.x = 0;
            calculateLaserEnd(walls, deltaTime);
            if (mStateTimer >= mLaserChargeTime) {
                mState = LASER_FIRE;
                mStateTimer = 0.0f;
            }
            break;

        case LASER_FIRE:
            mStateTimer += deltaTime;
            mVelocity.x = 0;
            if (mUpdateCounter % 2 == 0) {
                calculateLaserEnd(walls, deltaTime * 2.0f);
            }
            if (mStateTimer > mLaserFireTime) {
                mState = VULNERABLE;
                mStateTimer = 0.0f;
                mLaserInitialized = false;
            }
            break;

        case VULNERABLE:
            mStateTimer += deltaTime;
            mVelocity.x = 0;
            if (mStateTimer >= mVulnerableTime) {
                mState = AOE_ATTACK;
                mStateTimer = 0.0f;
                mAoeRadius = 0.0f;
                mAoeDealtDamage = false;
            }
            break;

        case AOE_ATTACK:
            updateAoeState(deltaTime, walls);
            break;

        case TELEPORT_OUT:
            mStateTimer += deltaTime;
            mVelocity = { 0, 0 };
            mTeleportAlpha = 1.0f - (mStateTimer / mTeleportationDuration);
            if (mTeleportAlpha < 0) mTeleportAlpha = 0;

            if (mStateTimer >= mTeleportationDuration) {
                mPosition = mTeleportTarget;
                mStartY = mTeleportTarget.y;
                mState = TELEPORT_IN;
                mStateTimer = 0.0f;
            }
            break;

        case TELEPORT_IN:
            mStateTimer += deltaTime;
            mVelocity = { 0, 0 };
            mTeleportAlpha = mStateTimer / mTeleportationDuration;
            if (mTeleportAlpha > 1.0f) mTeleportAlpha = 1.0f;

            if (pTarget) {
                mIsFacingRight = (pTarget->mPosition.x > mPosition.x);
            }

            if (mStateTimer >= mTeleportationDuration) {
                mTeleportAlpha = 1.0f;
                mLaserInitialized = false;

                if (mTeleportAttack && mIsEnraged) {
                    if (GetRandomValue(0, 1) == 0) {
                        mState = CAST_FIREBALL;
                        mHasFireballCast = false;
                        mCurrentBurst = 0;
                        mBurstTimer = 0.0f;
                    }
                    else {
                        mState = LASER_CHARGE;
                    }
                }
                else {
                    mState = IDLE;
                }
                mStateTimer = 0.0f;
            }
            break;

        case DYING:
            break;
        }
    }

    void updateIdleState(float deltaTime) {
        mStateTimer += deltaTime;

        if (pTarget) {
            float distToPlayer = pTarget->mPosition.x - mPosition.x;
            mIsFacingRight = (distToPlayer > 0);

            float chaseDistance = mIsEnraged ? 120.0f : 150.0f;
            if (fabsf(distToPlayer) > chaseDistance) {
                mVelocity.x = (distToPlayer > 0 ? 1.0f : -1.0f) * mMoveSpeed;
            }
            else {
                mVelocity.x = 0;
            }
        }

        if (mStateTimer >= mIdleTime) {
            mStateTimer = 0;
            mVelocity.x = 0;
            mAttackCounter++;
            mChooseFireball = false;

            if (mIsEnraged) {
                int choice = GetRandomValue(0, 3);
                if (choice == 0) mChooseFireball = true;
                else {
                    mState = LASER_CHARGE;
                    if (mAttackCounter % 4 == 0) spawnFireballPattern();
                }
            }
            else {
                mChooseFireball = (GetRandomValue(0, 4) < 2);
                if (!mChooseFireball) mState = LASER_CHARGE;
            }

            if (mChooseFireball) {
                mState = CAST_FIREBALL;
                mHasFireballCast = false;
                mCurrentBurst = 0;
                mBurstTimer = 0.0f;
            }
        }
    }

    void updateFireballState(float deltaTime) {
        mStateTimer += deltaTime;
        mVelocity.x = 0;

        if (mStateTimer > 0.3f) {
            mBurstTimer += deltaTime;
            int maxBurst = mIsEnraged ? mFireballBurstCountEnraged : mFireballBurstCount;

            if (mCurrentBurst < maxBurst && mBurstTimer >= mFireballBurstDelay) {
                if ((int)mFireballs.size() < MAX_FIREBALLS - 5) {
                    spawnFireballPattern();
                    mCurrentBurst++;
                    mBurstTimer = 0.0f;
                }
            }
        }

        if (mCurrentBurst >= (mIsEnraged ? mFireballBurstCountEnraged : mFireballBurstCount)
            && mStateTimer > 0.6f) {
            mState = IDLE;
            mStateTimer = 0.0f;
            mCurrentBurst = 0;
        }
    }

    void updateAoeState(float deltaTime, std::vector<Entity*>& walls) {
        mStateTimer += deltaTime;
        mVelocity.x = 0;

        if (mStateTimer < mAoeWarningTime) {
            mAoeRadius = (mStateTimer / mAoeWarningTime) * mAoeMaxRadius;
        }
        else if (mStateTimer < mAoeWarningTime + mAoeDuration) {
            mAoeRadius = mAoeMaxRadius * 1.2f;
        }
        else {
            mState = TELEPORT_OUT;
            mStateTimer = 0.0f;
            mTeleportTarget = chooseTeleportDestination(walls);
            mTeleportAlpha = 1.0f;
            mAoeRadius = 0.0f;
            mCurrentFrame = mDeathFrameStart;
        }
    }

    void updateAnimation(float deltaTime) {
        mFrameTimer += deltaTime;
        float animSpeed = mIsEnraged ? 0.12f : 0.15f;

        if (mFrameTimer < animSpeed) return;
        mFrameTimer = 0.0f;

        switch (mState) {
        case INACTIVE: break;
        case APPEARING:
            if (mCurrentFrame > mDeathFrameStart) mCurrentFrame--;
            else {
                mState = IDLE;
                mCurrentFrame = 0;
                mStateTimer = 0.0f;
            }
            break;
        case DYING:
            if (mCurrentFrame < mDeathFrameEnd) mCurrentFrame++;
            else mActive = false;
            break;
        case TELEPORT_IN:
            if (mCurrentFrame > mDeathFrameStart) mCurrentFrame--;
            break;
        case TELEPORT_OUT:
            if (mCurrentFrame < mDeathFrameEnd) mCurrentFrame++;
            break;
        case IDLE:
            mCurrentFrame = (mCurrentFrame + 1) & 3;
            break;
        case CAST_FIREBALL:
        case LASER_CHARGE:
            mCurrentFrame++;
            if (mCurrentFrame < 9) mCurrentFrame = 9;
            if (mCurrentFrame > 12) mCurrentFrame = 9;
            break;
        case LASER_FIRE:
            mCurrentFrame = 12;
            break;
        case AOE_ATTACK:
            if (mCurrentFrame < 9) mCurrentFrame = 9;
            else if (mCurrentFrame >= 12) mCurrentFrame = 9;
            else mCurrentFrame++;
            break;
        case VULNERABLE:
            if (mCurrentFrame < 4) mCurrentFrame = 4;
            else if (mCurrentFrame > 8) mCurrentFrame = 4;
            else mCurrentFrame++;
            break;
        }
    }

    void spawnFireballPattern() {
        if (!pTarget) return;
        if ((int)mFireballs.size() >= MAX_FIREBALLS) return;

        Vector2 spawnPos = mCachedCenter;
        Vector2 playerCenter = {
            pTarget->mPosition.x + pTarget->mSize.x / 2,
            pTarget->mPosition.y + pTarget->mSize.y / 2
        };

        Vector2 playerVelocity = { 0, 0 };
        Player* player = dynamic_cast<Player*>(pTarget);
        if (player) playerVelocity = player->mVelocity;

        float dx = playerCenter.x - spawnPos.x;
        float dy = playerCenter.y - spawnPos.y;
        float distToPlayer = sqrtf(dx * dx + dy * dy);
        float flightTime = distToPlayer / mFireballSpeed;

        float predictionMult = mIsEnraged ? 0.7f : 0.5f;
        Vector2 predictedPos = {
            playerCenter.x + playerVelocity.x * flightTime * predictionMult,
            playerCenter.y + playerVelocity.y * flightTime * 0.4f
        };

        float baseAngle = atan2f(predictedPos.y - spawnPos.y, predictedPos.x - spawnPos.x);

        for (int i = 0; i < mFireballCount; i++) {
            if ((int)mFireballs.size() >= MAX_FIREBALLS) break;

            Fireball fb;
            fb.active = true;
            fb.position = spawnPos;

            float angle = baseAngle;
            if (mFireballCount > 1) {
                float spreadRad = mFireballSpread * DEG2RAD;
                float offset = -spreadRad / 2.0f + (spreadRad * i / (mFireballCount - 1));
                angle += offset;
            }

            fb.velocity = { cosf(angle) * mFireballSpeed, sinf(angle) * mFireballSpeed };
            fb.rotation = angle * RAD2DEG;
            mFireballs.push_back(fb);
        }
    }

    void calculateLaserEnd(std::vector<Entity*>& walls, float deltaTime) {
        if (!pTarget) return;

        mLaserStart = mCachedCenter;
        Vector2 targetCenter = {
            pTarget->mPosition.x + pTarget->mSize.x / 2,
            pTarget->mPosition.y + pTarget->mSize.y / 2
        };

        float targetAngle = atan2f(targetCenter.y - mLaserStart.y, targetCenter.x - mLaserStart.x);

        if (!mLaserInitialized) {
            mLaserAngle = targetAngle;
            mLaserInitialized = true;
        }

        float trackSpeed = (mState == LASER_FIRE) ? mLaserTrackSpeedFire : mLaserTrackSpeedCharge;
        float angleDiff = targetAngle - mLaserAngle;

        while (angleDiff > PI) angleDiff -= 2.0f * PI;
        while (angleDiff < -PI) angleDiff += 2.0f * PI;

        mLaserAngle += angleDiff * trackSpeed * deltaTime;

        mLaserEnd = raycastLaser(mLaserAngle, walls);

        if (mDualLaser) {
            mLaserAngle2 = mLaserAngle + PI;
            mLaserEnd2 = raycastLaser(mLaserAngle2, walls);
        }
    }

    Vector2 raycastLaser(float angle, std::vector<Entity*>& walls) {
        Vector2 dir = { cosf(angle), sinf(angle) };
        float maxDist = 800.0f;
        float step = 8.0f;

        for (float dist = 0; dist < maxDist; dist += step) {
            Vector2 point = {
                mLaserStart.x + dir.x * dist,
                mLaserStart.y + dir.y * dist
            };

            for (auto* wall : walls) {
                Rectangle wallRect = { wall->mPosition.x, wall->mPosition.y, wall->mSize.x, wall->mSize.y };
                if (CheckCollisionPointRec(point, wallRect)) {
                    return point;
                }
            }
        }

        return { mLaserStart.x + dir.x * maxDist, mLaserStart.y + dir.y * maxDist };
    }

    bool checkAoeCollision(Entity* player) {
        if (!player || mState != AOE_ATTACK) return false;
        if (mStateTimer < mAoeWarningTime) return false;
        if (mStateTimer >= mAoeWarningTime + mAoeDuration) return false;
        if (mAoeDealtDamage) return false;

        Vector2 playerCenter = {
            player->mPosition.x + player->mSize.x / 2,
            player->mPosition.y + player->mSize.y / 2
        };

        float dx = playerCenter.x - mCachedCenter.x;
        float dy = playerCenter.y - mCachedCenter.y;

        return (dx * dx + dy * dy) <= (mAoeMaxRadius * mAoeMaxRadius);
    }

    bool checkLaserCollision(Entity* player) {
        if (mState != LASER_FIRE) return false;

        Vector2 playerCenter = {
            player->mPosition.x + player->mSize.x / 2,
            player->mPosition.y + player->mSize.y / 2
        };

        bool hit1 = CheckCollisionCircleLine(playerCenter, player->mSize.x / 2, mLaserStart, mLaserEnd);
        bool hit2 = mDualLaser && CheckCollisionCircleLine(playerCenter, player->mSize.x / 2, mLaserStart, mLaserEnd2);

        return hit1 || hit2;
    }

    void update(float deltaTime) override {}

    Rectangle getRect() override {
        return { mPosition.x + 10, mPosition.y + 10, mSize.x - 20, mSize.y - 10 };
    }

    void onCollision(Entity* pOther) override {
        if (mState == DYING) return;

        if (pOther->mType == WALL) {
            Rectangle otherRect = { pOther->mPosition.x, pOther->mPosition.y, pOther->mSize.x, pOther->mSize.y };
            float myCenterX = mPosition.x + mSize.x / 2.0f;
            float myCenterY = mPosition.y + mSize.y / 2.0f;
            float wallCenterX = otherRect.x + otherRect.width / 2.0f;
            float wallCenterY = otherRect.y + otherRect.height / 2.0f;

            float dx = fabsf(myCenterX - wallCenterX);
            float dy = fabsf(myCenterY - wallCenterY);

            float minDistX = (mSize.x / 2.0f) + (otherRect.width / 2.0f);
            float minDistY = (mSize.y / 2.0f) + (otherRect.height / 2.0f);

            float overlapX = minDistX - dx;
            float overlapY = minDistY - dy;

            if (overlapX < overlapY) {
                mPosition.x += (myCenterX < wallCenterX) ? -overlapX : overlapX;
            }
            else {
                mPosition.y += (myCenterY < wallCenterY) ? -overlapY : overlapY;
            }
        }
    }

    void draw() override {
        if (mState == INACTIVE || !mActive) return;

        // Draw AOE
        if (mState == AOE_ATTACK) {
            if (mStateTimer < mAoeWarningTime) {
                if (mAoeRadius > 0) {
                    Color warningColor = mIsEnraged ? ORANGE : RED;
                    DrawCircleLinesV(mCachedCenter, mAoeRadius, warningColor);
                    if ((mUpdateCounter / 3) & 1) {
                        DrawCircleV(mCachedCenter, 10.0f, warningColor);
                    }
                }
            }
            else if (mStateTimer < mAoeWarningTime + mAoeDuration) {
                float progress = (mStateTimer - mAoeWarningTime) / mAoeDuration;
                float alpha = 1.0f - progress;
                Color explosionColor = mIsEnraged ? ORANGE : RED;
                DrawCircleV(mCachedCenter, mAoeMaxRadius, Fade(explosionColor, alpha * 0.6f));
                DrawCircleLinesV(mCachedCenter, mAoeMaxRadius, Fade(WHITE, alpha));
            }
        }

        // Draw Fireballs
        for (const auto& fb : mFireballs) {
            if (!fb.active) continue;

            int spriteIndex = 13 + fb.animFrame;
            Rectangle source = { spriteIndex * SPRITE_SIZE, 0.0f, SPRITE_SIZE, SPRITE_SIZE };
            Rectangle dest = { fb.position.x, fb.position.y, SPRITE_SIZE, SPRITE_SIZE };
            Vector2 origin = { SPRITE_SIZE / 2.0f, SPRITE_SIZE / 2.0f };

            if (mIsEnraged) {
                DrawCircleV(fb.position, fb.radius * 1.5f, Fade(ORANGE, 0.3f));
            }
            DrawTexturePro(mTexture, source, dest, origin, fb.rotation, WHITE);
        }

        // Draw Laser
        if (mState == LASER_CHARGE) {
            if ((mUpdateCounter / 3) & 1) {
                Color chargeColor = mIsEnraged ? Fade(ORANGE, 0.5f) : Fade(SKYBLUE, 0.5f);
                DrawLineEx(mLaserStart, mLaserEnd, 2.0f, chargeColor);
                if (mDualLaser) {
                    DrawLineEx(mLaserStart, mLaserEnd2, 2.0f, chargeColor);
                }
            }
        }
        else if (mState == LASER_FIRE) {
            Color outer = mIsEnraged ? ORANGE : SKYBLUE;
            Color inner = mIsEnraged ? RED : BLUE;

            DrawLineEx(mLaserStart, mLaserEnd, mLaserWidth, outer);
            DrawLineEx(mLaserStart, mLaserEnd, mLaserWidth * 0.5f, inner);
            DrawLineEx(mLaserStart, mLaserEnd, 2.0f, WHITE);
            DrawCircleV(mLaserEnd, 4.0f, RAYWHITE);

            if (mDualLaser) {
                DrawLineEx(mLaserStart, mLaserEnd2, mLaserWidth, outer);
                DrawLineEx(mLaserStart, mLaserEnd2, mLaserWidth * 0.5f, inner);
                DrawLineEx(mLaserStart, mLaserEnd2, 2.0f, WHITE);
                DrawCircleV(mLaserEnd2, 4.0f, RAYWHITE);
            }
        }

        // Draw Boss
        Vector2 drawPos = {
            mPosition.x - (SPRITE_SIZE - mSize.x) / 2,
            mPosition.y - (SPRITE_SIZE - mSize.y) / 2
        };

        if (mState == TELEPORT_OUT || mState == TELEPORT_IN) {
            drawPos.y += (1.0f - mTeleportAlpha) * -20.0f;
        }

        Rectangle source = {
            mCurrentFrame * SPRITE_SIZE,
            0.0f,
            mIsFacingRight ? SPRITE_SIZE : -SPRITE_SIZE,
            SPRITE_SIZE
        };

        Color bossColor = WHITE;
        if (mIsEnraged) bossColor = mEnrageColor;
        if (mState == VULNERABLE) bossColor = GRAY;
        if (mHurtTimer > 0) bossColor = RED;
        if (mState == TELEPORT_OUT || mState == TELEPORT_IN) {
            bossColor = Fade(bossColor, mTeleportAlpha);
        }

        if (mIsEnraged && mState != TELEPORT_OUT && mState != TELEPORT_IN) {
            if ((mUpdateCounter / 4) & 1) {
                DrawCircleV(mCachedCenter, mSize.x * 0.7f, Fade(ORANGE, 0.2f));
            }
        }

        DrawTextureRec(mTexture, source, drawPos, bossColor);
    }
};