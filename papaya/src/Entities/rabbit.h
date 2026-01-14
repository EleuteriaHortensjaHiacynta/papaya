#pragma once
#include "Entities/Entity.h"
#include "raylib.h"
#include <cmath>

class RabbitEnemy : public Entity {
public:
    Vector2 mVelocity = { 0, 0 };
    Vector2 mPrevPosition = { 0, 0 };

    enum RabbitState {
        RABBIT_IDLE,
        RABBIT_HOP,
        MORPHING,
        MONSTER_CHARGE,
        MONSTER_RUN,
        MONSTER_SLIDE,
        MONSTER_TURN,
        UNMORPHING,
        MONSTER_DYING,
    };

    RabbitState mState = RABBIT_IDLE;
    Texture2D mTexture;
    Entity* pTarget = nullptr;

    float mHopForceY = -150.0f;
    float mHopForceX = 80.0f;
    float mMonsterSpeed = 180.0f;
    float mGravity = 800.0f;
    int mHealth = 3;

    float mFollowTimer = 0.0f;
    float mTimeToMorph = 2.0f;
    float mUnmorphTimer = 0.0f;

    const float UNMORPH_TIME = 3.0f;
    const float SAFE_DISTANCE = 120.0f;
    const float MORPH_TRIGGER_DIST = 80.0f;

    //Vector2 mSize = RABBIT_SIZE;

    float mHopCooldown = 0.0f;
    float mHurtTimer = 0.0f;
    float mTurnCooldown = 0.0f;

    int mCurrentFrame = 0;
    float mFrameTimer = 0.0f;
    float mFrameSpeed = 0.15f;

    bool mIsFacingRight = false;
    bool mIsGrounded = false;
    bool mIsDead = false;
    bool mHitWallThisFrame = false;

    const float SPRITE_SIZE = 16.0f;
    const float TURN_DEAD_ZONE = 30.0f;
    const Vector2 MONSTER_SIZE = { 16.0f, 8.0f };
    const Vector2 RABBIT_SIZE = { 16.0f, 16.0f };

    RabbitEnemy(float x, float y, Texture tex, Entity* target)
        : Entity({ x, y }, ENEMY)
    {
        mPosition = { x, y };
        mPrevPosition = mPosition;
        mTexture = tex;
        pTarget = target;

        mSize = RABBIT_SIZE;
        mState = RABBIT_IDLE;
        mActive = true;

        mCurrentFrame = 0;
        mFrameTimer = 0.0f;

        mVelocity = { 0, 0 };
        mHopCooldown = 0.0f;
        mHurtTimer = 0.0f;
        mTurnCooldown = 0.0f;
        mFollowTimer = 0.0f;
        mUnmorphTimer = 0.0f;

        mIsFacingRight = false;
        mIsGrounded = false;
        mIsDead = false;
        mHitWallThisFrame = false;
    }

    void takeDamage(int amount) {
        if (mState == MONSTER_DYING || mIsDead) return;

        mHealth -= amount;
        mHurtTimer = 0.2f;
        mUnmorphTimer = 0.0f;

        if (mHealth > 0) {
            if (mState == RABBIT_IDLE || mState == RABBIT_HOP) {
                startMorphing();
            }
        }
        else {
            if (mSize.y != MONSTER_SIZE.y) {
                float oldHeight = mSize.y;
                mSize = MONSTER_SIZE;
                mPosition.y -= (mSize.y - oldHeight);
            }

            mState = MONSTER_DYING;
            mVelocity.x = 0;
            mCurrentFrame = 25;
            mFrameTimer = 0.0f;
        }
    }

    void startMorphing() {
        if (mState != MORPHING && mState < MONSTER_CHARGE) {
            mState = MORPHING;
            mCurrentFrame = 4;
            mVelocity.x = 0.0f;
            mUnmorphTimer = 0.0f;
        }
    }

    void startUnmorphing() {
        if (mState >= MONSTER_CHARGE && mState < UNMORPHING && mState != MONSTER_DYING) {
            mState = UNMORPHING;
            mCurrentFrame = 12;
            mVelocity.x = 0.0f;
            mFollowTimer = 0.0f;
            mUnmorphTimer = 0.0f;
        }
    }

    bool hasValidTarget() const {
        return pTarget != nullptr && pTarget->mActive;
    }

