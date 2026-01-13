#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Entity.h"
#include "Entities/Player.h"
#include "Entities/Wall.h"
#include "Entities/Dummy.h"
#include "Entities/rabbit.h"
#include "Entities/mage_boss.h"
#include "Saves/map.hpp"

class Enemy : public Entity {
public:
    Vector2 mVelocity;
    Vector2 mPrevPosition;

    float mSpeed = 40.0f;
    float mGravity = 500.0f;
    int mHealth = 3;

    bool mIsFacingRight = true;
    bool mIsDead = false;
    float mHurtTimer = 0.0f;

    Enemy(float startX, float startY) : Entity({ startX, startY }, ENEMY) {
        mPosition = { startX, startY };
        mPrevPosition = mPosition;
        mVelocity = { mSpeed, 0 };
        mSize = { 16.0f, 16.0f };
    }

    void takeDamage(int amount) {
        mHealth -= amount;
        mHurtTimer = 0.2f;
        mVelocity.y = -150.0f;
        if (mHealth <= 0) {
            mIsDead = true;
        }
    }

    void update(float deltaTime) override {
        if (mIsDead) return;

        mPrevPosition = mPosition;
        mVelocity.y += mGravity * deltaTime;
        mPosition.x += mVelocity.x * deltaTime;
        mPosition.y += mVelocity.y * deltaTime;

        if (mHurtTimer > 0) mHurtTimer -= deltaTime;
    }

    Rectangle getRect() override {
        return { mPosition.x, mPosition.y, mSize.x, mSize.y };
    }

    void onCollision(Entity* pOther) override {
        if (pOther->mType == WALL) {
            Rectangle otherRect = pOther->getRect();
            float prevBottom = mPrevPosition.y + mSize.y;
            float prevRight = mPrevPosition.x + mSize.x;
            float prevLeft = mPrevPosition.x;

            if (prevBottom <= otherRect.y + 5.0f) {
                mPosition.y = otherRect.y - mSize.y;
                mVelocity.y = 0;
            }
            else {
                if (prevRight <= otherRect.x + 5.0f) {
                    mPosition.x = otherRect.x - mSize.x;
                    mVelocity.x *= -1;
                    mIsFacingRight = !mIsFacingRight;
                }
                else if (prevLeft >= otherRect.x + otherRect.width - 5.0f) {
                    mPosition.x = otherRect.x + otherRect.width;
                    mVelocity.x *= -1;
                    mIsFacingRight = !mIsFacingRight;
                }
            }
        }
    }

    void draw() override {
        if (mIsDead) return;
        Color enemyColor = (mHurtTimer > 0) ? WHITE : GREEN;
        DrawRectangle((int)mPosition.x, (int)mPosition.y, (int)mSize.x, (int)mSize.y, enemyColor);
        float eyeOffset = mIsFacingRight ? 8.0f : 2.0f;
        DrawRectangle((int)mPosition.x + (int)eyeOffset, (int)mPosition.y + 4, 4, 4, BLACK);
    }
};

// --- SCENA ---
class SceneTestRoom {
public:
    std::vector<Entity*> mEntities;
    std::vector<Entity*> walls;
    std::vector<Enemy*> enemies;

    Player* pPlayer = nullptr;
    RabbitEnemy* pRabbit = nullptr;
    MageBoss* pBoss = nullptr;

    Texture2D texRabbit;
    Texture2D texBoss;

    Camera2D mCamera = { 0 };
    bool mDebugMode = true;

