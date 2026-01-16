#pragma once
#include "../Entity.h"
#include "../Player/Player.h"
#include "raylib.h"
#include <vector>
#include <cmath>
#include <algorithm> // Do std::max, std::min
#include <iostream>  // DO LOGOWANIA W KONSOLI

// Struktura pocisku
struct Fireball {
    Vector2 position;
    Vector2 velocity;
    bool active;
    float radius = 5.0f;
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

    BossState mState;
    Texture2D mTexture;
    Vector2 mVelocity;
    Entity* pTarget = nullptr;

    // Granice areny
    Rectangle mArenaBounds = { 0, 0, 0, 0 };

    // Statystyki
    int mHealth = 20;
    int mMaxHealth = 20;
    float mFloatSpeed = 2.0f;     // Prędkość lewitacji góra/dół
    float mFloatOffset = 0.0f;
    float mStartY;
    float mMoveSpeed = 50.0f;

    // Timery stanów
    float mStateTimer = 0.0f;
    float mIdleTimeBase = 2.0f;
    float mLaserChargeTimeBase = 2.5f;
    float mLaserFireTimeBase = 1.0f;   // Czas trwania ataku
    float mVulnerableTime = 3.0f;      // Czas na bicie bossa

    // Aktualne wartości (modyfikowane przez enrage)
    float mIdleTime = 2.0f;
    float mLaserChargeTime = 2.5f;
    float mLaserFireTime = 1.0f;

    // Laser
    float mLaserAngle = 0.0f;   // aktualny kąt lasera (radiany)
    float mLaserTrackSpeedChargeBase = 2.5f;
    float mLaserTrackSpeedFireBase = 0.4f;
    float mLaserTrackSpeedCharge = 2.5f;    // szybkość śledzenia podczas ładowania
    float mLaserTrackSpeedFire = 0.4f;      // wolniejsza szybkość podczas strzału
    bool mLaserInitialized = false;
    Vector2 mLaserStart;
    Vector2 mLaserEnd;
    float mLaserWidth = 5.0f;

    // Fireball
    std::vector<Fireball> mFireballs;
    float mFireballSpeed = 240.0f;
    int mFireballCountBase = 1;
    int mFireballCount = 1; // ile fireballi na raz
    float mFireballSpread = 15.0f; // rozrzut w stopniach
    bool mHasFireballCast = false;
    bool mChooseFireball = false;

    // Teleportacja
    float mTeleportationDuration = 0.5f;
    Vector2 mTeleportTarget;
    float mTeleportAlpha = 1.0f;

    // AOE attack
    float mAoeRadius = 0.0f;
    float mAoeMaxRadius = 80.0f;
    float mAoeDuration = 0.6f;
    float mAoeWarningTime = 1.8f;  // Czas ostrzeżenia przed AOE
    bool mAoeDealtDamage = false;

    // Enrage system
    bool mIsEnraged = false;
    float mEnrageThreshold = 0.5f; // 50% HP
    float mEnrageSpeedMultiplier = 1.5f;
    Color mEnrageColor = { 255, 100, 100, 255 };

    // animacja
    int mCurrentFrame = 0;
    float mFrameTimer = 0.0f;
    const float SPRITE_SIZE = 32.0f;

    bool mIsFacingRight = false;
    float mHurtTimer = 0.0f;

    int mDeathFrameStart = 17;
    int mDeathFrameEnd = 20;
    float mActivationDistance = 120.0f;

    // fizyka kolizji
    Vector2 mPrevPosition;

    // Debug: statystyka walki
    int mAttackCounter = 0;

    MageBoss(float x, float y, Texture tex, Entity* target)
        : Entity({ x,y }, MAGE_BOSS) {
        mPosition = { x,y };
        mTexture = tex;
        pTarget = target;
        mSize = { 32.0f,38.0f };
        mState = INACTIVE;
        mIsFacingRight = false;
        mVelocity = { 0,0 };
        mLaserStart = { 0,0 };
        mLaserEnd = { 0,0 };
        mPrevPosition = mPosition;
        mStartY = y;
    }

    void setArenaBounds(float x, float y, float w, float h) {
        mArenaBounds = { x, y, w, h };
    }

