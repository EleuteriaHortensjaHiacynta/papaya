#include "scene_game.hpp"
#include "raylib.h"
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include <string>
#include <cmath>

#include "../Saves/saves.hpp"
#include "../Entities/Entity.h"
#include "../Entities/Player.h"
#include "../Entities/Wall.h" 

// --- STAŁE ---
const int VIRTUAL_WIDTH = 320;
const int VIRTUAL_HEIGHT = 180;
const int TILE_SIZE_PX = 8;
const int ATLAS_COLUMNS = 64;
const std::string TILESET_PATH = "assets/tiles/atlas_512x512.png";

// --- STRUKTURY POMOCNICZE ---

struct GameMap {
    int width = 0, height = 0;
    int minX = 0, minY = 0;
    std::vector<int> tiles;

    void init(const std::vector<RenderTile>& rawTiles) {
        if (rawTiles.empty()) return;
        int minPixelX = 999999, minPixelY = 999999;
        int maxPixelX = -999999, maxPixelY = -999999;
        for (const auto& t : rawTiles) {
            if (t.x < minPixelX) minPixelX = t.x;
            if (t.y < minPixelY) minPixelY = t.y;
            if (t.x > maxPixelX) maxPixelX = t.x;
            if (t.y > maxPixelY) maxPixelY = t.y;
        }
        minX = minPixelX;
        minY = minPixelY;
        width = (maxPixelX - minPixelX) / TILE_SIZE_PX + 1;
        height = (maxPixelY - minPixelY) / TILE_SIZE_PX + 1;
        tiles.assign(width * height, 0);
        for (const auto& t : rawTiles) {
            int gx = (t.x - minX) / TILE_SIZE_PX;
            int gy = (t.y - minY) / TILE_SIZE_PX;
            if (gx >= 0 && gx < width && gy >= 0 && gy < height)
                tiles[gy * width + gx] = static_cast<int>(t.textureID);
        }
    }

    int getTileAt(int gridX, int gridY) const {
        if (gridX < 0 || gridX >= width || gridY < 0 || gridY >= height) return 0;
        return tiles[gridY * width + gridX];
    }
};

struct SpatialGrid {
    std::vector<std::vector<Entity*>> cells;
    int cellSize = 64;
    int width = 0, height = 0;
    int offsetX = 0, offsetY = 0;

    void init(int mapW, int mapH, int minX, int minY) {
        offsetX = minX;
        offsetY = minY;
        width = (mapW / cellSize) + 2;
        height = (mapH / cellSize) + 2;
        cells.assign(width * height, {});
    }

    void insertStatic(Entity* e) {
        int cx = std::clamp(((int)e->mPosition.x - offsetX) / cellSize, 0, width - 1);
        int cy = std::clamp(((int)e->mPosition.y - offsetY) / cellSize, 0, height - 1);
        cells[cy * width + cx].push_back(e);
    }

    void getNearby(float x, float y, std::vector<Entity*>& outResult) {
        int cx = ((int)x - offsetX) / cellSize;
        int cy = ((int)y - offsetY) / cellSize;
        for (int ny = cy - 1; ny <= cy + 1; ny++) {
            for (int nx = cx - 1; nx <= cx + 1; nx++) {
                if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                    const auto& cell = cells[ny * width + nx];
                    outResult.insert(outResult.end(), cell.begin(), cell.end());
                }
            }
        }
    }
};

struct SmoothCamera {
    Vector2 currentPos = { 0, 0 };
    float smoothSpeed = 12.0f;
    float deadZoneX = 15.0f;
    float deadZoneY = 5.0f;

    void update(Vector2 target, float dt) {
        float dx = target.x - currentPos.x;
        float dy = target.y - currentPos.y;
        float targetX = currentPos.x;
        float targetY = currentPos.y;

        if (std::abs(dx) > deadZoneX) {
            targetX = (dx > 0) ? target.x - deadZoneX : target.x + deadZoneX;
        }
        if (std::abs(dy) > deadZoneY) {
            targetY = (dy > 0) ? target.y - deadZoneY : target.y + deadZoneY;
        }

        currentPos.x += (targetX - currentPos.x) * smoothSpeed * dt;
        currentPos.y += (targetY - currentPos.y) * smoothSpeed * dt;
    }

