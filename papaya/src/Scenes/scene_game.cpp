#include "scene_game.hpp"
#include "raylib.h"

#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include <string>
#include <cmath>

#include "../Core/GameConstants.hpp"
#include "../Core/GameWrapper.hpp"
#include "../Core/Physics.hpp"

#include "../Saves/saves.hpp"
#include "../Entities/Entity.h"
#include "../Entities/Player/Player.h"
#include "../Entities/Environment/Wall.h"
#include "../Entities/Enemies/mage_boss.h"
#include "../Entities/Enemies/rabbit.h"

using namespace GameConstants;

void sceneGame(int windowWidth, int windowHeight, bool& shouldQuit, int& state) {
    SetTargetFPS(60);

    {
        Saves tempSaves("assets/saves/main_save");

        if (tempSaves.getMap().getRenderData().empty()) {
            std::string jsonPath = "assets/editor";
            std::cout << "[SYSTEM] Wykryto pustą mapę. Próba importu z '" << jsonPath << "'..." << std::endl;

            try {
                tempSaves.loadFromEditorDir(jsonPath);
                std::cout << "[SYSTEM] Import zakończony." << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "[ERROR] Błąd importu mapy: " << e.what() << std::endl;
            }
        }
    }

    Saves saves("assets/saves/main_save");

    Texture2D tileset = LoadTexture(TILESET_PATH.c_str());
    Texture2D playerTexture = LoadTexture(PLAYER_TEXTURE_PATH.c_str());
    Texture2D bossTexture = LoadTexture(BOSS_TEXTURE_PATH.c_str());
    Texture2D rabbitTexture = LoadTexture(RABBIT_TEXTURE_PATH.c_str());

    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(playerTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(bossTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(rabbitTexture, TEXTURE_FILTER_POINT);

    GameMap gameMap;
    gameMap.init(saves.getMap().getRenderData());

    std::cout << "[INFO] Mapa: " << gameMap.width << "x" << gameMap.height
        << " min=(" << gameMap.minX << "," << gameMap.minY << ")" << std::endl;

    SpatialGrid staticCollisionGrid;
    staticCollisionGrid.init(
        gameMap.width * TILE_SIZE_PX + 1000,
        gameMap.height * TILE_SIZE_PX + 1500,
        gameMap.minX - 500,
        gameMap.minY - 1000
    );

    std::vector<std::unique_ptr<Entity>> activeEntities;
    std::vector<Entity*> wallEntities;
    Player* playerPtr = nullptr;

    std::vector<MageBoss*> bosses;
    std::vector<RabbitEnemy*> rabbits;
    std::vector<BossSpawnData> bossSpawnPoints;

    {
        std::vector<CollisionRect> collisions = saves.getMap().getCollisions();
        std::cout << "[INFO] Załadowano " << collisions.size() << " kolizji." << std::endl;

        for (const auto& col : collisions) {
            auto wallEnt = std::make_unique<Wall>(
                static_cast<float>(col.x),
                static_cast<float>(col.y),
                static_cast<float>(col.w),
                static_cast<float>(col.h)
            );
            staticCollisionGrid.insertStatic(wallEnt.get());
            wallEntities.push_back(wallEnt.get());
            activeEntities.push_back(std::move(wallEnt));
        }
    }

    Vector2 playerSpawnPoint = { 464.5f, 442.0f };
    {
        auto playerEntity = std::make_unique<Player>(playerSpawnPoint.x, playerSpawnPoint.y);
        playerEntity->mTexture = playerTexture;
        playerPtr = playerEntity.get();
        activeEntities.push_back(std::move(playerEntity));
    }

    {
        auto rawEntities = saves.getEntities().getAll();
        std::cout << "[DEBUG] Liczba encji w pliku binarnym: " << rawEntities.size() << std::endl;

        for (const auto& eData : rawEntities) {
            std::cout << "[DEBUG] Znaleziono encję -> ID: " << (int)eData.entityType
                << " X: " << eData.x << " Y: " << eData.y
                << " Health: " << eData.health << std::endl;

            if (eData.entityType == PLAYER) continue;

            float pixelX = static_cast<float>(eData.x * TILE_SIZE_PX);
            float pixelY = static_cast<float>(eData.y * TILE_SIZE_PX);

            if (eData.entityType == MAGE_BOSS) {
                auto boss = std::make_unique<MageBoss>(pixelX, pixelY, bossTexture, playerPtr);
                Rectangle arena = {
                    boss->mPosition.x - 116.0f,
                    boss->mPosition.y - 50.0f,
                    232.0f,
                    100.0f
                };
                boss->setArenaBounds(arena.x, arena.y, arena.width, arena.height);

                std::cout << "[DEBUG] Boss created with HP: " << boss->mHealth
                    << "/" << boss->mMaxHealth << std::endl;

                bossSpawnPoints.push_back({ {pixelX, pixelY}, arena, boss->mMaxHealth });
                bosses.push_back(boss.get());
                activeEntities.push_back(std::move(boss));
            }
            else if (eData.entityType == RABBIT) {
                auto rabbit = std::make_unique<RabbitEnemy>(pixelX, pixelY, rabbitTexture, playerPtr);
                if (eData.health > 0) rabbit->mHealth = eData.health;
                rabbits.push_back(rabbit.get());
                activeEntities.push_back(std::move(rabbit));
            }
        }
    }

    if (bosses.empty() && rabbits.empty()) {
        std::cout << "[INFO] Brak wrogów w pliku - tworzę testowych." << std::endl;

        auto testBoss = std::make_unique<MageBoss>(666.0f, -376.0f, bossTexture, playerPtr);
        Rectangle arena = { 550.0f, -448.0f, 232.0f, 100.0f };
        testBoss->setArenaBounds(arena.x, arena.y, arena.width, arena.height);

        std::cout << "[DEBUG] Test boss created with HP: " << testBoss->mHealth
            << "/" << testBoss->mMaxHealth << std::endl;

        bossSpawnPoints.push_back({ {666.0f, -376.0f}, arena, testBoss->mMaxHealth });
        bosses.push_back(testBoss.get());
        activeEntities.push_back(std::move(testBoss));

        auto testRabbit = std::make_unique<RabbitEnemy>(270.0f, 322.0f, rabbitTexture, playerPtr);
        rabbits.push_back(testRabbit.get());
        activeEntities.push_back(std::move(testRabbit));
    }

    std::cout << "[INFO] Bosses: " << bosses.size() << ", Rabbits: " << rabbits.size() << std::endl;

    SmoothCamera smoothCam;
    if (playerPtr) smoothCam.setPosition(playerPtr->mPosition);

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = { (float)VIRTUAL_WIDTH / 2, (float)VIRTUAL_HEIGHT / 2 };

    RenderTexture2D target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    std::vector<Entity*> nearbyObstacles;
    bool showDebug = false;

    CheatSystem cheats;
    WeaponUIState weaponUI;

    auto respawnAllBosses = [&]() {
        int bossIndex = 0;
        for (MageBoss* boss : bosses) {
            if (boss && bossIndex < (int)bossSpawnPoints.size()) {
                const auto& spawn = bossSpawnPoints[bossIndex];

                boss->mPosition = spawn.position;
                boss->mStartY = spawn.position.y;
                boss->mPrevPosition = spawn.position;

                boss->mHealth = spawn.maxHealth;
                boss->mMaxHealth = spawn.maxHealth;

                boss->mState = MageBoss::INACTIVE;
                boss->mActive = true;
                boss->mIsEnraged = false;
                boss->mDualLaser = false;
                boss->mTeleportAttack = false;

                boss->mStateTimer = 0.0f;
                boss->mHurtTimer = 0.0f;
                boss->mFloatOffset = 0.0f;
                boss->mTeleportAlpha = 1.0f;
                boss->mLaserInitialized = false;

                boss->mCurrentFrame = 0;
                boss->mFrameTimer = 0.0f;

                boss->mFireballs.clear();
                boss->mAttackCounter = 0;
                boss->mCurrentBurst = 0;
                boss->mAoeRadius = 0.0f;
                boss->mAoeDealtDamage = false;

                boss->mIdleTime = boss->mIdleTimeBase;
                boss->mLaserChargeTime = boss->mLaserChargeTimeBase;
                boss->mLaserFireTime = boss->mLaserFireTimeBase;
                boss->mLaserTrackSpeedCharge = boss->mLaserTrackSpeedChargeBase;
                boss->mLaserTrackSpeedFire = boss->mLaserTrackSpeedFireBase;
                boss->mFireballCount = boss->mFireballCountBase;
                boss->mFireballSpeed = 320.0f;
                boss->mMoveSpeed = 70.0f;
                boss->mAoeMaxRadius = 100.0f;
                boss->mAoeWarningTime = 1.4f;

                boss->setArenaBounds(
                    spawn.arenaBounds.x,
                    spawn.arenaBounds.y,
                    spawn.arenaBounds.width,
                    spawn.arenaBounds.height
                );

                std::cout << "[DEBUG] Boss respawned with HP: " << boss->mHealth
                    << "/" << boss->mMaxHealth << std::endl;

                bossIndex++;
            }
        }
        cheats.showMessage("BOSSES RESPAWNED!");
        };

    auto respawnPlayer = [&]() {
        if (playerPtr) {
            playerPtr->mPosition = playerSpawnPoint;
            playerPtr->mVelocity = { 0, 0 };
            playerPtr->mHealth = playerPtr->mMaxHealth;
            playerPtr->mInvincibilityTimer = 1.0f;
            playerPtr->mActive = true;
            smoothCam.setPosition(playerSpawnPoint);
        }
        cheats.showMessage("PLAYER RESPAWNED!");
        };

    while (!WindowShouldClose() && !shouldQuit) {
        float dt = std::min(GetFrameTime(), 0.033f);

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = 0;
            break;
        }
        if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

        if (playerPtr) {
            bool weaponChanged = false;

            if (IsKeyPressed(KEY_ONE)) {
                playerPtr->setWeapon(WeaponType::SWORD_DEFAULT);
                weaponChanged = true;
            }
            else if (IsKeyPressed(KEY_TWO)) {
                playerPtr->setWeapon(WeaponType::DAGGER_SWIFT);
                weaponChanged = true;
            }
            else if (IsKeyPressed(KEY_THREE)) {
                playerPtr->setWeapon(WeaponType::AXE_HEAVY);
                weaponChanged = true;
            }
            else if (IsKeyPressed(KEY_FOUR)) {
                playerPtr->setWeapon(WeaponType::SPEAR_LONG);
                weaponChanged = true;
            }
            else if (IsKeyPressed(KEY_FIVE)) {
                playerPtr->setWeapon(WeaponType::KATANA_BLOOD);
                weaponChanged = true;
            }

            if (weaponChanged) {
                weaponUI.triggerChange();
                WeaponStats stats = playerPtr->getCurrentWeaponStats();
                std::cout << "[WEAPON] Zmieniono broń" << std::endl;
            }
        }

        if (IsKeyPressed(KEY_G)) {
            cheats.godMode = !cheats.godMode;
            cheats.showMessage(cheats.godMode ? "GODMODE: ON" : "GODMODE: OFF");
        }

        if (IsKeyPressed(KEY_N)) {
            cheats.noclip = !cheats.noclip;
            cheats.showMessage(cheats.noclip ? "NOCLIP: ON" : "NOCLIP: OFF");
        }

        if (IsKeyPressed(KEY_R)) {
            respawnAllBosses();
        }

        if (IsKeyPressed(KEY_T)) {
            if (playerPtr && !bosses.empty()) {
                MageBoss* nearestBoss = bosses[0];
                playerPtr->mPosition = {
                    nearestBoss->mPosition.x - 60.0f,
                    nearestBoss->mPosition.y
                };
                playerPtr->mVelocity = { 0, 0 };
                smoothCam.setPosition(playerPtr->mPosition);
                cheats.showMessage("TELEPORTED TO BOSS!");
            }
        }

        if (IsKeyPressed(KEY_H)) {
            if (playerPtr) {
                playerPtr->mHealth = playerPtr->mMaxHealth;
                cheats.showMessage("HEALTH RESTORED!");
            }
        }

        if (IsKeyPressed(KEY_K)) {
            for (MageBoss* boss : bosses) {
                if (boss && boss->mActive) {
                    boss->takeDamage(9999);
                }
            }
            cheats.showMessage("BOSSES KILLED!");
        }

        if (IsKeyPressed(KEY_L)) {
            for (MageBoss* boss : bosses) {
                if (boss && boss->mActive && boss->mState != MageBoss::DYING) {
                    boss->takeDamage(10);
                    cheats.showMessage(TextFormat("BOSS DAMAGED! HP: %d/%d", boss->mHealth, boss->mMaxHealth));
                    break;
                }
            }
        }

        if (IsKeyPressed(KEY_F1)) {
            respawnPlayer();
            respawnAllBosses();
            cheats.showMessage("FULL RESET!");
        }

        if (IsKeyPressed(KEY_F2)) {
            for (MageBoss* boss : bosses) {
                if (boss && boss->mActive && !boss->mIsEnraged) {
                    boss->mHealth = (int)(boss->mMaxHealth * 0.4f);
                    boss->checkEnrage();
                    cheats.showMessage(TextFormat("BOSS ENRAGED! HP: %d/%d", boss->mHealth, boss->mMaxHealth));
                    break;
                }
            }
        }

        cheats.update(dt);
        weaponUI.update(dt);

        if (playerPtr && playerPtr->mActive) {
            playerPtr->update(dt);

            if (cheats.godMode) {
                playerPtr->mHealth = playerPtr->mMaxHealth;
                playerPtr->mInvincibilityTimer = 0.5f;
            }
        }

        for (MageBoss* boss : bosses) {
            if (!boss || !boss->mActive) continue;
            boss->updateBossLogic(dt, wallEntities);
        }

        for (RabbitEnemy* rabbit : rabbits) {
            if (!rabbit || !rabbit->mActive) continue;
            rabbit->update(dt);
        }

        if (!cheats.noclip) {
            for (auto& ent : activeEntities) {
                if (!ent || !ent->mActive || ent->mType == WALL) continue;

                nearbyObstacles.clear();
                staticCollisionGrid.getNearby(ent->mPosition.x, ent->mPosition.y, nearbyObstacles);

                for (Entity* obs : nearbyObstacles) {
                    if (CheckCollisionRecs(ent->getRect(), obs->getRect())) {
                        ent->onCollision(obs);
                    }
                }
            }
        }
        else {
            for (auto& ent : activeEntities) {
                if (!ent || !ent->mActive || ent->mType == WALL) continue;
                if (ent.get() == playerPtr) continue;

                nearbyObstacles.clear();
                staticCollisionGrid.getNearby(ent->mPosition.x, ent->mPosition.y, nearbyObstacles);

                for (Entity* obs : nearbyObstacles) {
                    if (CheckCollisionRecs(ent->getRect(), obs->getRect())) {
                        ent->onCollision(obs);
                    }
                }
            }
        }

        if (playerPtr && playerPtr->mActive) {

            Vector2 playerCenter = Physics::GetEntityCenter(playerPtr);

            for (MageBoss* boss : bosses) {
                if (!boss || !boss->mActive) continue;

                if (playerPtr->mIsAttacking) {
                    if (CheckCollisionRecs(playerPtr->mAttackArea, boss->getRect())) {
                        if (!playerPtr->hasHit(boss)) {

                            Vector2 enemyCenter = Physics::GetEntityCenter(boss);

                            nearbyObstacles.clear();
                            staticCollisionGrid.getNearby(playerCenter.x, playerCenter.y, nearbyObstacles);

                            if (Physics::IsPathBlocked(playerCenter, enemyCenter, nearbyObstacles)) {
                                continue;
                            }

                            if (playerPtr->mAttackDir == AttackDirection::DOWN) {
                                playerPtr->pogoBounce();
                            }

                            playerPtr->mHitEntities.push_back(boss);
                            int damage = playerPtr->getAttackDamage();
                            boss->takeDamage(damage);
                        }
                    }
                }

                if (boss->mState != MageBoss::VULNERABLE &&
                    boss->mState != MageBoss::DYING &&
                    boss->mState != MageBoss::INACTIVE) {
                    if (CheckCollisionRecs(playerPtr->getRect(), boss->getRect())) {
                        if (playerPtr->mInvincibilityTimer <= 0) {
                            playerPtr->mHealth -= 1;
                            playerPtr->mInvincibilityTimer = 1.0f;
                            float knockDir = Physics::GetKnockbackDirection(boss->mPosition.x, playerPtr->mPosition.x);
                            Physics::ApplyKnockback(playerPtr->mVelocity, knockDir, 120.0f, 80.0f);
                        }
                    }
                }

                if (cheats.godMode) continue;

                for (auto& fb : boss->mFireballs) {
                    if (!fb.active) continue;
                    if (playerPtr->mInvincibilityTimer <= 0) {
                        Rectangle playerRect = playerPtr->getRect();
                        if (CheckCollisionCircleRec(fb.position, fb.radius, playerRect)) {
                            playerPtr->mHealth -= 1;
                            playerPtr->mInvincibilityTimer = 1.0f;
                            float knockDir = Physics::GetKnockbackDirection(fb.position.x, playerPtr->mPosition.x);
                            Physics::ApplyKnockback(playerPtr->mVelocity, knockDir, 150.0f, 100.0f);
                            fb.active = false;
                        }
                    }
                }

                if (boss->mState == MageBoss::LASER_FIRE) {
                    if (playerPtr->mInvincibilityTimer <= 0) {
                        if (boss->checkLaserCollision(playerPtr)) {
                            playerPtr->mHealth -= 2;
                            playerPtr->mInvincibilityTimer = 1.5f;
                            float knockDir = Physics::GetKnockbackDirection(boss->mPosition.x, playerPtr->mPosition.x);
                            Physics::ApplyKnockback(playerPtr->mVelocity, knockDir, 200.0f, 150.0f);
                        }
                    }
                }

                if (boss->checkAoeCollision(playerPtr)) {
                    if (playerPtr->mInvincibilityTimer <= 0) {
                        playerPtr->mHealth -= 2;
                        playerPtr->mInvincibilityTimer = 1.5f;
                        float dx = playerPtr->mPosition.x - boss->mPosition.x;
                        float dy = playerPtr->mPosition.y - boss->mPosition.y;
                        float len = std::sqrt(dx * dx + dy * dy);
                        if (len > 0) {
                            playerPtr->mVelocity.x = (dx / len) * 250.0f;
                            playerPtr->mVelocity.y = (dy / len) * 250.0f - 100.0f;
                        }
                        boss->mAoeDealtDamage = true;
                    }
                }
            }

            for (RabbitEnemy* rabbit : rabbits) {
                if (!rabbit || !rabbit->mActive || rabbit->mIsDead) continue;

                if (playerPtr->mIsAttacking) {
                    if (CheckCollisionRecs(playerPtr->mAttackArea, rabbit->getRect())) {
                        if (!playerPtr->hasHit(rabbit)) {

                            Vector2 enemyCenter = Physics::GetEntityCenter(rabbit);

                            nearbyObstacles.clear();
                            staticCollisionGrid.getNearby(playerCenter.x, playerCenter.y, nearbyObstacles);

                            if (Physics::IsPathBlocked(playerCenter, enemyCenter, nearbyObstacles)) {
                                continue;
                            }

                            if (playerPtr->mAttackDir == AttackDirection::DOWN) {
                                playerPtr->pogoBounce();
                            }

                            playerPtr->mHitEntities.push_back(rabbit);
                            int damage = playerPtr->getAttackDamage();
                            rabbit->takeDamage(damage);

                            if (rabbit->mIsDead) {
                                rabbit->mActive = false;
                            }
                        }
                    }
                }

                if (cheats.godMode) continue;

                if (!rabbit->mIsDead) {
                    if (CheckCollisionRecs(playerPtr->getRect(), rabbit->getRect())) {
                        if (playerPtr->mInvincibilityTimer <= 0) {
                            int damage = 1;
                            if (rabbit->mState >= RabbitEnemy::MONSTER_CHARGE &&
                                rabbit->mState <= RabbitEnemy::MONSTER_TURN) {
                                damage = 2;
                            }
                            playerPtr->mHealth -= damage;
                            playerPtr->mInvincibilityTimer = 1.0f;
                            float knockDir = Physics::GetKnockbackDirection(rabbit->mPosition.x, playerPtr->mPosition.x);
                            Physics::ApplyKnockback(playerPtr->mVelocity, knockDir, 100.0f, 60.0f);
                        }
                    }
                }
            }
        }

        if (playerPtr && playerPtr->mActive) {
            playerPtr->lateUpdate(dt);
            smoothCam.update(playerPtr->mPosition, dt);
            camera.target = smoothCam.getPosition();
        }

        BeginTextureMode(target);
        ClearBackground({ 40, 44, 52, 255 });
        BeginMode2D(camera);

        int camL = (int)(camera.target.x - VIRTUAL_WIDTH / 2 - gameMap.minX) / TILE_SIZE_PX - 1;
        int camR = (int)(camera.target.x + VIRTUAL_WIDTH / 2 - gameMap.minX) / TILE_SIZE_PX + 2;
        int camT = (int)(camera.target.y - VIRTUAL_HEIGHT / 2 - gameMap.minY) / TILE_SIZE_PX - 1;
        int camB = (int)(camera.target.y + VIRTUAL_HEIGHT / 2 - gameMap.minY) / TILE_SIZE_PX + 2;

        for (int gy = std::max(0, camT); gy < std::min(gameMap.height, camB); ++gy) {
            for (int gx = std::max(0, camL); gx < std::min(gameMap.width, camR); ++gx) {
                int id = gameMap.getTileAt(gx, gy);
                if (id == 0) continue;
                Rectangle src = {
                    (float)(id % ATLAS_COLUMNS) * TILE_SIZE_PX,
                    (float)(id / ATLAS_COLUMNS) * TILE_SIZE_PX,
                    (float)TILE_SIZE_PX,
                    (float)TILE_SIZE_PX
                };
                Vector2 pos = {
                    (float)(gx * TILE_SIZE_PX + gameMap.minX),
                    (float)(gy * TILE_SIZE_PX + gameMap.minY)
                };
                DrawTextureRec(tileset, src, pos, WHITE);
            }
        }

        for (RabbitEnemy* rabbit : rabbits) {
            if (rabbit && rabbit->mActive) {
                rabbit->draw();
            }
        }

        for (MageBoss* boss : bosses) {
            if (boss && boss->mActive) {
                boss->draw();
            }
        }

        if (playerPtr && playerPtr->mActive) {
            playerPtr->draw();

            if (cheats.godMode) {
                DrawCircleLines(
                    (int)(playerPtr->mPosition.x + playerPtr->mSize.x / 2),
                    (int)(playerPtr->mPosition.y + playerPtr->mSize.y / 2),
                    (int)(playerPtr->mSize.x * 0.8f + sinf(GetTime() * 5) * 3),
                    Fade(GOLD, 0.5f)
                );
            }

            if (cheats.noclip) {
                DrawCircleLines(
                    (int)(playerPtr->mPosition.x + playerPtr->mSize.x / 2),
                    (int)(playerPtr->mPosition.y + playerPtr->mSize.y / 2),
                    (int)(playerPtr->mSize.x * 0.6f),
                    Fade(SKYBLUE, 0.6f)
                );
            }
        }

        if (showDebug) {
            if (playerPtr) {
                DrawRectangleLinesEx(playerPtr->getRect(), 1, GREEN);
                if (playerPtr->mIsAttacking) {
                    DrawRectangleLinesEx(playerPtr->mAttackArea, 1, YELLOW);
                }
                DrawText(TextFormat("%.0f,%.0f", playerPtr->mPosition.x, playerPtr->mPosition.y),
                    (int)playerPtr->mPosition.x - 15, (int)playerPtr->mPosition.y - 20, 5, LIME);
            }

            for (RabbitEnemy* rabbit : rabbits) {
                if (rabbit && rabbit->mActive) {
                    DrawRectangleLinesEx(rabbit->getRect(), 1, ORANGE);
                }
            }

            for (MageBoss* boss : bosses) {
                if (boss && boss->mActive) {
                    DrawText(TextFormat("HP:%d/%d", boss->mHealth, boss->mMaxHealth),
                        (int)boss->mPosition.x, (int)boss->mPosition.y - 30, 6,
                        boss->mIsEnraged ? ORANGE : GREEN);
                }
            }
        }

        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        float scale = std::min((float)windowWidth / VIRTUAL_WIDTH, (float)windowHeight / VIRTUAL_HEIGHT);
        DrawTexturePro(
            target.texture,
            { 0, 0, (float)target.texture.width, -(float)target.texture.height },
            { (windowWidth - VIRTUAL_WIDTH * scale) / 2, (windowHeight - VIRTUAL_HEIGHT * scale) / 2,
              VIRTUAL_WIDTH * scale, VIRTUAL_HEIGHT * scale },
            { 0, 0 }, 0, WHITE
        );

        if (playerPtr) {
            Color hpColor = cheats.godMode ? GOLD : RED;
            DrawText(TextFormat("HP: %d", playerPtr->mHealth), 20, 20, 24, hpColor);

            int cheatY = 50;
            if (cheats.godMode) {
                DrawText("GOD", 20, cheatY, 16, GOLD);
                cheatY += 18;
            }
            if (cheats.noclip) {
                DrawText("NOCLIP", 20, cheatY, 16, SKYBLUE);
            }
        }

        if (cheats.hasActiveMessage()) {
            float alpha = std::min(cheats.messageTimer, 1.0f);
            int textWidth = MeasureText(cheats.message.c_str(), 24);
            DrawText(cheats.message.c_str(),
                (windowWidth - textWidth) / 2,
                windowHeight / 2 - 50,
                24,
                Fade(YELLOW, alpha));
        }

        for (MageBoss* boss : bosses) {
            if (boss && boss->mActive && boss->mState != MageBoss::INACTIVE) {
                float bossBarWidth = 200.0f;
                float bossBarX = (windowWidth - bossBarWidth) / 2;
                float bossBarY = 30.0f;
                float hpPercent = (float)boss->mHealth / (float)boss->mMaxHealth;

                DrawRectangle((int)bossBarX - 2, (int)bossBarY - 2, (int)bossBarWidth + 4, 14, BLACK);
                DrawRectangle((int)bossBarX, (int)bossBarY, (int)bossBarWidth, 10, DARKGRAY);
                DrawRectangle((int)bossBarX, (int)bossBarY, (int)(bossBarWidth * hpPercent), 10,
                    boss->mIsEnraged ? RED : MAROON);

                DrawText("MAGE BOSS", (int)bossBarX, (int)bossBarY - 18, 16, WHITE);

                if (boss->mIsEnraged) {
                    DrawText("ENRAGED!", (int)(bossBarX + bossBarWidth + 10), (int)bossBarY - 2, 12, RED);
                }
                break;
            }
        }

        int controlsY = windowHeight - 30;
        DrawRectangle(0, controlsY - 5, windowWidth, 35, Fade(BLACK, 0.8f));
        DrawText("[1-5] Bron", 20, controlsY, 14, YELLOW);
        const char* cheatsText = "[G]od [N]oclip [R]espawn [T]TP [H]eal [K]ill [F1]Reset";
        int cheatWidth = MeasureText(cheatsText, 12);
        DrawText(cheatsText, (windowWidth - cheatWidth) / 2, controlsY + 2, 12, GRAY);
        DrawText("[F3] Debug", windowWidth - 120, controlsY, 14, GRAY);

        EndDrawing();
    }

    UnloadTexture(bossTexture);
    UnloadTexture(rabbitTexture);
    UnloadTexture(playerTexture);
    UnloadTexture(tileset);
    UnloadRenderTexture(target);
}