    void checkEnrage() {
        float hpPercent = (float)mHealth / (float)mMaxHealth;

        if (!mIsEnraged && hpPercent < mEnrageThreshold) {
            mIsEnraged = true;

            // modyfikacja statystyk bossa
            mIdleTime = mIdleTimeBase / mEnrageSpeedMultiplier;
            mLaserChargeTime = mLaserChargeTimeBase / mEnrageSpeedMultiplier;
            mLaserFireTime = mLaserFireTimeBase / mEnrageSpeedMultiplier;
            mLaserTrackSpeedCharge = mLaserTrackSpeedChargeBase * mEnrageSpeedMultiplier;
            mLaserTrackSpeedFire = mLaserTrackSpeedFireBase * mEnrageSpeedMultiplier;
            mFireballCount = 3;
            mFireballSpeed *= 1.2f;
        }
    }

    void takeDamage(int amount) {
        if (mState == DYING || mState == INACTIVE || !mActive) return;
        if (mState == TELEPORT_OUT || mState == TELEPORT_IN) return; // i-frames

        mHealth -= amount;
        mHurtTimer = 0.2f;

        checkEnrage();

        if (mHealth <= 0) {
            mHealth = 0;
            mState = DYING;
            mCurrentFrame = mDeathFrameStart;
            mFrameTimer = 0.0f;

            for (auto& fb : mFireballs) {
                fb.active = false;
            }
        }
    }

