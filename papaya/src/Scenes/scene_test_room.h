#pragma once

#include "raylib.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "Saves/save_data.h"
#include "Saves/save_manager.h"
#include "Entities/Entity.h"
#include "Entities/Player.h"
#include "Entities/Wall.h"
#include "Entities/Dummy.h"
#include "Entities/Enemy.h"
#include "Entities/MageBoss.h"

inline void ResolveMapCollision(Player* pPlayer, const std::vector<Wall>& mapWalls, float dt) {
    if (!pPlayer) return;
    Rectangle pRect = pPlayer->getRect();

    pPlayer->mPosition.x += pPlayer->mVelocity.x * dt;
    pRect = pPlayer->getRect();

    for (const auto& wall : mapWalls) {
        if (CheckCollisionRecs(pRect, wall.getRect())) {
            if (pPlayer->mVelocity.x > 0) pPlayer->mPosition.x = wall.getRect().x - pPlayer->mSize.x;
            else if (pPlayer->mVelocity.x < 0) pPlayer->mPosition.x = wall.getRect().x + wall.getRect().width;
            pPlayer->mVelocity.x = 0;
            pRect = pPlayer->getRect();
        }
    }

    pPlayer->mPosition.y += pPlayer->mVelocity.y * dt;
    pRect = pPlayer->getRect();

    for (const auto& wall : mapWalls) {
        if (CheckCollisionRecs(pRect, wall.getRect())) {
            if (pPlayer->mVelocity.y > 0) {
                pPlayer->mPosition.y = wall.getRect().y - pPlayer->mSize.y;
                pPlayer->mVelocity.y = 0;
                pPlayer->onGroundHit();
            }
            else if (pPlayer->mVelocity.y < 0) {
                pPlayer->mPosition.y = wall.getRect().y + wall.getRect().height;
                pPlayer->mVelocity.y = 0;
            }
            pRect = pPlayer->getRect();
        }
    }
}

