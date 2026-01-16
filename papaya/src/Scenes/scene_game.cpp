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
 #include "../Entities/mage_boss.h" // Odkomentuj jeśli masz ten plik nagłówkowy
 #include "../Entities/rabbit.h"    // Odkomentuj jeśli masz ten plik nagłówkowy

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
    SetTargetFPS(60);

    // ==========================================
    // KROK 1: IMPORT DANYCH (W OSOBNYM ZAKRESIE)
    // ==========================================
    {
        // Tworzymy obiekt Saves tylko na chwilę, żeby sprawdzić/zaimportować mapę
        Saves tempSaves("assets/saves/main_save");

        if (tempSaves.getMap().getRenderData().empty()) {
            std::string jsonPath = "assets/editor"; // Twoja ścieżka do JSONów
            std::cout << "[SYSTEM] Wykryto pustą mapę. Próba importu z '" << jsonPath << "'..." << std::endl;

            try {
                tempSaves.loadFromEditorDir(jsonPath);
                std::cout << "[SYSTEM] Import zakończony. Zamykanie pliku w celu zapisania zmian..." << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "[ERROR] Błąd importu mapy: " << e.what() << std::endl;
            }
        }
        // TUTAJ tempSaves jest niszczone.
        // Destruktor zamyka pliki i wymusza zapis na dysk.
    }

    // ==========================================
    // KROK 2: WŁAŚCIWA GRA
    // ==========================================
    // Otwieramy pliki ponownie - teraz na pewno mają dane!
    Saves saves("assets/saves/main_save");

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
    // Teraz getRenderData() pobierze świeże dane z pliku
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

    // 1. Ściany z kolizji mapy
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
            activeEntities.push_back(std::move(wallEnt));
        }
    }

    // 2. Gracz
    {
        auto playerEntity = std::make_unique<Player>(464.5f, 442.0f);
        playerEntity->mTexture = playerTexture;
        playerPtr = playerEntity.get();
        activeEntities.push_back(std::move(playerEntity));
    }

    // 3. Wrogowie z pliku
    {
        auto rawEntities = saves.getEntities().getAll();
        std::cout << "[INFO] Wczytano " << rawEntities.size() << " encji z pliku." << std::endl;

        for (const auto& eData : rawEntities) {
            if (eData.entityType == PLAYER) continue;

            Texture2D* textureToUse = &tileset;
            if (eData.entityType == MAGE_BOSS) textureToUse = &bossTexture;
            else if (eData.entityType == RABBIT) textureToUse = &rabbitTexture;

            auto newEnemy = EntitySToEntity(eData, *textureToUse, playerPtr);
            if (newEnemy) {
                newEnemy->mHealth = (eData.health > 0) ? eData.health : 10;
                newEnemy->mActive = true;
                activeEntities.push_back(std::move(newEnemy));
            }
        }
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

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = 0;
            break;
        }
        if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

        // ==========================================
        // UPDATE
        // ==========================================

        // 1. Update wszystkich entity (fizyka, input, AI)
        for (auto& ent : activeEntities) {
            if (!ent || !ent->mActive) continue;
            ent->update(dt);
        }

        // 2. Kolizje ze ścianami (z mapy)
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

        // 3. Walka - atak gracza na wrogów
        if (playerPtr && playerPtr->mActive) {
            for (auto& ent : activeEntities) {
                if (!ent || !ent->mActive) continue;
                if (ent->mType == WALL || ent.get() == playerPtr) continue;

                // Atak gracza
                if (playerPtr->mIsAttacking) {
                    if (CheckCollisionRecs(playerPtr->mAttackArea, ent->getRect())) {
                        if (!playerPtr->hasHit(ent.get())) {
                            playerPtr->mHitEntities.push_back(ent.get());
                            int damage = playerPtr->getAttackDamage();
                            ent->mHealth -= damage;
                            if (ent->mHealth <= 0) ent->mActive = false;
                        }
                    }
                }

                // Kolizja z graczem (damage)
                if (CheckCollisionRecs(playerPtr->getRect(), ent->getRect())) {
                    if (playerPtr->mInvincibilityTimer <= 0) {
                        playerPtr->mHealth -= 1;
                        playerPtr->mInvincibilityTimer = 1.0f;
                        float knockbackDir = (playerPtr->mPosition.x < ent->mPosition.x) ? -1.0f : 1.0f;
                        playerPtr->mVelocity.x = knockbackDir * 120.0f;
                        playerPtr->mVelocity.y = -80.0f;
                    }
                }
            }
        }

        // 4. Late Update (animacje + wspinaczka PO kolizjach)
        if (playerPtr && playerPtr->mActive) {
            playerPtr->lateUpdate(dt);
            smoothCam.update(playerPtr->mPosition, dt);
            camera.target = smoothCam.getPosition();
        }

        // ==========================================
        // DRAW
        // ==========================================

        BeginTextureMode(target);
        ClearBackground({ 40, 44, 52, 255 });
        BeginMode2D(camera);

        // Mapa
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

        // Entities
        for (auto& ent : activeEntities) {
            if (!ent || !ent->mActive || ent->mType == WALL) continue;
            ent->draw();

            if (ent->mType == MAGE_BOSS) {
                float barW = 32.0f;
                DrawRectangle((int)(ent->mPosition.x - barW / 2 + 8), (int)(ent->mPosition.y - 12), (int)barW, 3, DARKGRAY);
                DrawRectangle((int)(ent->mPosition.x - barW / 2 + 8), (int)(ent->mPosition.y - 12), (int)(barW * ((float)ent->mHealth / 50.0f)), 3, RED);
            }
        }

        if (showDebug && playerPtr) {
            DrawRectangleLinesEx(playerPtr->getRect(), 1, GREEN);
            if (playerPtr->mIsAttacking) DrawRectangleLinesEx(playerPtr->mAttackArea, 1, YELLOW);
        }

        EndMode2D();
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);
        float scale = std::min((float)windowWidth / VIRTUAL_WIDTH, (float)windowHeight / VIRTUAL_HEIGHT);
        DrawTexturePro(target.texture, { 0, 0, (float)target.texture.width, -(float)target.texture.height },
            { (windowWidth - VIRTUAL_WIDTH * scale) / 2, (windowHeight - VIRTUAL_HEIGHT * scale) / 2, VIRTUAL_WIDTH * scale, VIRTUAL_HEIGHT * scale },
            { 0, 0 }, 0, WHITE);

        if (playerPtr) DrawText(TextFormat("HP: %d", playerPtr->mHealth), 20, 20, 24, RED);
        EndDrawing();
    }

    UnloadTexture(bossTexture);
    UnloadTexture(rabbitTexture);
    UnloadTexture(playerTexture);
    UnloadTexture(tileset);
    UnloadRenderTexture(target);
}