    // --- ZMODYFIKOWANA TELEPORTACJA (KRÓTKI DYSTANS) ---
    Vector2 chooseTeleportDestination(const std::vector<Entity*>& walls) {

        // Próbujemy 10 razy znaleźć bezpieczne miejsce blisko siebie
        for (int attempt = 0; attempt < 10; attempt++) {
            Vector2 candidate = mPosition; // Startujemy od aktualnej pozycji

            // --- LOGIKA: Krótki skok (BLINK) ---

            // 1. Oś X: Losujemy skok w lewo lub w prawo (np. od 80 do 200 pikseli)
            float dirX = (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
            float distX = (float)GetRandomValue(80, 200);

            // 2. Oś Y: Minimalna zmiana (tylko +/- 30 pikseli), żeby nie wlatywał w sufit/podłogę
            float distY = (float)GetRandomValue(-30, 30);

            candidate.x += dirX * distX;
            candidate.y += distY;

            // --- ZABEZPIECZENIE: Granice areny ---
            // Jeśli mamy ustawioną arenę, upewnij się, że nie wychodzimy poza nią
            if (mArenaBounds.width > 0) {
                // ZMNIEJSZONO MARGINES Z 32.0f na 10.0f, żeby boss mieścił się w niskiej arenie (100px)
                float margin = 10.0f;
                float minX = mArenaBounds.x + margin;
                float maxX = mArenaBounds.x + mArenaBounds.width - margin - mSize.x;
                float minY = mArenaBounds.y + margin;
                float maxY = mArenaBounds.y + mArenaBounds.height - margin - mSize.y;

                if (minX >= maxX) minX = maxX = mArenaBounds.x;
                if (minY >= maxY) minY = maxY = mArenaBounds.y;

                // Clamp (przytnij) pozycję do granic mapy
                candidate.x = std::clamp(candidate.x, minX, maxX);
                candidate.y = std::clamp(candidate.y, minY, maxY);
            }

            // --- SPRAWDZANIE KOLIZJI ZE ŚCIANAMI ---
            Rectangle candidateRect = { candidate.x, candidate.y, mSize.x, mSize.y };
            bool collidesWithWall = false;

            for (auto* wall : walls) {
                if (CheckCollisionRecs(candidateRect, wall->getRect())) {
                    collidesWithWall = true;
                    break;
                }
            }

            if (collidesWithWall) continue; // Wpadł w ścianę? Losuj jeszcze raz

            // 2. Sprawdź dystans od gracza (opcjonalnie, żeby nie wpadł w gracza)
            if (pTarget) {
                float distToPlayer = sqrtf(powf(candidate.x - pTarget->mPosition.x, 2) +
                    powf(candidate.y - pTarget->mPosition.y, 2));

                if (distToPlayer < 40.0f) continue;
            }

            // Znaleziono dobry punkt obok!
            std::cout << "[BOSS TP] Short Blink to: " << candidate.x << ", " << candidate.y << std::endl;
            return candidate;
        }

        // Jeśli 10 prób się nie uda (np. zablokowany), zostań w miejscu
        std::cout << "[BOSS TP] Blocked! Staying." << std::endl;
        return mPosition;
    }

    // --- UPDATE ---
    void updateBossLogic(float deltaTime, std::vector<Entity*>& walls) {
        if (!mActive) return;

        if (mState == DYING) {
            updateAnimation(deltaTime);
            return;
        }

        mPrevPosition = mPosition;

        // efekt lewitacji (nie podczas teleportacji)
        if (mState != TELEPORT_OUT && mState != TELEPORT_IN) {
            mFloatOffset += deltaTime * mFloatSpeed;
            float targetY = mStartY + sinf(mFloatOffset) * 10.0f;

            if (mState != VULNERABLE && mState != LASER_FIRE && mState != AOE_ATTACK) {
                mPosition.y += (targetY - mPosition.y) * 2.0f * deltaTime;
            }
        }

        mPosition.x += mVelocity.x * deltaTime;

        if (mHurtTimer > 0) mHurtTimer -= deltaTime;

        // Logika Fireballi
        for (auto& fb : mFireballs) {
            if (!fb.active) continue;

            fb.position.x += fb.velocity.x * deltaTime;
            fb.position.y += fb.velocity.y * deltaTime;

            fb.animTimer += deltaTime;
            if (fb.animTimer > 0.1f) {
                fb.animTimer = 0.0f;
                fb.animFrame++;
                if (fb.animFrame > 3) fb.animFrame = 0;
            }

            for (auto& wall : walls) {
                if (CheckCollisionCircleRec(fb.position, fb.radius, wall->getRect())) {
                    fb.active = false;
                }
            }

            float distFromBoss = sqrtf(
                powf(fb.position.x - mPosition.x, 2) +
                powf(fb.position.y - mPosition.y, 2)
            );
            if (distFromBoss > 1000.0f) fb.active = false;
        }

        auto it = mFireballs.begin();
        while (it != mFireballs.end()) {
            if (!it->active) {
                it = mFireballs.erase(it);
            }
            else {
                ++it;
            }
        }

        // Maszyna stanów
        switch (mState) {
        case INACTIVE:
            mVelocity = { 0,0 };
            if (pTarget) {
                float dist = sqrt(
                    pow(mPosition.x - pTarget->mPosition.x, 2) +
                    pow(mPosition.y - pTarget->mPosition.y, 2)
                );
                if (dist < mActivationDistance) {
                    mState = APPEARING;
                    mCurrentFrame = mDeathFrameEnd;
                    mFrameTimer = 0.0f;
                    mIsFacingRight = (pTarget->mPosition.x > mPosition.x);
                }
            }
            break;

        case APPEARING:
            mVelocity = { 0,0 };
            break;

        case IDLE:
            mStateTimer += deltaTime;
            if (pTarget) {
                float distToPlayer = pTarget->mPosition.x - mPosition.x;
                mIsFacingRight = (distToPlayer > 0);

                if (fabs(distToPlayer) > 150.0f) {
                    float dir = (distToPlayer > 0) ? 1.0f : -1.0f;
                    mVelocity.x = dir * mMoveSpeed;
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
                    if (mAttackCounter % 3 == 0) mChooseFireball = true;
                    else mState = LASER_CHARGE;
                }
                else {
                    if (GetRandomValue(0, 1) == 0) mChooseFireball = true;
                    else mState = LASER_CHARGE;
                }

                if (mChooseFireball) {
                    mState = CAST_FIREBALL;
                    mHasFireballCast = false;
                }
            }
            break;

        case CAST_FIREBALL:
            mStateTimer += deltaTime;
            mVelocity.x = 0;

            if (mStateTimer > 0.4f && !mHasFireballCast) {
                if (mFireballs.size() < 30) {
                    spawnFireballPattern();
                    mHasFireballCast = true;
                }
            }

            if (mStateTimer > 0.8f) {
                mState = IDLE;
                mStateTimer = 0.0f;
            }
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
            calculateLaserEnd(walls, deltaTime);
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
            mStateTimer += deltaTime;
            mVelocity.x = 0;
            if (mStateTimer < mAoeWarningTime) {
                mAoeRadius = (mStateTimer / mAoeWarningTime) * mAoeMaxRadius;
            }
            else if (mStateTimer < mAoeWarningTime * mAoeDuration) {
                mAoeRadius = mAoeMaxRadius * 1.2f;
                if (!mAoeDealtDamage) {
                    mAoeDealtDamage = true;
                }
            }
            else if (mStateTimer >= mAoeWarningTime + mAoeDuration) {
                mState = TELEPORT_OUT;
                mStateTimer = 0.0f;
                mTeleportTarget = chooseTeleportDestination(walls);
                mTeleportAlpha = 1.0f;
                mAoeRadius = 0.0f;
                mCurrentFrame = mDeathFrameStart;
            }
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
            mVelocity = { 0,0 };

            mTeleportAlpha = mStateTimer / mTeleportationDuration;
            if (mTeleportAlpha > 1.0f) mTeleportAlpha = 1.0f;

            if (pTarget) {
                mIsFacingRight = (pTarget->mPosition.x > mPosition.x);
            }

            if (mStateTimer >= mTeleportationDuration) {
                mState = IDLE;
                mStateTimer = 0.0f;
                mTeleportAlpha = 1.0f;
                mLaserInitialized = false;
            }
            break;
        }
        updateAnimation(deltaTime);
    }

    void updateAnimation(float deltaTime) {
        mFrameTimer += deltaTime;
        float animSpeed = 0.15f;

        if (mState == DYING && mCurrentFrame < mDeathFrameStart) {
            mCurrentFrame = mDeathFrameStart;
        }

        if (mFrameTimer >= animSpeed) {
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
                else if (mState == APPEARING) {
                    mState = IDLE;
                    mCurrentFrame = 0;
                    mStateTimer = 0.0f;
                }
                break;
            case TELEPORT_OUT:
                if (mCurrentFrame < mDeathFrameEnd) mCurrentFrame++;
                break;
            case IDLE:
                mCurrentFrame++;
                if (mCurrentFrame > 3) mCurrentFrame = 0;
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
                if (mCurrentFrame > 8) mCurrentFrame = 4;
                else mCurrentFrame++;
                break;
            }
        }
    }