inline void debugRoom(int windowWidth, int windowHeight) {
    const int gameWidth = 320;
    const int gameHeight = 180;

    RenderTexture2D target = LoadRenderTexture(gameWidth, gameHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    SaveData currentSave;
    Player player(50.0f, 100.0f);

    SaveManager::apply(currentSave, player);

    std::vector<Wall> walls;
    walls.push_back(Wall(0, 160, 600.0f, 20.0f));
    walls.push_back(Wall(150, 120, 50.0f, 10.0f));
    walls.push_back(Wall(250, 90, 50.0f, 10.0f));
    walls.push_back(Wall(400, 120, 20.0f, 40.0f));

    std::vector<Entity*> entities;
    Texture2D bossTex = LoadTexture("Assets/mage_boss.png");

    if (SaveManager::saveExists()) {
        SaveManager::loadFromFile(currentSave);
        std::cout << "[SAVE] Wczytano zapis!" << std::endl;
    }

    if (!SaveManager::isEnemyDead(currentSave, 101)) {
        Dummy* d1 = new Dummy(200, 100);
        d1->mID = 101;
        entities.push_back(d1);
    }

    if (!SaveManager::isEnemyDead(currentSave, 102)) {
        Enemy* e1 = new Enemy(300, 100);
        e1->mID = 102;
        entities.push_back(e1);
    }

    if (!currentSave.boss1Defeated) {
        MageBoss* boss = new MageBoss(450, 50, bossTex, &player);
        boss->mID = 999;
        entities.push_back(boss);
    }

    std::vector<Entity*> wallPointers;
    for (auto& w : walls) wallPointers.push_back(&w);

    Camera2D camera = { 0 };
    camera.offset = { gameWidth / 2.0f, gameHeight / 2.0f };
    camera.zoom = 1.0f;

    bool godMode = false;
    bool showStats = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_F4)) godMode = !godMode;
        if (IsKeyPressed(KEY_F2)) showStats = !showStats;
        if (IsKeyPressed(KEY_R)) {
            player.mPosition = { 50, 100 };
            player.mHealth = player.mMaxHealth;
        }

        if (IsKeyPressed(KEY_P)) {
            SaveManager::setCheckpoint(currentSave, player.mPosition.x, player.mPosition.y, "debug_room");
            SaveManager::saveToFile(currentSave);
            std::cout << "[SAVE] Zapisano!" << std::endl;
        }

        SaveManager::collect(currentSave, player);

        player.update(dt);
        ResolveMapCollision(&player, walls, dt);

        for (auto it = entities.begin(); it != entities.end(); ) {
            Entity* e = *it;

            MageBoss* mage = dynamic_cast<MageBoss*>(e);
            if (mage) mage->updateBossLogic(dt, wallPointers);
            else e->update(dt);

            // === ATAKI GRACZA NA WROGÓW ===
            if (player.mIsAttacking && CheckCollisionRecs(player.mAttackArea, e->getRect())) {
                bool alreadyHit = false;
                for (auto* hit : player.mHitEntities) {
                    if (hit == e) { alreadyHit = true; break; }
                }

                if (!alreadyHit) {
                    if (mage) mage->takeDamage(player.mAttackDamage);
                    else if (Dummy* dum = dynamic_cast<Dummy*>(e)) dum->takeDamage();
                    else if (Enemy* en = dynamic_cast<Enemy*>(e)) en->takeDamage(player.mAttackDamage);

                    player.mHitEntities.push_back(e);

                    float playerBottom = player.mPosition.y + player.mSize.y;
                    float playerTop = player.mPosition.y;
                    float enemyCenterY = e->mPosition.y + e->mSize.y / 2.0f;

                    bool isBelow = (playerTop > enemyCenterY);
                    bool isPogo = (playerBottom < enemyCenterY) && (player.mVelocity.y >= 0);

                    if (isBelow) {
                        // nic
                    }
                    else if (isPogo) {
                        player.mVelocity.y = -250.0f;
                        player.mCanDash = true;
                        player.mCanDoubleJump = true;
                        player.mCanWavedash = true;
                    }
                    else {
                        float recoilForce = 150.0f;

                        if (player.mPosition.x < e->mPosition.x) {
                            player.mVelocity.x = -recoilForce;
                        }
                        else {
                            player.mVelocity.x = recoilForce;
                        }
                    }
                }
            }

            // === ATAKI WROGÓW NA GRACZA ===
            if (!godMode) {
                // Kolizja z ciałem wroga (nie Dummy)
                if (CheckCollisionRecs(player.getRect(), e->getRect())) {
                    if (!dynamic_cast<Dummy*>(e)) {
                        player.takeDamage(1);
                    }
                }

                // Specjalne ataki bossa
                if (mage && mage->mState != MageBoss::DYING && mage->mState != MageBoss::INACTIVE) {

                    // 1. FIREBALLE
                    for (auto& fb : mage->mFireballs) {
                        if (!fb.active) continue;

                        if (CheckCollisionCircleRec(fb.position, fb.radius, player.getRect())) {
                            player.takeDamage(1);
                            fb.active = false;
                        }
                    }

                    // 2. LASER
                    if (mage->mState == MageBoss::LASER_FIRE) {
                        if (mage->checkLaserCollision(&player)) {
                            player.takeDamage(1);
                        }
                    }

                    // 3. AOE ATAK
                    if (mage->checkAoeCollision(&player)) {
                        player.takeDamage(2);
                        mage->mAoeDealtDamage = true;
                    }
                }
            }

            // === SPRAWDZANIE ŚMIERCI WROGÓW ===
            bool isDead = false;

            if (mage && mage->mState == MageBoss::DYING && !mage->mActive) {
                isDead = true;
                currentSave.boss1Defeated = true;
            }
            if (Enemy* en = dynamic_cast<Enemy*>(e)) {
                if (en->mIsDead) isDead = true;
            }
            if (Dummy* dum = dynamic_cast<Dummy*>(e)) {
                if (dum->mIsHit) isDead = true;
            }

            if (isDead && e->mID != -1) {
                SaveManager::killEnemy(currentSave, e->mID);
                SaveManager::saveToFile(currentSave);
                std::cout << "[SAVE] Wrog ID " << e->mID << " zabity!" << std::endl;
                delete e;
                it = entities.erase(it);
            }
            else {
                ++it;
            }
        }

        camera.target.x = std::floor(player.mPosition.x);
        camera.target.y = std::floor(player.mPosition.y);

        BeginTextureMode(target);
        ClearBackground({ 40, 44, 52, 255 });
        BeginMode2D(camera);

        for (const auto& w : walls) DrawRectangleRec(w.getRect(), DARKGRAY);

        for (auto* e : entities) {
            e->draw();
            DrawText(TextFormat("ID:%d", e->mID), (int)e->mPosition.x, (int)e->mPosition.y - 12, 5, YELLOW);
        }

        player.draw();

        if (currentSave.checkpointX != 0 || currentSave.checkpointY != 0) {
            DrawCircle((int)currentSave.checkpointX, (int)currentSave.checkpointY, 4, GREEN);
        }

        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(target.texture,
            { 0, 0, (float)target.texture.width, -(float)target.texture.height },
            { 0, 0, (float)windowWidth, (float)windowHeight },
            { 0, 0 }, 0.0f, WHITE);

        if (showStats) {
            int x = 10, y = 10;

            DrawRectangle(x - 5, y - 5, 220, 200, Fade(BLACK, 0.8f));
            DrawText("=== SAVE DATA ===", x, y, 16, YELLOW);
            y += 25;

            DrawText(TextFormat("Checkpoint: %.0f, %.0f", currentSave.checkpointX, currentSave.checkpointY), x, y, 10, WHITE);
            y += 15;

            DrawText(TextFormat("Player HP: %d/%d", player.mHealth, player.mMaxHealth), x, y, 10,
                player.mHealth > 2 ? GREEN : RED);
            y += 15;

            DrawText(TextFormat("God Mode: %s", godMode ? "ON" : "OFF"), x, y, 10,
                godMode ? YELLOW : GRAY);
            y += 15;

            DrawText("--- UNLOCKS ---", x, y, 10, SKYBLUE); y += 12;
            DrawText(TextFormat("Double Jump: %s", currentSave.hasDoubleJump ? "YES" : "NO"), x, y, 10,
                currentSave.hasDoubleJump ? GREEN : GRAY); y += 12;
            DrawText(TextFormat("Wave Dash: %s", currentSave.hasWaveDash ? "YES" : "NO"), x, y, 10,
                currentSave.hasWaveDash ? GREEN : GRAY); y += 15;

            DrawText("--- BOSS ---", x, y, 10, SKYBLUE); y += 12;
            DrawText(TextFormat("Mage Boss: %s", currentSave.boss1Defeated ? "DEFEATED" : "ALIVE"), x, y, 10,
                currentSave.boss1Defeated ? GREEN : RED); y += 15;

            DrawText("--- DEAD ENEMIES ---", x, y, 10, SKYBLUE); y += 12;
            if (currentSave.deadEnemies.empty()) {
                DrawText("(none)", x, y, 10, GRAY);
            }
            else {
                std::string ids = "";
                for (int id : currentSave.deadEnemies) {
                    ids += std::to_string(id) + " ";
                }
                DrawText(ids.c_str(), x, y, 10, RED);
            }
        }

        DrawText("F2 - Stats | F4 - God | R - Reset | P - Checkpoint", 10, windowHeight - 25, 12, GRAY);
        DrawFPS(windowWidth - 80, 10);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadTexture(bossTex);
    for (auto* e : entities) delete e;

    std::cout << "\n=== FINAL SAVE ===" << std::endl;
    std::cout << "Checkpoint: " << currentSave.checkpointX << ", " << currentSave.checkpointY << std::endl;
    std::cout << "Boss defeated: " << (currentSave.boss1Defeated ? "YES" : "NO") << std::endl;
    std::cout << "Dead enemies: " << currentSave.deadEnemies.size() << std::endl;
}