    SceneTestRoom() {
        // === GRACZ ===
        pPlayer = new Player(50, 200);
        mEntities.push_back(pPlayer);

        // === ŁADOWANIE TEKSTUR ===
        texRabbit = LoadTexture("Assets/rabbit.png");
        texBoss = LoadTexture("Assets/mage_boss.png");

        // === ARENA WALKI ===
        // Główna podłoga
        walls.push_back(new Wall(0, 250, 600, 32));

        // Ściany boczne (żeby nie wypaść)
        walls.push_back(new Wall(-16, 50, 16, 232));   // Lewa
        walls.push_back(new Wall(600, 50, 16, 232));   // Prawa

        // Platformy do skakania
        walls.push_back(new Wall(50, 200, 80, 16));    // Lewa niska
        walls.push_back(new Wall(470, 200, 80, 16));   // Prawa niska
        walls.push_back(new Wall(200, 150, 100, 16));  // Środkowa średnia
        walls.push_back(new Wall(300, 150, 100, 16));  // Środkowa średnia 2
        walls.push_back(new Wall(250, 100, 100, 16));  // Górna środkowa

        // Małe schronienia
        walls.push_back(new Wall(100, 170, 40, 8));    // Mini platforma lewa
        walls.push_back(new Wall(460, 170, 40, 8));    // Mini platforma prawa

        // Dodaj ściany do entities
        for (auto* w : walls) {
            mEntities.push_back(w);
        }

        // === WROGOWIE ===
        // Rabbit - na lewej platformie
        pRabbit = new RabbitEnemy(100, 180, texRabbit, pPlayer);
        mEntities.push_back(pRabbit);

        // Boss - w centrum areny, wysoko
        pBoss = new MageBoss(280, 60, texBoss, pPlayer);
        pBoss->mActivationDistance = 200.0f; // Aktywuje się gdy gracz się zbliży
        mEntities.push_back(pBoss);

        // Dummy do testów
        mEntities.push_back(new Dummy(500, 200));

        // === KAMERA ===
        mCamera.target = { pPlayer->mPosition.x, pPlayer->mPosition.y };
        mCamera.offset = { 160.0f, 90.0f };
        mCamera.rotation = 0.0f;
        mCamera.zoom = 1.0f;
    }

    ~SceneTestRoom() {
        for (Entity* e : mEntities) delete e;
        mEntities.clear();
        walls.clear();

        for (Enemy* e : enemies) delete e;
        enemies.clear();

        UnloadTexture(texRabbit);
        UnloadTexture(texBoss);
    }