    void spawnFireballPattern() {
        if (!pTarget) return;

        Vector2 spawnPos = { mPosition.x + mSize.x / 2, mPosition.y + mSize.y / 2 };
        Vector2 playerCenter = {
            pTarget->mPosition.x + pTarget->mSize.x / 2,
            pTarget->mPosition.y + pTarget->mSize.y / 2
        };

        Vector2 playerVelocity = { 0,0 };
        Player* player = dynamic_cast<Player*>(pTarget);
        if (player) playerVelocity = player->mVelocity;

        float distToPlayer = sqrtf(
            powf(playerCenter.x - spawnPos.x, 2) +
            powf(playerCenter.y - spawnPos.y, 2)
        );
        float flightTime = distToPlayer / mFireballSpeed;

        Vector2 predictedPos = {
            playerCenter.x + playerVelocity.x * flightTime * 0.5f,
            playerCenter.y + playerVelocity.y * flightTime * 0.3f
        };

        float baseAngle = atan2f(
            predictedPos.y - spawnPos.y,
            predictedPos.x - spawnPos.x
        );

        for (int i = 0; i < mFireballCount; i++) {
            Fireball fb;
            fb.active = true;
            fb.position = spawnPos;

            float angle = baseAngle;
            if (mFireballCount > 1) {
                float spreadRad = mFireballSpeed * DEG2RAD;
                float offset = -spreadRad / 2.0f * (spreadRad * i / (mFireballCount - 1));
                angle += offset;
            }

            fb.velocity = {
                cosf(angle) * mFireballSpeed,
                sinf(angle) * mFireballSpeed
            };
            fb.rotation = angle * RAD2DEG;
            mFireballs.push_back(fb);
        }
    }