    Vector2 getPosition() {
        return { std::floor(currentPos.x), std::floor(currentPos.y) };
    }
};

// --- GŁÓWNA FUNKCJA SCENY ---

void sceneGame(int windowWidth, int windowHeight, bool& shouldQuit, int& state) {
    Saves saves("assets/saves/main_save");
    SetTargetFPS(60);

    // ==========================================
    // TEKSTURY
    // ==========================================
    Texture2D tileset = LoadTexture(TILESET_PATH.c_str());
    Texture2D playerTexture = LoadTexture("assets/player.png");
    Texture2D bossTexture = LoadTexture("assets/mage_boss.png");
    Texture2D rabbitTexture = LoadTexture("assets/rabbit.png");

    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(playerTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(bossTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(rabbitTexture, TEXTURE_FILTER_POINT);

    // ==========================================
    // MAPA
    // ==========================================
    GameMap gameMap;
    gameMap.init(saves.getMap().getRenderData());

    std::cout << "[INFO] Mapa: " << gameMap.width << "x" << gameMap.height
        << " min=(" << gameMap.minX << "," << gameMap.minY << ")" << std::endl;

    // ==========================================
    // SPATIAL GRID
    // ==========================================
    SpatialGrid staticCollisionGrid;
    staticCollisionGrid.init(
        gameMap.width * TILE_SIZE_PX + 1000,
        gameMap.height * TILE_SIZE_PX + 1500,
        gameMap.minX - 500,
        gameMap.minY - 1000
    );

    // ==========================================
    // ENTITIES
    // ==========================================
    std::vector<std::unique_ptr<Entity>> activeEntities;
    Player* playerPtr = nullptr;

    // 1. Ściany
    std::vector<Wall> walls = saves.getMap().getAll();
    for (const auto& w : walls) {
        auto wallEnt = std::make_unique<Wall>(w);
        wallEnt->mType = WALL;
        staticCollisionGrid.insertStatic(wallEnt.get());
        activeEntities.push_back(std::move(wallEnt));
    }

    // 2. Gracz
    auto playerEntity = std::make_unique<Player>(464.5f, 442.0f);
    playerEntity->mTexture = playerTexture;
    playerPtr = playerEntity.get();
    activeEntities.push_back(std::move(playerEntity));

    // 3. Boss (wymuszony spawn)
    EntityS bossD;
    bossD.x = 658;
    bossD.y = 100;  // Zmień na widoczną pozycję!
    bossD.entityType = MAGE_BOSS;
    auto boss = EntitySToEntity(bossD, bossTexture, playerPtr);
    if (boss) {
        boss->mActive = true;
        boss->mHealth = 50;
        activeEntities.push_back(std::move(boss));
        std::cout << "[INFO] Boss zespawnowany na " << bossD.x << ", " << bossD.y << std::endl;
    }

    // 4. Królik
    EntityS rabbitD;
    rabbitD.x = 306;
    rabbitD.y = 322;
    rabbitD.entityType = RABBIT;
    auto rabbit = EntitySToEntity(rabbitD, rabbitTexture, playerPtr);
    if (rabbit) {
        rabbit->mActive = true;
        rabbit->mHealth = 5;
        activeEntities.push_back(std::move(rabbit));
    }

    // 5. Inne z zapisu
    auto rawEntities = saves.getEntities().getAll();
    for (const auto& eData : rawEntities) {
        if (eData.entityType == PLAYER || eData.entityType == MAGE_BOSS || eData.entityType == RABBIT) continue;
        auto newEnemy = EntitySToEntity(eData, tileset, playerPtr);
        if (newEnemy) activeEntities.push_back(std::move(newEnemy));
    }

    // ==========================================
    // KAMERA
    // ==========================================
    SmoothCamera smoothCam;
    if (playerPtr) smoothCam.currentPos = playerPtr->mPosition;

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = { (float)VIRTUAL_WIDTH / 2, (float)VIRTUAL_HEIGHT / 2 };

    RenderTexture2D target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    // ==========================================
    // POMOCNICZE
    // ==========================================
    std::vector<Entity*> nearbyObstacles;
    bool showDebug = false;

    // ==========================================
    // GŁÓWNA PĘTLA
    // ==========================================
    while (!WindowShouldClose() && !shouldQuit) {
        float dt = std::min(GetFrameTime(), 0.033f);

        // --- INPUT ---
        if (IsKeyPressed(KEY_ESCAPE)) {
            state = 0;
            break;
        }
        if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

        // --- UPDATE ENTITIES ---
        for (auto& ent : activeEntities) {
            if (!ent || !ent->mActive) continue;
            ent->update(dt);

            // Kolizje ze ścianami (dla każdego entity oprócz ścian)
            if (ent->mType != WALL) {
                nearbyObstacles.clear();
                staticCollisionGrid.getNearby(ent->mPosition.x, ent->mPosition.y, nearbyObstacles);
                for (Entity* obs : nearbyObstacles) {
                    if (CheckCollisionRecs(ent->getRect(), obs->getRect())) {
                        ent->onCollision(obs);
                    }
                }
            }
        }

        // --- WALKA (POPRAWIONA!) ---
        if (playerPtr && playerPtr->mActive) {
            for (auto& ent : activeEntities) {
                if (!ent || !ent->mActive) continue;
                if (ent->mType == WALL || ent.get() == playerPtr) continue;

                // ============================================
                // 1. GRACZ ATAKUJE WROGA (używamy mAttackArea!)
                // ============================================
                if (playerPtr->mIsAttacking) {
                    if (CheckCollisionRecs(playerPtr->mAttackArea, ent->getRect())) {
                        if (!playerPtr->hasHit(ent.get())) {
                            playerPtr->mHitEntities.push_back(ent.get());

                            int damage = playerPtr->getAttackDamage();
                            ent->mHealth -= damage;

                            std::cout << "[COMBAT] Trafiono! Damage: " << damage
                                << " | HP wroga: " << ent->mHealth << std::endl;

                            if (ent->mHealth <= 0) {
                                ent->mActive = false;
                                std::cout << "[COMBAT] Wrog pokonany!" << std::endl;
                            }
                        }
                    }
                }

                // ============================================
                // 2. WRÓG DOTYKA GRACZA (zadaje obrażenia)
                // ============================================
                if (CheckCollisionRecs(playerPtr->getRect(), ent->getRect())) {
                    if (playerPtr->mInvincibilityTimer <= 0) {
                        playerPtr->mHealth -= 1;
                        playerPtr->mInvincibilityTimer = 1.0f;

                        // Odrzut
                        float knockbackDir = (playerPtr->mPosition.x < ent->mPosition.x) ? -1.0f : 1.0f;
                        playerPtr->mVelocity.x = knockbackDir * 120.0f;
                        playerPtr->mVelocity.y = -80.0f;

                        std::cout << "[COMBAT] Gracz trafiony! HP: " << playerPtr->mHealth << std::endl;
                    }
                }
            }

            // Late update (animacje, wspinaczka)
            playerPtr->lateUpdate(dt);

            // Kamera
            smoothCam.update(playerPtr->mPosition, dt);
            camera.target = smoothCam.getPosition();
        }

        // --- RYSOWANIE ---
        BeginTextureMode(target);
        ClearBackground({ 40, 44, 52, 255 });
        BeginMode2D(camera);

        // Mapa (z cullingiem)
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

        // Entities (bez ścian!)
        for (auto& ent : activeEntities) {
            if (!ent || !ent->mActive || ent->mType == WALL) continue;
            ent->draw();

            // Pasek HP dla bossa
            if (ent->mType == MAGE_BOSS) {
                float barWidth = 32.0f;
                float barHeight = 3.0f;
                float barX = ent->mPosition.x - barWidth / 2 + 8;
                float barY = ent->mPosition.y - 12;
                float hpPercent = (float)ent->mHealth / 50.0f;

                DrawRectangle((int)barX, (int)barY, (int)barWidth, (int)barHeight, DARKGRAY);
                DrawRectangle((int)barX, (int)barY, (int)(barWidth * hpPercent), (int)barHeight, RED);
                DrawRectangleLinesEx({ barX, barY, barWidth, barHeight }, 1, BLACK);
            }

            // Pasek HP dla innych wrogów
            if (ent->mType == RABBIT) {
                float barWidth = 16.0f;
                float hpPercent = (float)ent->mHealth / 5.0f;
                DrawRectangle((int)ent->mPosition.x, (int)ent->mPosition.y - 6, (int)(barWidth * hpPercent), 2, RED);
            }
        }

        // Debug: hitbox ataku
        if (showDebug && playerPtr && playerPtr->mIsAttacking) {
            Color attackColor = YELLOW;
            if (playerPtr->mComboCount == 2) attackColor = ORANGE;
            DrawRectangleLinesEx(playerPtr->mAttackArea, 1, attackColor);
        }

        // Debug: hitboxy wrogów
        if (showDebug) {
            for (auto& ent : activeEntities) {
                if (!ent || !ent->mActive || ent->mType == WALL) continue;
                if (ent.get() != playerPtr) {
                    DrawRectangleLinesEx(ent->getRect(), 1, RED);
                }
            }
        }

        EndMode2D();
        EndTextureMode();

        // --- SKALOWANIE NA EKRAN ---
        BeginDrawing();
        ClearBackground(BLACK);

        float scale = std::min((float)windowWidth / VIRTUAL_WIDTH, (float)windowHeight / VIRTUAL_HEIGHT);
        float renderW = VIRTUAL_WIDTH * scale;
        float renderH = VIRTUAL_HEIGHT * scale;
        float offsetX = (windowWidth - renderW) / 2;
        float offsetY = (windowHeight - renderH) / 2;

        DrawTexturePro(
            target.texture,
            { 0, 0, (float)target.texture.width, -(float)target.texture.height },
            { offsetX, offsetY, renderW, renderH },
            { 0, 0 }, 0, WHITE
        );

        // --- UI ---
        if (playerPtr) {
            // HP Gracza
            DrawText(TextFormat("HP: %d/%d", playerPtr->mHealth, playerPtr->mMaxHealth), 20, 20, 24, RED);

            // Combo info
            if (playerPtr->mIsAttacking || playerPtr->mComboWindowOpen) {
                const char* comboText = "";
                Color comboColor = WHITE;
                switch (playerPtr->mComboCount) {
                case 0: comboText = "Combo: 1"; comboColor = WHITE; break;
                case 1: comboText = "Combo: 2"; comboColor = SKYBLUE; break;
                case 2: comboText = "FINISHER!"; comboColor = GOLD; break;
                }
                DrawText(comboText, 20, 50, 20, comboColor);
            }
        }

        // Debug info
        if (showDebug) {
            DrawText(TextFormat("FPS: %d", GetFPS()), windowWidth - 100, 20, 20, LIME);
            DrawText("[F3] Debug", windowWidth - 100, 45, 16, GRAY);

            if (playerPtr) {
                DrawText(TextFormat("Pos: %.0f, %.0f", playerPtr->mPosition.x, playerPtr->mPosition.y),
                    20, windowHeight - 50, 16, WHITE);
                DrawText(TextFormat("Vel: %.0f, %.0f", playerPtr->mVelocity.x, playerPtr->mVelocity.y),
                    20, windowHeight - 30, 16, WHITE);
            }
        }

        // Kontrolki
        DrawText("WASD/Arrows: Move | Space: Jump | C: Dash | Z: Attack | F3: Debug",
            20, windowHeight - 20, 12, DARKGRAY);

        EndDrawing();
    }

    // ==========================================
    // CLEANUP
    // ==========================================
    UnloadTexture(bossTexture);
    UnloadTexture(rabbitTexture);
    UnloadTexture(playerTexture);
    UnloadTexture(tileset);
    UnloadRenderTexture(target);
}