    void Update(float deltaTime) {
        // Debug toggle
        if (IsKeyPressed(KEY_F1)) mDebugMode = !mDebugMode;

        // Reset rabbit
        if (IsKeyPressed(KEY_F2) && pRabbit) {
            pRabbit->mPosition = { 100, 180 };
            pRabbit->mVelocity = { 0, 0 };
            pRabbit->mState = RabbitEnemy::RABBIT_IDLE;
            pRabbit->mHealth = 3;
            pRabbit->mIsDead = false;
            pRabbit->mActive = true;
        }

        // Reset boss
        if (IsKeyPressed(KEY_F3) && pBoss) {
            pBoss->mPosition = { 280, 60 };
            pBoss->mHealth = pBoss->mMaxHealth;
            pBoss->mState = MageBoss::INACTIVE;
            pBoss->mActive = true;
            pBoss->mIsEnraged = false;
            pBoss->mFireballs.clear();
        }

        // === UPDATE ENTITIES ===
        for (Entity* pEntity : mEntities) {
            if (!pEntity->mActive) continue;

            // Special update dla bossa
            if (pEntity == pBoss && pBoss->mActive) {
                pBoss->updateBossLogic(deltaTime, walls);
            }
            else {
                pEntity->update(deltaTime);
            }
        }

        // === UPDATE BASIC ENEMIES ===
        for (size_t i = 0; i < enemies.size(); i++) {
            Enemy* e = enemies[i];
            e->update(deltaTime);

            for (auto& wall : walls) {
                if (CheckCollisionRecs(e->getRect(), wall->getRect())) {
                    e->onCollision(wall);
                }
            }

            if (e->mIsDead) {
                delete e;
                enemies.erase(enemies.begin() + i);
                i--;
            }
        }

        // === KOLIZJE ZE ŚCIANAMI ===
        for (Entity* entity : mEntities) {
            if (entity->mType == WALL || !entity->mActive) continue;

            for (Entity* wall : walls) {
                if (CheckCollisionRecs(entity->getRect(), wall->getRect())) {
                    entity->onCollision(wall);
                }
            }
        }

        // === KOLIZJA GRACZ VS WROGOWIE ===
        // Basic enemies
        for (Enemy* e : enemies) {
            if (!e->mIsDead && CheckCollisionRecs(pPlayer->getRect(), e->getRect())) {
                ApplyDamageToPlayer(e->mPosition.x + e->mSize.x / 2.0f);
            }
        }

        // Rabbit
        if (pRabbit && pRabbit->mActive && !pRabbit->mIsDead) {
            bool isDangerous = (pRabbit->mState != RabbitEnemy::RABBIT_IDLE &&
                pRabbit->mState != RabbitEnemy::RABBIT_HOP &&
                pRabbit->mState != RabbitEnemy::MORPHING &&
                pRabbit->mState != RabbitEnemy::UNMORPHING);

            if (isDangerous && CheckCollisionRecs(pPlayer->getRect(), pRabbit->getRect())) {
                ApplyDamageToPlayer(pRabbit->mPosition.x + pRabbit->mSize.x / 2.0f);
            }
        }

        // Boss
        if (pBoss && pBoss->mActive && pBoss->mState != MageBoss::DYING) {
            // Fireball collision
            for (auto& fb : pBoss->mFireballs) {
                if (fb.active && CheckCollisionCircleRec(fb.position, fb.radius, pPlayer->getRect())) {
                    ApplyDamageToPlayer(fb.position.x);
                    fb.active = false;
                }
            }

            // Laser collision
            if (pBoss->mState == MageBoss::LASER_FIRE) {
                if (pBoss->checkLaserCollision(pPlayer)) {
                    ApplyDamageToPlayer(pBoss->mPosition.x);
                }
            }

            // AOE collision
            if (pBoss->mState == MageBoss::AOE_ATTACK) {
                if (pBoss->checkAoeCollision(pPlayer)) {
                    ApplyDamageToPlayer(pBoss->mPosition.x + pBoss->mSize.x / 2.0f);
                    pBoss->mAoeDealtDamage = true;
                }
            }

            // Body collision
            if (CheckCollisionRecs(pPlayer->getRect(), pBoss->getRect())) {
                ApplyDamageToPlayer(pBoss->mPosition.x + pBoss->mSize.x / 2.0f);
            }
        }

        // === ATAK GRACZA ===
        if (pPlayer->mIsAttacking) {
            for (Entity* pEntity : mEntities) {
                handlePlayerHit(pPlayer, pEntity);
            }
            for (Enemy* pEnemy : enemies) {
                handlePlayerHit(pPlayer, pEnemy);
            }
        }

        // === RESET GRACZA ===
        if (pPlayer->mPosition.y > 400) {
            pPlayer->mPosition = { 50, 200 };
            pPlayer->mVelocity = { 0, 0 };
        }

        // === ANIMACJA I KAMERA ===
        pPlayer->updateAnimation(deltaTime);
        mCamera.target.x = floorf(pPlayer->mPosition.x);
        mCamera.target.y = floorf(pPlayer->mPosition.y);
    }

    void ApplyDamageToPlayer(float enemyCenterX) {
        if (pPlayer->mInvincibilityTimer > 0) return;

        pPlayer->takeDamage(1);

        float playerCenterX = pPlayer->mPosition.x + pPlayer->mSize.x / 2.0f;
        float recoilDir = (playerCenterX < enemyCenterX) ? -1.0f : 1.0f;

        pPlayer->mVelocity.x = recoilDir * 300.0f;
        pPlayer->mVelocity.y = -150.0f;
        pPlayer->mRecoilTimer = 0.2f;
        pPlayer->mInvincibilityTimer = 1.0f;
        pPlayer->mIsDashing = false;
        pPlayer->mIsAttacking = false;
    }