    void calculateLaserEnd(std::vector<Entity*>& walls, float deltaTime) {
        if (!pTarget) return;

        mLaserStart = { mPosition.x + mSize.x / 2, mPosition.y + mSize.y / 2 };
        Vector2 targetCenter = { pTarget->mPosition.x + pTarget->mSize.x / 2, pTarget->mPosition.y + pTarget->mSize.y / 2 };

        float targetAngle = atan2f(
            targetCenter.y - mLaserStart.y,
            targetCenter.x - mLaserStart.x
        );

        if (!mLaserInitialized) {
            mLaserAngle = targetAngle;
            mLaserInitialized = true;
        }

        float trackSpeed = (mState == LASER_FIRE) ? mLaserTrackSpeedFire : mLaserTrackSpeedCharge;
        float angleDiff = targetAngle - mLaserAngle;

        while (angleDiff > PI) angleDiff -= 2.0f * PI;
        while (angleDiff < -PI) angleDiff += 2.0f * PI;

        mLaserAngle += angleDiff * trackSpeed * deltaTime;

        Vector2 laserDir = { cosf(mLaserAngle), sinf(mLaserAngle) };

        float maxDist = 1000.0f;
        float currentDist = 0.0f;
        float step = 5.0f;
        bool hitWall = false;

        while (currentDist < maxDist) {
            Vector2 checkPoint = {
                mLaserStart.x + laserDir.x * currentDist,
                mLaserStart.y + laserDir.y * currentDist
            };

            for (auto wall : walls) {
                if (CheckCollisionPointRec(checkPoint, wall->getRect())) {
                    hitWall = true;
                    break;
                }
            }

            if (hitWall) break;
            currentDist += step;
        }

        mLaserEnd = {
            mLaserStart.x + laserDir.x * currentDist,
            mLaserStart.y + laserDir.y * currentDist
        };
    }

    bool checkAoeCollision(Entity* player) {
        if (!player || mState != AOE_ATTACK) return false;
        if (mStateTimer < mAoeWarningTime) return false;
        if (mStateTimer >= mAoeWarningTime + mAoeDuration) return false;
        if (mAoeDealtDamage) return false;

        Vector2 bossCenter = { mPosition.x + mSize.x / 2, mPosition.y + mSize.y / 2 };
        Vector2 playerCenter = {
            player->mPosition.x + player->mSize.x / 2,
            player->mPosition.y + player->mSize.y / 2
        };

        float dist = sqrtf(
            powf(playerCenter.x - bossCenter.x, 2) +
            powf(playerCenter.y - bossCenter.y, 2)
        );

        return dist <= mAoeMaxRadius;
    }

    bool checkLaserCollision(Entity* player) {
        Vector2 playerCenter = { player->mPosition.x + player->mSize.x / 2 , player->mPosition.y + player->mSize.y / 2 };
        return CheckCollisionCircleLine(playerCenter, player->mSize.x / 2, mLaserStart, mLaserEnd);
    }

    void update(float deltaTime) override {}

    Rectangle getRect() override {
        return { mPosition.x + 10, mPosition.y + 10, mSize.x - 20, mSize.y - 10 };
    }

    void onCollision(Entity* pOther) override {
        if (mState == DYING) return;

        if (pOther->mType == WALL) {
            Rectangle otherRect = pOther->getRect();
            float myCenterX = mPosition.x + mSize.x / 2.0f;
            float myCenterY = mPosition.y + mSize.y / 2.0f;
            float wallCenterX = otherRect.x + otherRect.width / 2.0f;
            float wallCenterY = otherRect.y + otherRect.height / 2.0f;

            float dx = fabs(myCenterX - wallCenterX);
            float dy = fabs(myCenterY - wallCenterY);

            float minDistX = (mSize.x / 2.0f) + (otherRect.width / 2.0f);
            float minDistY = (mSize.y / 2.0f) + (otherRect.height / 2.0f);

            float overlapX = minDistX - dx;
            float overlapY = minDistY - dy;

            if (overlapX < overlapY) {
                if (myCenterX < wallCenterX) mPosition.x -= overlapX;
                else mPosition.x += overlapX;
            }
            else {
                if (myCenterY < wallCenterY) mPosition.y -= overlapY;
                else mPosition.y += overlapY;
            }
        }
    }