    void update(float deltaTime) override {
        mHitWallThisFrame = false;

        if (mTurnCooldown > 0) mTurnCooldown -= deltaTime;
        if (mHurtTimer > 0) mHurtTimer -= deltaTime;
        if (mHopCooldown > 0) mHopCooldown -= deltaTime;

        float distToPlayer = 0.0f;
        float absDist = 9999.0f;
        bool playerIsRight = mIsFacingRight;

        if (hasValidTarget()) {
            distToPlayer = pTarget->mPosition.x - mPosition.x;
            float dy = pTarget->mPosition.y - mPosition.y;
            absDist = std::sqrt(distToPlayer * distToPlayer + dy * dy);
            playerIsRight = (distToPlayer > 0);
        }

        if (!mIsDead && mState != MONSTER_DYING) {

            if (mState >= MONSTER_CHARGE && mState < UNMORPHING) {
                bool isCalm = hasValidTarget() && (absDist > SAFE_DISTANCE) && (mState != MONSTER_CHARGE);
                if (isCalm || !hasValidTarget()) {
                    mUnmorphTimer += deltaTime;
                    if (mUnmorphTimer >= UNMORPH_TIME) startUnmorphing();
                }
                else {
                    mUnmorphTimer = 0.0f;
                }
            }

            if (mState == MONSTER_RUN || mState == MONSTER_CHARGE) {
                if (hasValidTarget()) {
                    bool wrongDirection = (mIsFacingRight && distToPlayer < -20.0f) ||
                        (!mIsFacingRight && distToPlayer > 20.0f);

                    if (wrongDirection && mState != MONSTER_SLIDE) {
                        mState = MONSTER_SLIDE;
                        mCurrentFrame = 20;
                        mFrameTimer = 0.0f;
                    }
                }
            }

            switch (mState) {
            case RABBIT_IDLE:
                mVelocity.x = 0;
                mFollowTimer = 0.0f;
                if (hasValidTarget() && absDist < 650.0f) {
                    mState = RABBIT_HOP;
                    mHopCooldown = 0.5f;
                }
                break;

            case RABBIT_HOP:
                if (hasValidTarget()) mIsFacingRight = playerIsRight;
                if (mIsGrounded && mHopCooldown <= 0) {
                    mVelocity.y = mHopForceY;
                    mVelocity.x = (mIsFacingRight ? 1.0f : -1.0f) * mHopForceX;
                    mHopCooldown = 1.0f;
                }
                if (hasValidTarget() && absDist < MORPH_TRIGGER_DIST) {
                    mFollowTimer += deltaTime;
                    if (mFollowTimer >= mTimeToMorph) startMorphing();
                }
                else {
                    if (mFollowTimer > 0) mFollowTimer -= deltaTime * 0.5f;
                }
                if (!hasValidTarget() || absDist > 700.0f) {
                    mState = RABBIT_IDLE;
                }
                break;

            case MORPHING:
            case UNMORPHING:
            case MONSTER_TURN:
                mVelocity.x = 0;
                break;

            case MONSTER_CHARGE:
                if (mCurrentFrame == 13 && hasValidTarget()) mIsFacingRight = playerIsRight;
                break;

            case MONSTER_SLIDE:
                mVelocity.x *= 0.85f;
                if (fabs(mVelocity.x) < 5.0f) mVelocity.x = 0;
                break;

            case MONSTER_RUN:
                if (hasValidTarget()) {
                    mVelocity.x = (mIsFacingRight ? 1.0f : -1.0f) * mMonsterSpeed;
                }
                else {
                    mVelocity.x = 0;
                }
                break;
            default: break;
            }
        }

        mPrevPosition = mPosition;

        mVelocity.y += mGravity * deltaTime;
        mPosition.x += mVelocity.x * deltaTime;
        mPosition.y += mVelocity.y * deltaTime;

        if (mIsGrounded && mState != MONSTER_RUN && mState != MONSTER_CHARGE) {
            mVelocity.x *= 0.85f;
            if (fabs(mVelocity.x) < 5.0f) mVelocity.x = 0;
        }

        updateAnimation(deltaTime);
        mIsGrounded = false;
    }

