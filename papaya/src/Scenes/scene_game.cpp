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
#include "../Entities/Player/Player.h"
#include "../Entities/Environment/Wall.h"
#include "../Entities/Enemies/mage_boss.h"
#include "../Entities/Enemies/rabbit.h"

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

// =============================================================================
// GŁÓWNA FUNKCJA SCENY
// =============================================================================

void sceneGame(int windowWidth, int windowHeight, bool& shouldQuit, int& state) {
    SetTargetFPS(60);

    // =========================================================================
    // KROK 1: IMPORT DANYCH
    // =========================================================================
    {
        Saves tempSaves("assets/saves/main_save");

        if (tempSaves.getMap().getRenderData().empty()) {
            std::string jsonPath = "assets/editor";
            std::cout << "[SYSTEM] Wykryto pustą mapę. Próba importu z '" << jsonPath << "'..." << std::endl;

            try {
                tempSaves.loadFromEditorDir(jsonPath);
                std::cout << "[SYSTEM] Import zakończony. Zamykanie pliku w celu zapisania zmian..." << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "[ERROR] Błąd importu mapy: " << e.what() << std::endl;
            }
        }
    }

    // =========================================================================
    // KROK 2: WŁAŚCIWA GRA
    // =========================================================================
    Saves saves("assets/saves/main_save");

    // =========================================================================
    // TEKSTURY
    // =========================================================================
    Texture2D tileset = LoadTexture(TILESET_PATH.c_str());
    Texture2D playerTexture = LoadTexture("assets/player.png");
    Texture2D bossTexture = LoadTexture("assets/mage_boss.png");
    Texture2D rabbitTexture = LoadTexture("assets/rabbit.png");

    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(playerTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(bossTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(rabbitTexture, TEXTURE_FILTER_POINT);

    // =========================================================================
    // MAPA
    // =========================================================================
    GameMap gameMap;
    gameMap.init(saves.getMap().getRenderData());

    std::cout << "[INFO] Mapa: " << gameMap.width << "x" << gameMap.height
        << " min=(" << gameMap.minX << "," << gameMap.minY << ")" << std::endl;

    // =========================================================================
    // SPATIAL GRID
    // =========================================================================
    SpatialGrid staticCollisionGrid;
    staticCollisionGrid.init(
        gameMap.width * TILE_SIZE_PX + 1000,
        gameMap.height * TILE_SIZE_PX + 1500,
        gameMap.minX - 500,
        gameMap.minY - 1000
    );

    // =========================================================================
    // ENTITIES
    // =========================================================================
    std::vector<std::unique_ptr<Entity>> activeEntities;
    std::vector<Entity*> wallEntities;  // Wskaźniki do ścian dla bossa
    Player* playerPtr = nullptr;

    // Osobne listy dla wrogów (dla łatwiejszego dostępu)
    std::vector<MageBoss*> bosses;
    std::vector<RabbitEnemy*> rabbits;

    // --- 1. Ściany z kolizji mapy ---
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

    // --- 2. Gracz ---
    {
        auto playerEntity = std::make_unique<Player>(464.5f, 442.0f);
        playerEntity->mTexture = playerTexture;
        playerPtr = playerEntity.get();
        activeEntities.push_back(std::move(playerEntity));
    }

    // --- 3. Wrogowie z pliku ---
    {
        auto rawEntities = saves.getEntities().getAll();
        std::cout << "[INFO] Wczytano " << rawEntities.size() << " encji z pliku." << std::endl;

        for (const auto& eData : rawEntities) {
            if (eData.entityType == PLAYER) continue;

            if (eData.entityType == MAGE_BOSS) {
                auto boss = std::make_unique<MageBoss>(
                    eData.x, eData.y,
                    bossTexture,
                    playerPtr
                );
                if (eData.health > 0) {
                    boss->mHealth = eData.health;
                    boss->mMaxHealth = eData.health;
                }
                bosses.push_back(boss.get());
                activeEntities.push_back(std::move(boss));
            }
            else if (eData.entityType == RABBIT) {
                auto rabbit = std::make_unique<RabbitEnemy>(
                    eData.x, eData.y,
                    rabbitTexture,
                    playerPtr
                );
                if (eData.health > 0) rabbit->mHealth = eData.health;
                rabbits.push_back(rabbit.get());
                activeEntities.push_back(std::move(rabbit));
            }
        }
    }

    // --- 4. Testowi wrogowie (jeśli brak w pliku) ---
    if (bosses.empty() && rabbits.empty()) {
        std::cout << "[INFO] Brak wrogów w pliku - tworzę testowych." << std::endl;

        // Testowy boss
        auto testBoss = std::make_unique<MageBoss>(
            666.0f, -376.0f,
            bossTexture,
            playerPtr
        );

        bosses.push_back(testBoss.get());
        activeEntities.push_back(std::move(testBoss));

        // Testowe króliki
        auto testRabbit = std::make_unique<RabbitEnemy>(
            270.0f, 322.0f,
            rabbitTexture,
            playerPtr
        );
        rabbits.push_back(testRabbit.get());
        activeEntities.push_back(std::move(testRabbit));
    }

    std::cout << "[INFO] Bosses: " << bosses.size() << ", Rabbits: " << rabbits.size() << std::endl;

    // --- WAŻNE: Aktualizacja granic mapy (ARENA 550,-448 do 782,-348) ---
    {
        // Obliczamy wymiary na podstawie podanych punktów:
        // X: od 550 do 782 -> Szerokość = 782 - 550 = 232
        // Y: od -448 do -348 -> Wysokość = |-348 - (-448)| = 100
        // Top-Left: (550, -448)

        for (MageBoss* boss : bosses) {
            boss->setArenaBounds(
                550.0f,     // X
                -448.0f,    // Y (górna krawędź, mniejsza wartość = wyżej w Y-down niekoniecznie, ale tu są ujemne)
                // Zakładamy, że -448 to góra, a -348 to dół.
                232.0f,     // Szerokość
                100.0f      // Wysokość
            );
            std::cout << "[INFO] Boss Arena set to: 550, -448 (232x100)" << std::endl;
        }
    }

    // =========================================================================
    // KAMERA
    // =========================================================================
    SmoothCamera smoothCam;
    if (playerPtr) smoothCam.currentPos = playerPtr->mPosition;

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = { (float)VIRTUAL_WIDTH / 2, (float)VIRTUAL_HEIGHT / 2 };

    RenderTexture2D target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    // =========================================================================
    // POMOCNICZE
    // =========================================================================
    std::vector<Entity*> nearbyObstacles;
    bool showDebug = false;

    // =========================================================================
    // GŁÓWNA PĘTLA
    // =========================================================================
    while (!WindowShouldClose() && !shouldQuit) {
        float dt = std::min(GetFrameTime(), 0.033f);

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = 0;
            break;
        }
        if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;

        // =====================================================================
        // UPDATE
        // =====================================================================

        // --- 1. Update gracza ---
        if (playerPtr && playerPtr->mActive) {
            playerPtr->update(dt);
        }

        // --- 2. Update bossów (specjalna logika z wallEntities) ---
        for (MageBoss* boss : bosses) {
            if (!boss || !boss->mActive) continue;
            boss->updateBossLogic(dt, wallEntities);
        }

        // --- 3. Update królików ---
        for (RabbitEnemy* rabbit : rabbits) {
            if (!rabbit || !rabbit->mActive) continue;
            rabbit->update(dt);
        }

        // --- 4. Kolizje ze ścianami ---
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

        // --- 5. Walka gracza vs wrogowie ---
        if (playerPtr && playerPtr->mActive) {

            // === BOSS ===
            for (MageBoss* boss : bosses) {
                if (!boss || !boss->mActive) continue;

                // Atak gracza na bossa
                if (playerPtr->mIsAttacking) {
                    if (CheckCollisionRecs(playerPtr->mAttackArea, boss->getRect())) {
                        if (!playerPtr->hasHit(boss)) {
                            playerPtr->mHitEntities.push_back(boss);
                            int damage = playerPtr->getAttackDamage();
                            boss->takeDamage(damage);
                        }
                    }
                }

                // Fireballe bossa vs gracz
                for (auto& fb : boss->mFireballs) {
                    if (!fb.active) continue;

                    if (playerPtr->mInvincibilityTimer <= 0) {
                        Rectangle playerRect = playerPtr->getRect();
                        if (CheckCollisionCircleRec(fb.position, fb.radius, playerRect)) {
                            playerPtr->mHealth -= 1;
                            playerPtr->mInvincibilityTimer = 1.0f;

                            float knockDir = (playerPtr->mPosition.x < fb.position.x) ? -1.0f : 1.0f;
                            playerPtr->mVelocity.x = knockDir * 150.0f;
                            playerPtr->mVelocity.y = -100.0f;

                            fb.active = false;
                        }
                    }
                }

                // Laser bossa vs gracz
                if (boss->mState == MageBoss::LASER_FIRE) {
                    if (playerPtr->mInvincibilityTimer <= 0) {
                        if (boss->checkLaserCollision(playerPtr)) {
                            playerPtr->mHealth -= 2;
                            playerPtr->mInvincibilityTimer = 1.5f;

                            float knockDir = (playerPtr->mPosition.x < boss->mPosition.x) ? -1.0f : 1.0f;
                            playerPtr->mVelocity.x = knockDir * 200.0f;
                            playerPtr->mVelocity.y = -150.0f;
                        }
                    }
                }

                // AOE bossa vs gracz
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

                // Dotyk bossa (gdy NIE VULNERABLE)
                if (boss->mState != MageBoss::VULNERABLE &&
                    boss->mState != MageBoss::DYING &&
                    boss->mState != MageBoss::INACTIVE) {
                    if (CheckCollisionRecs(playerPtr->getRect(), boss->getRect())) {
                        if (playerPtr->mInvincibilityTimer <= 0) {
                            playerPtr->mHealth -= 1;
                            playerPtr->mInvincibilityTimer = 1.0f;

                            float knockDir = (playerPtr->mPosition.x < boss->mPosition.x) ? -1.0f : 1.0f;
                            playerPtr->mVelocity.x = knockDir * 120.0f;
                            playerPtr->mVelocity.y = -80.0f;
                        }
                    }
                }
            }

            // === KRÓLIKI ===
            for (RabbitEnemy* rabbit : rabbits) {
                if (!rabbit || !rabbit->mActive || rabbit->mIsDead) continue;

                // Atak gracza na królika
                if (playerPtr->mIsAttacking) {
                    if (CheckCollisionRecs(playerPtr->mAttackArea, rabbit->getRect())) {
                        if (!playerPtr->hasHit(rabbit)) {
                            playerPtr->mHitEntities.push_back(rabbit);
                            int damage = playerPtr->getAttackDamage();
                            rabbit->takeDamage(damage);

                            if (rabbit->mIsDead) {
                                rabbit->mActive = false;
                            }
                        }
                    }
                }

                // Dotyk królika vs gracz
                if (!rabbit->mIsDead) {
                    if (CheckCollisionRecs(playerPtr->getRect(), rabbit->getRect())) {
                        if (playerPtr->mInvincibilityTimer <= 0) {
                            // Monster form zadaje więcej obrażeń
                            int damage = 1;
                            if (rabbit->mState >= RabbitEnemy::MONSTER_CHARGE &&
                                rabbit->mState <= RabbitEnemy::MONSTER_TURN) {
                                damage = 2;
                            }

                            playerPtr->mHealth -= damage;
                            playerPtr->mInvincibilityTimer = 1.0f;

                            float knockDir = (playerPtr->mPosition.x < rabbit->mPosition.x) ? -1.0f : 1.0f;
                            playerPtr->mVelocity.x = knockDir * 100.0f;
                            playerPtr->mVelocity.y = -60.0f;
                        }
                    }
                }
            }
        }

        // --- 6. Late Update gracza ---
        if (playerPtr && playerPtr->mActive) {
            playerPtr->lateUpdate(dt);
            smoothCam.update(playerPtr->mPosition, dt);
            camera.target = smoothCam.getPosition();
        }

        // =====================================================================
        // DRAW
        // =====================================================================

        BeginTextureMode(target);
        ClearBackground({ 40, 44, 52, 255 });
        BeginMode2D(camera);

        // --- Mapa ---
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

        // --- Króliki ---
        for (RabbitEnemy* rabbit : rabbits) {
            if (rabbit && rabbit->mActive) {
                rabbit->draw();
            }
        }

        // --- Bossy ---
        for (MageBoss* boss : bosses) {
            if (boss && boss->mActive) {
                boss->draw();
            }
        }

        // --- Gracz ---
        if (playerPtr && playerPtr->mActive) {
            playerPtr->draw();
        }

        // --- Debug ---
        if (showDebug) {
            // Koordynaty gracza
            if (playerPtr) {
                DrawRectangleLinesEx(playerPtr->getRect(), 1, GREEN);
                if (playerPtr->mIsAttacking) {
                    DrawRectangleLinesEx(playerPtr->mAttackArea, 1, YELLOW);
                }
                DrawText(TextFormat("%.0f,%.0f", playerPtr->mPosition.x, playerPtr->mPosition.y),
                    (int)playerPtr->mPosition.x - 15, (int)playerPtr->mPosition.y - 20, 5, LIME);
            }

            // Koordynaty kursora myszy w świecie
            Vector2 mouseScreen = GetMousePosition();
            float scale = std::min((float)windowWidth / VIRTUAL_WIDTH, (float)windowHeight / VIRTUAL_HEIGHT);
            float offsetX = (windowWidth - VIRTUAL_WIDTH * scale) / 2;
            float offsetY = (windowHeight - VIRTUAL_HEIGHT * scale) / 2;

            Vector2 mouseWorld = {
                (mouseScreen.x - offsetX) / scale + camera.target.x - VIRTUAL_WIDTH / 2.0f,
                (mouseScreen.y - offsetY) / scale + camera.target.y - VIRTUAL_HEIGHT / 2.0f
            };

            // Krzyżyk na kursorze
            DrawLine((int)mouseWorld.x - 6, (int)mouseWorld.y,
                (int)mouseWorld.x + 6, (int)mouseWorld.y, YELLOW);
            DrawLine((int)mouseWorld.x, (int)mouseWorld.y - 6,
                (int)mouseWorld.x, (int)mouseWorld.y + 6, YELLOW);
            DrawText(TextFormat("%.0f,%.0f", mouseWorld.x, mouseWorld.y),
                (int)mouseWorld.x + 8, (int)mouseWorld.y - 3, 5, YELLOW);

            // Stany wrogów
            for (RabbitEnemy* rabbit : rabbits) {
                if (rabbit && rabbit->mActive) {
                    DrawRectangleLinesEx(rabbit->getRect(), 1, ORANGE);
                    const char* stateText = "?";
                    switch (rabbit->mState) {
                    case RabbitEnemy::RABBIT_IDLE: stateText = "IDLE"; break;
                    case RabbitEnemy::RABBIT_HOP: stateText = "HOP"; break;
                    case RabbitEnemy::MORPHING: stateText = "MORPH"; break;
                    case RabbitEnemy::MONSTER_CHARGE: stateText = "CHARGE"; break;
                    case RabbitEnemy::MONSTER_RUN: stateText = "RUN"; break;
                    case RabbitEnemy::MONSTER_SLIDE: stateText = "SLIDE"; break;
                    case RabbitEnemy::MONSTER_TURN: stateText = "TURN"; break;
                    case RabbitEnemy::UNMORPHING: stateText = "UNMORPH"; break;
                    case RabbitEnemy::MONSTER_DYING: stateText = "DYING"; break;
                    }
                    DrawText(stateText, (int)rabbit->mPosition.x, (int)rabbit->mPosition.y - 10, 6, WHITE);
                }
            }

            for (MageBoss* boss : bosses) {
                if (boss && boss->mActive) {
                    const char* stateText = "?";
                    switch (boss->mState) {
                    case MageBoss::INACTIVE: stateText = "INACTIVE"; break;
                    case MageBoss::APPEARING: stateText = "APPEAR"; break;
                    case MageBoss::IDLE: stateText = "IDLE"; break;
                    case MageBoss::CAST_FIREBALL: stateText = "FIREBALL"; break;
                    case MageBoss::LASER_CHARGE: stateText = "LASER_CHG"; break;
                    case MageBoss::LASER_FIRE: stateText = "LASER!!!"; break;
                    case MageBoss::VULNERABLE: stateText = "VULN"; break;
                    case MageBoss::TELEPORT_OUT: stateText = "TP_OUT"; break;
                    case MageBoss::TELEPORT_IN: stateText = "TP_IN"; break;
                    case MageBoss::AOE_ATTACK: stateText = "AOE"; break;
                    case MageBoss::DYING: stateText = "DYING"; break;
                    }
                    DrawText(stateText, (int)boss->mPosition.x, (int)boss->mPosition.y - 20, 8,
                        boss->mIsEnraged ? RED : WHITE);

                    // Rysuj granice areny w debugu
                    DrawRectangleLines(
                        (int)boss->mArenaBounds.x,
                        (int)boss->mArenaBounds.y,
                        (int)boss->mArenaBounds.width,
                        (int)boss->mArenaBounds.height,
                        Fade(YELLOW, 0.3f)
                    );
                }
            }
        }

        EndMode2D();
        EndTextureMode();

        // --- Render do okna ---
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

        // --- UI ---
        if (playerPtr) {
            DrawText(TextFormat("HP: %d", playerPtr->mHealth), 20, 20, 24, RED);
        }

        // Debug UI
        if (showDebug) {
            // Recalculate mouse world position for UI
            Vector2 mouseScreen = GetMousePosition();
            float offsetX = (windowWidth - VIRTUAL_WIDTH * scale) / 2;
            float offsetY = (windowHeight - VIRTUAL_HEIGHT * scale) / 2;
            Vector2 mouseWorld = {
                (mouseScreen.x - offsetX) / scale + camera.target.x - VIRTUAL_WIDTH / 2.0f,
                (mouseScreen.y - offsetY) / scale + camera.target.y - VIRTUAL_HEIGHT / 2.0f
            };

            DrawText(TextFormat("Player: %.0f, %.0f", playerPtr->mPosition.x, playerPtr->mPosition.y),
                20, 50, 16, LIME);
            DrawText(TextFormat("Mouse:  %.0f, %.0f", mouseWorld.x, mouseWorld.y),
                20, 70, 16, YELLOW);
            DrawText(TextFormat("Camera: %.0f, %.0f", camera.target.x, camera.target.y),
                20, 90, 16, SKYBLUE);
            DrawText(TextFormat("Enemies: %d boss, %d rabbit", (int)bosses.size(), (int)rabbits.size()),
                20, 110, 16, WHITE);

            DrawText("[F3] Debug Mode", 20, windowHeight - 30, 14, GRAY);
            DrawFPS(windowWidth - 100, 20);
        }

        // Pasek HP bossa na górze ekranu
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

                break;  // Tylko jeden boss na ekranie
            }
        }

        if (showDebug) {
            DrawText("F3: Debug", 20, windowHeight - 30, 16, GRAY);
            DrawFPS(windowWidth - 100, 20);
        }

        EndDrawing();
    }

    // =========================================================================
    // CLEANUP
    // =========================================================================
    UnloadTexture(bossTexture);
    UnloadTexture(rabbitTexture);
    UnloadTexture(playerTexture);
    UnloadTexture(tileset);
    UnloadRenderTexture(target);
}