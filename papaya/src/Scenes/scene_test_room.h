#pragma once
#include "raylib.h"
#include <vector>
#include <fstream>
#include <algorithm>
#include "Entities/Entity.h"
#include "Entities/Player.h"
#include "Entities/Wall.h"
#include "Entities/Dummy.h"
#include "Entities/RabbitEnemy.h"
#include "Saves/map.hpp"

class Enemy : public Entity {
public:
    Vector2 mPosition, mVelocity, mSize, mPrevPosition;
    float mSpeed = 40.0f, mGravity = 500.0f, mHurtTimer = 0.0f;
    int mHealth = 3;
    bool mIsFacingRight = true, mIsDead = false;

    Enemy(float startX, float startY) : Entity({ startX, startY }, ENEMY) {
        mPosition = { startX, startY }; mPrevPosition = mPosition;
        mVelocity = { mSpeed, 0 }; mSize = { 16.0f, 16.0f };
    }
    void takeDamage(int amount) {
        mHealth -= amount; mHurtTimer = 0.2f; mVelocity.y = -150.0f;
        if (mHealth <= 0) mIsDead = true;
    }
    void update(float deltaTime) override {
        if (mIsDead) return;
        mPrevPosition = mPosition;
        mVelocity.y += mGravity * deltaTime;
        mPosition.x += mVelocity.x * deltaTime;
        mPosition.y += mVelocity.y * deltaTime;
        if (mHurtTimer > 0) mHurtTimer -= deltaTime;
    }
    Rectangle getRect() override { return { mPosition.x, mPosition.y, mSize.x, mSize.y }; }
    void onCollision(Entity* pOther) override {
        if (pOther->mType == WALL) {
            Rectangle otherRect = pOther->getRect();
            float prevBottom = mPrevPosition.y + mSize.y;
            float prevRight = mPrevPosition.x + mSize.x;
            float prevLeft = mPrevPosition.x;

            if (prevBottom <= otherRect.y + 5.0f) {
                mPosition.y = otherRect.y - mSize.y; mVelocity.y = 0;
            }
            else {
                if (prevRight <= otherRect.x + 5.0f) {
                    mPosition.x = otherRect.x - mSize.x; mVelocity.x *= -1; mIsFacingRight = !mIsFacingRight;
                }
                else if (prevLeft >= otherRect.x + otherRect.width - 5.0f) {
                    mPosition.x = otherRect.x + otherRect.width; mVelocity.x *= -1; mIsFacingRight = !mIsFacingRight;
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

class SceneTestRoom {
public:
    std::vector<Entity*> mEntities;
    std::vector<Entity*> walls;
    std::vector<Enemy*> enemies;

    Player* pPlayer = nullptr;
    RabbitEnemy* pRabbit = nullptr;
    Camera2D mCamera = { 0 };
    Texture2D texRabbit;

    bool mDebugMode = true;

    SceneTestRoom() {
        pPlayer = new Player(30, 100);
        mEntities.push_back(pPlayer);

        texRabbit = LoadTexture("Assets/rabbit.png");

        pRabbit = new RabbitEnemy(180, 100, texRabbit, pPlayer);
        mEntities.push_back(pRabbit);

        // Ładowanie mapy
        bool mapLoaded = false;
        std::fstream f("Assets/maps/test.map", std::ios::in | std::ios::binary);
        if (f.is_open()) {
            auto map = MapLoader(f);
            auto w = map.getAll();
            for (auto& wall : w) {
                Wall* wObj = new Wall(wall.mPosition.x, wall.mPosition.y, wall.mSize.x, wall.mSize.y);
                mEntities.push_back(wObj);
                walls.push_back(wObj);
            }
            f.close();
            mapLoaded = true;
        }

        if (!mapLoaded || walls.empty()) {
            CreateTestWalls();
        }

        mCamera.target = { pPlayer->mPosition.x, pPlayer->mPosition.y };
        mCamera.offset = { 160.0f, 90.0f };
        mCamera.rotation = 0.0f;
        mCamera.zoom = 1.0f;
    }

    void CreateTestWalls() {
        Wall* floor = new Wall(0, 200, 400, 32);
        mEntities.push_back(floor); walls.push_back(floor);
        Wall* leftWall = new Wall(0, 100, 32, 100);
        mEntities.push_back(leftWall); walls.push_back(leftWall);
        Wall* rightWall = new Wall(368, 100, 32, 100);
        mEntities.push_back(rightWall); walls.push_back(rightWall);
        Wall* platform = new Wall(150, 150, 100, 16);
        mEntities.push_back(platform); walls.push_back(platform);
    }

    ~SceneTestRoom() {
        // czyszczenie wszystkiego
        for (Entity* e : mEntities) delete e;
        mEntities.clear();
        walls.clear();

        for (Enemy* e : enemies) delete e;
        enemies.clear();
        UnloadTexture(texRabbit);
    }

    void Update(float deltaTime) {
        if (IsKeyPressed(KEY_F1)) mDebugMode = !mDebugMode;

        if (IsKeyPressed(KEY_F2)) {
            if (!pRabbit) {
                pRabbit = new RabbitEnemy(180, 100, texRabbit, pPlayer);
                mEntities.push_back(pRabbit);
            }
            else {
                pRabbit->mPosition = { 180, 100 };
                pRabbit->mVelocity = { 0, 0 };
                pRabbit->mState = RabbitEnemy::RABBIT_IDLE;
                pRabbit->mHealth = 3;
                pRabbit->mIsDead = false;
                pRabbit->mActive = true;
                pRabbit->mFollowTimer = 0;
                pRabbit->mCurrentFrame = 0;
                pRabbit->mSize = { 16.0f, 16.0f };
            }
        }

        // 1. Aktualizacja encji
        for (Entity* pEntity : mEntities) {
            if (pEntity->mActive) {
                pEntity->update(deltaTime);
            }
        }

        // 2. Aktualizacja prostych wrogów
        for (size_t i = 0; i < enemies.size(); i++) {
            Enemy* e = enemies[i];
            e->update(deltaTime);
            for (Entity* wall : walls) {
                if (CheckCollisionRecs(e->getRect(), wall->getRect())) e->onCollision(wall);
            }
            if (e->mIsDead) {
                delete e;
                enemies.erase(enemies.begin() + i);
                i--;
            }
        }

        // A. Ruszające się obiekty vs Ściany
        for (Entity* entity : mEntities) {
            // Sprawdzamy tylko "żywe" byty (Gracz, Wrogowie), pomijamy Ściany (WALL)
            if (entity->mType == WALL || !entity->mActive) continue;

            for (Entity* wall : walls) { // walls to Twoja osobna lista ścian
                if (CheckCollisionRecs(entity->getRect(), wall->getRect())) {
                    entity->onCollision(wall);
                }
            }
        }

        // 4. Kolizja z graczem (Obrażenia)
        for (Enemy* e : enemies) {
            if (!e->mIsDead && CheckCollisionRecs(pPlayer->getRect(), e->getRect())) {
                ApplyDamageToPlayer(e->mPosition.x + e->mSize.x / 2.0f);
            }
        }

        if (pRabbit && pRabbit->mActive) {
            bool isAlive = !pRabbit->mIsDead && pRabbit->mState != RabbitEnemy::MONSTER_DYING;
 
            bool isDangerous = (pRabbit->mState != RabbitEnemy::RABBIT_IDLE &&
                pRabbit->mState != RabbitEnemy::RABBIT_HOP &&
                pRabbit->mState != RabbitEnemy::MORPHING &&
                pRabbit->mState != RabbitEnemy::UNMORPHING);

            // zadaj obrażenia tylko jeśli żyje oraz jest niebezpieczny
            if (isAlive && isDangerous) {
                if (CheckCollisionRecs(pPlayer->getRect(), pRabbit->getRect())) {
                    ApplyDamageToPlayer(pRabbit->mPosition.x + pRabbit->mSize.x / 2.0f);
                }
            }
        }

        // 5. Atak gracza
        if (pPlayer->mIsAttacking) {
            for (Entity* pEntity : mEntities) handlePlayerHit(pPlayer, pEntity);
            for (Enemy* pEnemy : enemies) handlePlayerHit(pPlayer, pEnemy);
        }

        // 6. Reset gracza
        if (pPlayer->mPosition.y > 500) {
            pPlayer->mPosition = { 30, 100 };
            pPlayer->mVelocity = { 0, 0 };
        }

        // 7. Kamera
        if (pPlayer) {
            pPlayer->updateAnimation(deltaTime);
            mCamera.target.x = floorf(pPlayer->mPosition.x);
            mCamera.target.y = floorf(pPlayer->mPosition.y);
        }

    }

    void ApplyDamageToPlayer(float enemyCenterX) {
        if (pPlayer->mInvincibilityTimer <= 0) {
            pPlayer->mHealth--;

            float playerCenterX = pPlayer->mPosition.x + pPlayer->mSize.x / 2.0f;
            float recoilDir = (playerCenterX < enemyCenterX) ? -1.0f : 1.0f;

            pPlayer->mVelocity.x = recoilDir * 300.0f;
            pPlayer->mVelocity.y = -150.0f;
            pPlayer->mRecoilTimer = 0.2f;
            pPlayer->mInvincibilityTimer = 1.0f;
            pPlayer->mIsDashing = false;
            pPlayer->mIsAttacking = false;
        }
    }

    void Draw() {
        ClearBackground(DARKGRAY);
        BeginMode2D(mCamera);

        for (Entity* pEntity : mEntities) {
            if (pEntity->mActive) pEntity->draw();
        }
        for (Enemy* e : enemies) e->draw();

        if (mDebugMode) {
            for (Entity* wall : walls) DrawRectangleLinesEx(wall->getRect(), 2, RED);
            DrawRectangleLinesEx(pPlayer->getRect(), 1, GREEN);
        }

        for (Entity* pEntity : mEntities) {
            if (pEntity->mActive) pEntity->draw();
        }

        for (Enemy* e : enemies) e->draw();

        // DEBUG
        //if (pPlayer && pRabbit) {
        //    // Rysuj linię między królikiem a graczem
        //    DrawLineV(
        //        { pRabbit->mPosition.x + 8, pRabbit->mPosition.y + 8 },
        //        { pPlayer->mPosition.x + 5, pPlayer->mPosition.y + 7 },
        //        YELLOW
        //    );
        //}

        EndMode2D();

        // UI
        DrawText("ARROWS - Move", 10, 10, 10, WHITE);
        DrawText(TextFormat("HP: %d", pPlayer->mHealth), 10, 30, 20, RED);

        if (mDebugMode) {
            DrawText("F2 - Reset Rabbit", 10, 105, 10, YELLOW);
            if (pRabbit) {
                const char* stateNames[] = { "IDLE", "HOP", "MORPH", "CHARGE", "RUN", "SLIDE", "TURN", "UNMORPH", "DYING" };
                DrawText(TextFormat("Rabbit State: %s", stateNames[pRabbit->mState]), 10, 135, 10, YELLOW);
            }
            else {
                DrawText("Rabbit DEAD/NULL", 10, 135, 10, RED);
            }
        }
    }

private:
    void handlePlayerHit(Player* pPlayer, Entity* target) {
        if (target == pPlayer || target->mType == WALL) return;
        if (!target->mActive) return;

        if (CheckCollisionRecs(pPlayer->mAttackArea, target->getRect())) {
            // sprawdamy czy już nie oberwał w tej klatce ataku
            for (Entity* hit : pPlayer->mHitEntities) if (hit == target) return;

            RabbitEnemy* pRabbitEnemy = dynamic_cast<RabbitEnemy*>(target);
            if (pRabbitEnemy) {
                // jeśli królik jest martwy, ignorujemy trafienie całkowicie
                // (brak obrażeń, brak pogo, brak recoilu)
                if (pRabbitEnemy->mIsDead) return;

                pRabbitEnemy->takeDamage(1);
            }

            Dummy* pDummy = dynamic_cast<Dummy*>(target);
            if (pDummy && !pDummy->mIsHit) pDummy->takeDamage();

            Enemy* pEnemy = dynamic_cast<Enemy*>(target);
            if (pEnemy) pEnemy->takeDamage(1);

            // dodajemy do listy trafionych (żeby nie bić co klatkę tego samego)
            pPlayer->mHitEntities.push_back(target);

            WeaponStats stats = pPlayer->getCurrentWeaponStats();

            // Pogo logic (Atak w dół)
            if (pPlayer->mAttackDir == 2) {
                pPlayer->mVelocity.y = stats.pogoForce;
                pPlayer->mIsPogoJump = true;
                pPlayer->mIsJumping = false;
                pPlayer->mJumpBufferCounter = 0;
                pPlayer->mIsDashing = false;
                pPlayer->mCanDash = true;
            }
            // Recoil (Atak w bok)
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
            }
        }
    }
};