    void draw() override {
        if (mState == INACTIVE || !mActive) return;

        Vector2 bossCenter = { mPosition.x + mSize.x / 2, mPosition.y + mSize.y / 2 };

        if (mState == AOE_ATTACK) {
            if (mStateTimer < mAoeWarningTime) {
                if (mAoeRadius > 0) {
                    DrawCircleLinesV(bossCenter, mAoeRadius, RED);
                    if ((int)(GetTime() * 10) % 2 == 0) DrawCircleV(bossCenter, 8.0f, RED);
                }
            }
            else if (mStateTimer < mAoeWarningTime + mAoeDuration) {
                float explosionProgress = (mStateTimer - mAoeWarningTime) / mAoeDuration;
                float alpha = 1.0f - explosionProgress;
                DrawCircleV(bossCenter, mAoeMaxRadius, Fade(RED, alpha * 0.6f));
                DrawCircleLinesV(bossCenter, mAoeMaxRadius, Fade(WHITE, alpha));
            }
        }

        for (const auto& fb : mFireballs) {
            if (fb.active) {
                int spriteIndex = 13 + fb.animFrame;
                Rectangle source = { (float)spriteIndex * SPRITE_SIZE, 0.0f, SPRITE_SIZE, SPRITE_SIZE };
                Rectangle dest = { fb.position.x, fb.position.y, SPRITE_SIZE, SPRITE_SIZE };
                Vector2 origin = { SPRITE_SIZE / 2.0f, SPRITE_SIZE / 2.0f };
                DrawTexturePro(mTexture, source, dest, origin, fb.rotation, WHITE);
                DrawCircleLines(fb.position.x, fb.position.y, fb.radius, RED);
            }
        }

        if (mState == LASER_CHARGE) {
            if ((int)(GetTime() * 20) % 2 == 0) {
                DrawLineEx(mLaserStart, mLaserEnd, 2.0f, Fade(SKYBLUE, 0.5f));
            }
        }
        else if (mState == LASER_FIRE) {
            Color laserColorOuter = mIsEnraged ? RED : SKYBLUE;
            Color laserColorInner = mIsEnraged ? MAROON : BLUE;
            DrawLineEx(mLaserStart, mLaserEnd, 5.0f, laserColorOuter);
            DrawLineEx(mLaserStart, mLaserEnd, 2.0f, laserColorInner);
            DrawLineEx(mLaserStart, mLaserEnd, 1.0f, WHITE);
            DrawCircleV(mLaserEnd, 2.0f, RAYWHITE);
            DrawCircleLines(mLaserEnd.x, mLaserEnd.y, 5.0f + sin(GetTime() * 3) * 2.0f, SKYBLUE);
        }

        Vector2 drawPos = { mPosition.x - (SPRITE_SIZE - mSize.x) / 2, mPosition.y - (SPRITE_SIZE - mSize.y) / 2 };

        if (mState == TELEPORT_OUT || mState == TELEPORT_IN) {
            drawPos.y += (1.0f - mTeleportAlpha) * -20.0f;
        }

        Rectangle source = {
            (float)mCurrentFrame * SPRITE_SIZE,
            0.0f,
            mIsFacingRight ? SPRITE_SIZE : -SPRITE_SIZE,
            SPRITE_SIZE
        };

        Color bossColor = WHITE;
        if (mIsEnraged) bossColor = mEnrageColor;
        if (mState == VULNERABLE) bossColor = GRAY;
        if (mHurtTimer > 0) bossColor = RED;
        if (mState == TELEPORT_OUT || mState == TELEPORT_IN) bossColor = Fade(bossColor, mTeleportAlpha);

        DrawTextureRec(mTexture, source, drawPos, bossColor);

        // --- DEBUG TELEPORTU ---
        if (mState == TELEPORT_OUT) {
            DrawLineV(mPosition, mTeleportTarget, YELLOW);
            DrawCircleV(mTeleportTarget, 5.0f, YELLOW);
            DrawText("TP TARGET", (int)mTeleportTarget.x, (int)mTeleportTarget.y, 10, YELLOW);
        }

        if (mState != INACTIVE && mState != TELEPORT_OUT && mState != TELEPORT_IN) {
            float hpBarWidth = 40.0f;
            float hpPercent = (float)mHealth / (float)mMaxHealth;
            DrawRectangle(mPosition.x - 4, mPosition.y - 10, hpBarWidth, 4, DARKGRAY);
            DrawRectangle(mPosition.x - 4, mPosition.y - 10, hpBarWidth * hpPercent, 4, mIsEnraged ? RED : GREEN);
            if (mIsEnraged) DrawText("!", mPosition.x + mSize.x / 2 - 2, mPosition.y - 20, 10, RED);
        }
    }
};