    void Draw() {
        ClearBackground({ 30, 30, 40, 255 });

        BeginMode2D(mCamera);

        // Rysuj ściany
        for (Entity* pEntity : mEntities) {
            if (pEntity->mActive) pEntity->draw();
        }

        // Rysuj basic enemies
        for (Enemy* e : enemies) {
            e->draw();
        }

        // Debug hitboxy
        if (mDebugMode) {
            for (Entity* wall : walls) {
                DrawRectangleLinesEx(wall->getRect(), 1, RED);
            }
            DrawRectangleLinesEx(pPlayer->getRect(), 1, GREEN);

            if (pRabbit && pRabbit->mActive) {
                DrawRectangleLinesEx(pRabbit->getRect(), 1, ORANGE);
            }
            if (pBoss && pBoss->mActive) {
                DrawRectangleLinesEx(pBoss->getRect(), 1, PURPLE);
            }
        }

        EndMode2D();

        // === UI ===
        DrawText("=== CONTROLS ===", 10, 10, 10, WHITE);
        DrawText("ARROWS - Move", 10, 25, 10, WHITE);
        DrawText("SPACE - Jump", 10, 40, 10, WHITE);
        DrawText("C - Dash", 10, 55, 10, WHITE);
        DrawText("Z - Attack", 10, 70, 10, WHITE);

        DrawText("=== DEBUG ===", 10, 95, 10, YELLOW);
        DrawText("F1 - Toggle Hitboxes", 10, 110, 10, YELLOW);
        DrawText("F2 - Reset Rabbit", 10, 125, 10, YELLOW);
        DrawText("F3 - Reset Boss", 10, 140, 10, YELLOW);

        DrawText(TextFormat("HP: %d", pPlayer->mHealth), 10, 165, 20, RED);

        // Stan wrogów
        if (pRabbit) {
            const char* rabbitStates[] = { "IDLE", "HOP", "MORPH", "CHARGE", "RUN", "SLIDE", "TURN", "UNMORPH", "DYING" };
            DrawText(TextFormat("Rabbit: %s HP:%d", rabbitStates[pRabbit->mState], pRabbit->mHealth),
                200, 10, 10, pRabbit->mIsDead ? RED : GREEN);
        }

        if (pBoss) {
            const char* bossStates[] = { "INACTIVE", "APPEARING", "IDLE", "FIREBALL", "LASER_CHARGE",
                                         "LASER_FIRE", "VULNERABLE", "TP_OUT", "TP_IN", "AOE", "DYING" };
            Color bossColor = pBoss->mIsEnraged ? RED : (pBoss->mActive ? GREEN : GRAY);
            DrawText(TextFormat("Boss: %s HP:%d/%d %s",
                bossStates[pBoss->mState], pBoss->mHealth, pBoss->mMaxHealth,
                pBoss->mIsEnraged ? "[ENRAGED]" : ""),
                200, 25, 10, bossColor);
        }
    }

private:
    void handlePlayerHit(Player* pPlayer, Entity* target) {
        if (target == pPlayer || target->mType == WALL) return;
        if (!target->mActive) return;

        if (!CheckCollisionRecs(pPlayer->mAttackArea, target->getRect())) return;

        // Sprawdź czy już trafiony
        for (Entity* hit : pPlayer->mHitEntities) {
            if (hit == target) return;
        }

        // Handle different enemy types
        if (MageBoss* boss = dynamic_cast<MageBoss*>(target)) {
            if (boss->mState == MageBoss::VULNERABLE) {
                boss->takeDamage(2);  // Więcej obrażeń gdy vulnerable
            }
            else {
                boss->takeDamage(1);
            }
            pPlayer->mHitEntities.push_back(target);
        }
        else if (RabbitEnemy* rabbit = dynamic_cast<RabbitEnemy*>(target)) {
            if (!rabbit->mIsDead) {
                rabbit->takeDamage(1);
                pPlayer->mHitEntities.push_back(target);
            }
        }
        else if (Dummy* dummy = dynamic_cast<Dummy*>(target)) {
            if (!dummy->mIsHit) {
                dummy->takeDamage();
                pPlayer->mHitEntities.push_back(target);
            }
        }
        else if (Enemy* enemy = dynamic_cast<Enemy*>(target)) {
            enemy->takeDamage(1);
            pPlayer->mHitEntities.push_back(target);
        }

        // Pogo i recoil
        WeaponStats stats = pPlayer->getCurrentWeaponStats();

        if (pPlayer->mAttackDir == 2) {  // Down attack
            pPlayer->mVelocity.y = stats.pogoForce;
            pPlayer->mIsPogoJump = true;
            pPlayer->mIsJumping = false;
            pPlayer->mJumpBufferCounter = 0;
            pPlayer->mIsDashing = false;
            pPlayer->mCanDash = true;
        }

        if (pPlayer->mAttackDir == 0) {  // Side attack
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
};