    void updateAnimation(float deltaTime) {
        bool autoIncrement = (mState != UNMORPHING && mState != RABBIT_HOP && mState != MONSTER_SLIDE && !mIsDead);
        if (mState == MONSTER_DYING && !mIsDead) autoIncrement = true;

        if (autoIncrement) {
            mFrameTimer += deltaTime;
            if (mFrameTimer >= mFrameSpeed) {
                mFrameTimer = 0.0f;
                mCurrentFrame++;
            }
        }

        switch (mState) {
        case RABBIT_IDLE:
            if (mCurrentFrame > 1) mCurrentFrame = 0;
            break;

        case RABBIT_HOP:
            if (!mIsGrounded) {
                mCurrentFrame = (mVelocity.y < 0) ? 2 : 3;
            }
            else {
                mCurrentFrame = 0;
            }
            break;

        case MORPHING:
            if (mCurrentFrame < 4) mCurrentFrame = 4;
            if (mCurrentFrame > 12) {
                mState = MONSTER_CHARGE;
                float oldHeight = mSize.y;
                mSize = MONSTER_SIZE;
                mPosition.y -= (mSize.y - oldHeight);
                mCurrentFrame = 13;
                mFrameTimer = 0.0f;
            }
            break;

        case UNMORPHING:
            mFrameTimer += deltaTime;
            if (mFrameTimer >= mFrameSpeed) {
                mFrameTimer = 0.0f;
                mCurrentFrame--;
            }
            if (mCurrentFrame > 12) mCurrentFrame = 12;
            if (mCurrentFrame < 4) {
                mState = RABBIT_HOP;
                float oldHeight = mSize.y;
                mSize = RABBIT_SIZE;
                mPosition.y -= (mSize.y - oldHeight);
                mCurrentFrame = 0;
                mFrameTimer = 0.0f;
                mFollowTimer = 0.0f;
                mUnmorphTimer = 0.0f;
            }
            break;

        case MONSTER_CHARGE:
            if (mCurrentFrame < 13) mCurrentFrame = 13;
            if (mCurrentFrame > 16) {
                mState = MONSTER_SLIDE;
                mCurrentFrame = 20;
                mFrameTimer = 0.0f;
            }
            break;

        case MONSTER_SLIDE:
            mCurrentFrame = 20;
            mFrameTimer += deltaTime;
            if (mFrameTimer > 0.3f) {
                mState = MONSTER_TURN;
                mCurrentFrame = 21;
                mFrameTimer = 0.0f;
            }
            break;

        case MONSTER_TURN:
            if (mCurrentFrame < 21) mCurrentFrame = 21;
            if (mCurrentFrame > 24) {
                if (hasValidTarget()) {
                    float dx = pTarget->mPosition.x - mPosition.x;
                    mIsFacingRight = (dx > 0);
                }
                else {
                    mIsFacingRight = !mIsFacingRight;
                }
                mState = MONSTER_RUN;
                mCurrentFrame = 17;
                mFrameTimer = 0.0f;
                mTurnCooldown = 1.0f;
            }
            break;

        case MONSTER_RUN:
            if (mCurrentFrame < 17) mCurrentFrame = 17;
            if (mCurrentFrame > 18) mCurrentFrame = 17;
            if (mCurrentFrame >= 20) mCurrentFrame = 17;
            break;

        case MONSTER_DYING:
            if (mCurrentFrame < 25) mCurrentFrame = 25;
            if (mCurrentFrame > 28) {
                mCurrentFrame = 28;
                mIsDead = true;
            }
            break;
        }
    }

    void onCollision(Entity* pOther) override {
        if (pOther->mType != WALL) return;

        Rectangle myRect = getRect();
        Rectangle wallRect = pOther->getRect();

        float overlapLeft = (myRect.x + myRect.width) - wallRect.x;
        float overlapRight = (wallRect.x + wallRect.width) - myRect.x;
        float overlapTop = (myRect.y + myRect.height) - wallRect.y;
        float overlapBottom = (wallRect.y + wallRect.height) - myRect.y;

        float minOverlapX = (overlapLeft < overlapRight) ? overlapLeft : overlapRight;
        float minOverlapY = (overlapTop < overlapBottom) ? overlapTop : overlapBottom;

        if (minOverlapY < minOverlapX) {
            if (overlapTop < overlapBottom) {
                mPosition.y = wallRect.y - mSize.y;
                if (mVelocity.y > 0) mVelocity.y = 0;
                mIsGrounded = true;
            }
            else {
                mPosition.y = wallRect.y + wallRect.height;
                if (mVelocity.y < 0) mVelocity.y = 0;
            }
        }
        else {
            bool isJustAFloorSeam = (mPosition.y + mSize.y) <= (wallRect.y + 12.0f);

            if (isJustAFloorSeam) {
                mPosition.y = wallRect.y - mSize.y;
                if (mVelocity.y > 0) mVelocity.y = 0;
                mIsGrounded = true;
            }
            else {
                if (overlapLeft < overlapRight) mPosition.x = wallRect.x - mSize.x;
                else mPosition.x = wallRect.x + wallRect.width;

                mVelocity.x = 0;
                mHitWallThisFrame = true;

                if (!mIsDead) {
                    if (mState == MONSTER_CHARGE) {
                        mState = MONSTER_SLIDE;
                        mCurrentFrame = 20;
                        mFrameTimer = 0.0f;
                    }
                    else if (mState == MONSTER_RUN && mTurnCooldown <= 0) {
                        mState = MONSTER_TURN;
                        mCurrentFrame = 21;
                        mFrameTimer = 0.0f;
                    }
                }
            }
        }
    }

    Rectangle getRect() override {
        return { mPosition.x, mPosition.y, mSize.x, mSize.y };
    }

    void draw() override {
        Rectangle source = {
            (float)mCurrentFrame * SPRITE_SIZE,
            0.0f,
            mIsFacingRight ? SPRITE_SIZE : -SPRITE_SIZE,
            SPRITE_SIZE
        };

        Vector2 drawPos = mPosition;
        drawPos.x -= (SPRITE_SIZE - mSize.x) / 2.0f;
        drawPos.y -= (SPRITE_SIZE - mSize.y);

        Color color = WHITE;
        if (mHurtTimer > 0) color = RED;
        else if (mIsDead) color = DARKGRAY;

        DrawTextureRec(mTexture, source, drawPos, color);
    }
};