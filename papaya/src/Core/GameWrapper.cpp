#include "GameWrapper.hpp"
#include "Physics.hpp"

#include "../Saves/saves.hpp"
#include "../Entities/Entity.h"
#include "../Entities/Player/Player.h"
#include "../Entities/Environment/Wall.h"
#include "../Entities/Enemies/mage_boss.h"
#include "../Entities/Enemies/rabbit.h"

#include <iostream>
#include <algorithm>

using namespace GameConstants;

void GameMap::init(const std::vector<RenderTile>& rawTiles) {
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
        if (gx >= 0 && gx < width && gy >= 0 && gy < height) {
            tiles[gy * width + gx] = static_cast<int>(t.textureID);
        }
    }
}

int GameMap::getTileAt(int gridX, int gridY) const {
    if (gridX < 0 || gridX >= width || gridY < 0 || gridY >= height) return 0;
    return tiles[gridY * width + gridX];
}

void SpatialGrid::init(int mapW, int mapH, int minX_, int minY_) {
    offsetX = minX_;
    offsetY = minY_;
    width = (mapW / cellSize) + 2;
    height = (mapH / cellSize) + 2;
    cells.assign(width * height, {});
}

void SpatialGrid::clear() {
    for (auto& cell : cells) {
        cell.clear();
    }
}

void SpatialGrid::insertStatic(Entity* e) {
    int cx = std::clamp(((int)e->mPosition.x - offsetX) / cellSize, 0, width - 1);
    int cy = std::clamp(((int)e->mPosition.y - offsetY) / cellSize, 0, height - 1);
    cells[cy * width + cx].push_back(e);
}

void SpatialGrid::getNearby(float x, float y, std::vector<Entity*>& outResult) const {
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

void SmoothCamera::update(Vector2 target, float dt) {
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

Vector2 SmoothCamera::getPosition() const {
    return { std::floor(currentPos.x), std::floor(currentPos.y) };
}

void SmoothCamera::setPosition(Vector2 pos) {
    currentPos = pos;
}

void CheatSystem::showMessage(const std::string& msg) {
    message = msg;
    messageTimer = 2.0f;
    std::cout << "[CHEAT] " << msg << std::endl;
}

void CheatSystem::update(float dt) {
    if (messageTimer > 0) {
        messageTimer -= dt;
    }
}

bool CheatSystem::hasActiveMessage() const {
    return messageTimer > 0;
}

void WeaponUIState::triggerChange() {
    changeTimer = CHANGE_ANIM_TIME;
}

void WeaponUIState::update(float dt) {
    if (changeTimer > 0) {
        changeTimer -= dt;
    }
}

bool WeaponUIState::isAnimating() const {
    return changeTimer > 0;
}

GameWrapper::GameWrapper(bool& shouldQuit, int& state)
    : mShouldQuit(shouldQuit)
    , mState(state)
{
    std::cout << "[GameWrapper] Inicjalizacja..." << std::endl;

    {
        Saves tempSaves(SAVE_PATH);
        if (tempSaves.getMap().getRenderData().empty()) {
            std::cout << "[SYSTEM] Wykryto pust¹ mapê. Próba importu z '" << EDITOR_PATH << "'..." << std::endl;
            try {
                tempSaves.loadFromEditorDir(EDITOR_PATH);
                std::cout << "[SYSTEM] Import zakoñczony." << std::endl;
            }
            catch (const std::exception& e) {
                std::cout << "[ERROR] B³¹d importu mapy: " << e.what() << std::endl;
            }
        }
    }

    loadTextures();
    loadMap();
    loadEntities();

    mCamera = { 0 };
    mCamera.zoom = 1.0f;
    mCamera.offset = { (float)VIRTUAL_WIDTH / 2, (float)VIRTUAL_HEIGHT / 2 };

    mRenderTarget = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(mRenderTarget.texture, TEXTURE_FILTER_POINT);

    if (mPlayerPtr) {
        mSmoothCamera.setPosition(mPlayerPtr->mPosition);
    }

    mInitialized = true;
    std::cout << "[GameWrapper] Inicjalizacja zakoñczona!" << std::endl;
}

GameWrapper::~GameWrapper() {
    std::cout << "[GameWrapper] Sprz¹tanie..." << std::endl;
    unloadTextures();
    UnloadRenderTexture(mRenderTarget);
}

void GameWrapper::loadTextures() {
    mTileset = LoadTexture(TILESET_PATH.c_str());
    mPlayerTexture = LoadTexture(PLAYER_TEXTURE_PATH.c_str());
    mBossTexture = LoadTexture(BOSS_TEXTURE_PATH.c_str());
    mRabbitTexture = LoadTexture(RABBIT_TEXTURE_PATH.c_str());
    mBackground = LoadTexture(BACKGROUND_PATH.c_str());

    SetTextureFilter(mTileset, TEXTURE_FILTER_POINT);
    SetTextureFilter(mPlayerTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(mBossTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(mRabbitTexture, TEXTURE_FILTER_POINT);
    SetTextureFilter(mBackground, TEXTURE_FILTER_BILINEAR);
}

void GameWrapper::unloadTextures() {
    UnloadTexture(mTileset);
    UnloadTexture(mPlayerTexture);
    UnloadTexture(mBossTexture);
    UnloadTexture(mRabbitTexture);
    UnloadTexture(mBackground);
}

void GameWrapper::loadMap() {
    Saves saves(SAVE_PATH);

    mGameMap.init(saves.getMap().getRenderData());
    std::cout << "[INFO] Mapa: " << mGameMap.width << "x" << mGameMap.height
        << " min=(" << mGameMap.minX << "," << mGameMap.minY << ")" << std::endl;

    mCollisionGrid.init(
        mGameMap.width * TILE_SIZE_PX + 1000,
        mGameMap.height * TILE_SIZE_PX + 1500,
        mGameMap.minX - 500,
        mGameMap.minY - 1000
    );

    std::vector<CollisionRect> collisions = saves.getMap().getCollisions();
    std::cout << "[INFO] Za³adowano " << collisions.size() << " kolizji." << std::endl;

    for (const auto& col : collisions) {
        auto wallEnt = std::make_unique<Wall>(
            static_cast<float>(col.x),
            static_cast<float>(col.y),
            static_cast<float>(col.w),
            static_cast<float>(col.h)
        );
        mCollisionGrid.insertStatic(wallEnt.get());
        mWallEntities.push_back(wallEnt.get());
        mActiveEntities.push_back(std::move(wallEnt));
    }

    mLiquidRects = saves.getMap().getDamagingZones();
    std::cout << "[INFO] Za³adowano " << mLiquidRects.size() << " stref cieczy." << std::endl;
}

void GameWrapper::loadEntities() {
    Saves saves(SAVE_PATH);

    {
        auto playerEntity = std::make_unique<Player>(mPlayerSpawnPoint.x, mPlayerSpawnPoint.y);
        playerEntity->mTexture = mPlayerTexture;
        mPlayerPtr = playerEntity.get();
        mActiveEntities.push_back(std::move(playerEntity));
    }

    auto rawEntities = saves.getEntities().getAll();
    std::cout << "[DEBUG] Liczba encji w pliku binarnym: " << rawEntities.size() << std::endl;

    for (const auto& eData : rawEntities) {
        std::cout << "[DEBUG] Znaleziono encjê -> ID: " << (int)eData.entityType
            << " X: " << eData.x << " Y: " << eData.y
            << " Health: " << eData.health << std::endl;

        if (eData.entityType == PLAYER) continue;

        float pixelX = static_cast<float>(eData.x * TILE_SIZE_PX);
        float pixelY = static_cast<float>(eData.y * TILE_SIZE_PX);

        if (eData.entityType == MAGE_BOSS) {
            auto boss = std::make_unique<MageBoss>(pixelX, pixelY, mBossTexture, mPlayerPtr);
            Rectangle arena = {
                boss->mPosition.x - 116.0f,
                boss->mPosition.y - 50.0f,
                232.0f,
                100.0f
            };
            boss->setArenaBounds(arena.x, arena.y, arena.width, arena.height);

            std::cout << "[DEBUG] Boss created with HP: " << boss->mHealth
                << "/" << boss->mMaxHealth << std::endl;

            mBossSpawnPoints.push_back({ {pixelX, pixelY}, arena, boss->mMaxHealth });
            mBosses.push_back(boss.get());
            mActiveEntities.push_back(std::move(boss));
        }
        else if (eData.entityType == RABBIT) {
            auto rabbit = std::make_unique<RabbitEnemy>(pixelX, pixelY, mRabbitTexture, mPlayerPtr);
            if (eData.health > 0) rabbit->mHealth = eData.health;
            mRabbits.push_back(rabbit.get());
            mActiveEntities.push_back(std::move(rabbit));
        }
    }

    if (mBosses.empty() && mRabbits.empty()) {
        createTestEnemies();
    }

    std::cout << "[INFO] Bosses: " << mBosses.size() << ", Rabbits: " << mRabbits.size() << std::endl;
}

void GameWrapper::createTestEnemies() {
    std::cout << "[INFO] Brak wrogów w pliku - tworzê testowych." << std::endl;

    auto testBoss = std::make_unique<MageBoss>(666.0f, -376.0f, mBossTexture, mPlayerPtr);
    Rectangle arena = { 550.0f, -448.0f, 232.0f, 100.0f };
    testBoss->setArenaBounds(arena.x, arena.y, arena.width, arena.height);

    std::cout << "[DEBUG] Test boss created with HP: " << testBoss->mHealth
        << "/" << testBoss->mMaxHealth << std::endl;

    mBossSpawnPoints.push_back({ {666.0f, -376.0f}, arena, testBoss->mMaxHealth });
    mBosses.push_back(testBoss.get());
    mActiveEntities.push_back(std::move(testBoss));

    auto testRabbit = std::make_unique<RabbitEnemy>(270.0f, 322.0f, mRabbitTexture, mPlayerPtr);
    mRabbits.push_back(testRabbit.get());
    mActiveEntities.push_back(std::move(testRabbit));
}

void GameWrapper::runFrame(int windowWidth, int windowHeight) {
    float dt = std::min(GetFrameTime(), 0.033f);

    handleInput(dt);
    updateEntities(dt);
    updateCollisions();
    updateLiquids(dt);
    updateCombat();
    updateCamera(dt);

    mCheats.update(dt);
    mWeaponUI.update(dt);

    drawWorld();
    drawUI(windowWidth, windowHeight);
}

void GameWrapper::handleInput(float dt) {
    if (IsKeyPressed(KEY_ESCAPE)) {
        mState = 0;
        return;
    }

    if (IsKeyPressed(KEY_F3)) mShowDebug = !mShowDebug;

    if (mPlayerPtr) {
        bool weaponChanged = false;

        if (IsKeyPressed(KEY_ONE)) {
            mPlayerPtr->setWeapon(WeaponType::SWORD_DEFAULT);
            weaponChanged = true;
        }
        else if (IsKeyPressed(KEY_TWO)) {
            mPlayerPtr->setWeapon(WeaponType::DAGGER_SWIFT);
            weaponChanged = true;
        }
        else if (IsKeyPressed(KEY_THREE)) {
            mPlayerPtr->setWeapon(WeaponType::AXE_HEAVY);
            weaponChanged = true;
        }
        else if (IsKeyPressed(KEY_FOUR)) {
            mPlayerPtr->setWeapon(WeaponType::SPEAR_LONG);
            weaponChanged = true;
        }
        else if (IsKeyPressed(KEY_FIVE)) {
            mPlayerPtr->setWeapon(WeaponType::KATANA_BLOOD);
            weaponChanged = true;
        }

        if (weaponChanged) {
            mWeaponUI.triggerChange();
        }
    }

    if (IsKeyPressed(KEY_G)) {
        mCheats.godMode = !mCheats.godMode;
        mCheats.showMessage(mCheats.godMode ? "GODMODE: ON" : "GODMODE: OFF");
    }

    if (IsKeyPressed(KEY_N)) {
        mCheats.noclip = !mCheats.noclip;
        // --- POPRAWKA: Przekazanie noclip do gracza ---
        if (mPlayerPtr) mPlayerPtr->mNoclip = mCheats.noclip;
        // ----------------------------------------------
        mCheats.showMessage(mCheats.noclip ? "NOCLIP: ON" : "NOCLIP: OFF");
    }

    if (IsKeyPressed(KEY_R)) {
        respawnAllBosses();
    }

    if (IsKeyPressed(KEY_T)) {
        if (mPlayerPtr && !mBosses.empty()) {
            MageBoss* nearestBoss = mBosses[0];
            mPlayerPtr->mPosition = {
                nearestBoss->mPosition.x - 60.0f,
                nearestBoss->mPosition.y
            };
            mPlayerPtr->mVelocity = { 0, 0 };
            mSmoothCamera.setPosition(mPlayerPtr->mPosition);
            mCheats.showMessage("TELEPORTED TO BOSS!");
        }
    }

    if (IsKeyPressed(KEY_H)) {
        if (mPlayerPtr) {
            mPlayerPtr->mHealth = mPlayerPtr->mMaxHealth;
            mCheats.showMessage("HEALTH RESTORED!");
        }
    }

    if (IsKeyPressed(KEY_K)) {
        for (MageBoss* boss : mBosses) {
            if (boss && boss->mActive) {
                boss->takeDamage(9999);
            }
        }
        mCheats.showMessage("BOSSES KILLED!");
    }

    if (IsKeyPressed(KEY_L)) {
        for (MageBoss* boss : mBosses) {
            if (boss && boss->mActive && boss->mState != MageBoss::DYING) {
                boss->takeDamage(10);
                mCheats.showMessage(TextFormat("BOSS DAMAGED! HP: %d/%d", boss->mHealth, boss->mMaxHealth));
                break;
            }
        }
    }

    if (IsKeyPressed(KEY_F1)) {
        respawnPlayer();
        respawnAllBosses();
        mCheats.showMessage("FULL RESET!");
    }

    if (IsKeyPressed(KEY_F2)) {
        for (MageBoss* boss : mBosses) {
            if (boss && boss->mActive && !boss->mIsEnraged) {
                boss->mHealth = (int)(boss->mMaxHealth * 0.4f);
                boss->checkEnrage();
                mCheats.showMessage(TextFormat("BOSS ENRAGED! HP: %d/%d", boss->mHealth, boss->mMaxHealth));
                break;
            }
        }
    }
}

void GameWrapper::updateEntities(float dt) {
    if (mPlayerPtr && mPlayerPtr->mActive) {
        mPlayerPtr->update(dt);

        if (mCheats.godMode) {
            mPlayerPtr->mHealth = mPlayerPtr->mMaxHealth;
            mPlayerPtr->mInvincibilityTimer = 0.5f;
        }
    }

    for (MageBoss* boss : mBosses) {
        if (boss && boss->mActive) {
            boss->updateBossLogic(dt, mWallEntities);
        }
    }

    for (RabbitEnemy* rabbit : mRabbits) {
        if (rabbit && rabbit->mActive) {
            rabbit->update(dt);
        }
    }
}

void GameWrapper::updateCollisions() {
    if (mCheats.noclip) {
        for (auto& ent : mActiveEntities) {
            if (!ent || !ent->mActive || ent->mType == WALL) continue;
            if (ent.get() == mPlayerPtr) continue;

            mNearbyObstacles.clear();
            mCollisionGrid.getNearby(ent->mPosition.x, ent->mPosition.y, mNearbyObstacles);

            for (Entity* obs : mNearbyObstacles) {
                if (CheckCollisionRecs(ent->getRect(), obs->getRect())) {
                    ent->onCollision(obs);
                }
            }
        }
    }
    else {
        for (auto& ent : mActiveEntities) {
            if (!ent || !ent->mActive || ent->mType == WALL) continue;

            mNearbyObstacles.clear();
            mCollisionGrid.getNearby(ent->mPosition.x, ent->mPosition.y, mNearbyObstacles);

            for (Entity* obs : mNearbyObstacles) {
                if (CheckCollisionRecs(ent->getRect(), obs->getRect())) {
                    ent->onCollision(obs);
                }
            }
        }
    }
}

void GameWrapper::updateLiquids(float dt) {
    if (!mPlayerPtr || !mPlayerPtr->mActive) return;

    Rectangle playerRect = mPlayerPtr->getRect();

    for (const auto& liq : mLiquidRects) {
        Rectangle liquidRect = { (float)liq.x, (float)liq.y, (float)liq.w, (float)liq.h };

        if (CheckCollisionRecs(playerRect, liquidRect)) {
            if (mPlayerPtr->mInvincibilityTimer <= 0) {
                mPlayerPtr->mHealth -= 1;
                mPlayerPtr->mInvincibilityTimer = 1.0f;
                if (mPlayerPtr->mVelocity.y > 0) mPlayerPtr->mVelocity.y = -50.0f;
            }

            mPlayerPtr->mVelocity.x *= 0.9f;

            const float PUSH_FORCE = 900.0f;
            if (mPlayerPtr->mVelocity.y > 0) {
                mPlayerPtr->mVelocity.y -= PUSH_FORCE * 1.2f * dt;
            }
            else {
                mPlayerPtr->mVelocity.y -= PUSH_FORCE * 0.4f * dt;
            }

            if (mPlayerPtr->mVelocity.y > 60.0f) {
                mPlayerPtr->mVelocity.y = 60.0f;
            }
        }
    }
}

void GameWrapper::updateCombat() {
    if (!mPlayerPtr || !mPlayerPtr->mActive) return;

    Vector2 playerCenter = Physics::GetEntityCenter(mPlayerPtr);

    for (MageBoss* boss : mBosses) {
        if (!boss || !boss->mActive) continue;

        if (mPlayerPtr->mIsAttacking) {
            if (CheckCollisionRecs(mPlayerPtr->mAttackArea, boss->getRect())) {
                if (!mPlayerPtr->hasHit(boss)) {
                    Vector2 enemyCenter = Physics::GetEntityCenter(boss);

                    mNearbyObstacles.clear();
                    mCollisionGrid.getNearby(playerCenter.x, playerCenter.y, mNearbyObstacles);

                    if (!Physics::IsPathBlocked(playerCenter, enemyCenter, mNearbyObstacles)) {
                        if (mPlayerPtr->mAttackDir == AttackDirection::DOWN) {
                            mPlayerPtr->pogoBounce();
                        }

                        mPlayerPtr->mHitEntities.push_back(boss);
                        int damage = mPlayerPtr->getAttackDamage();
                        boss->takeDamage(damage);
                    }
                }
            }
        }

        if (boss->mState != MageBoss::VULNERABLE &&
            boss->mState != MageBoss::DYING &&
            boss->mState != MageBoss::INACTIVE) {
            if (CheckCollisionRecs(mPlayerPtr->getRect(), boss->getRect())) {
                if (mPlayerPtr->mInvincibilityTimer <= 0) {
                    mPlayerPtr->mHealth -= 1;
                    mPlayerPtr->mInvincibilityTimer = 1.0f;
                    float knockDir = Physics::GetKnockbackDirection(boss->mPosition.x, mPlayerPtr->mPosition.x);
                    Physics::ApplyKnockback(mPlayerPtr->mVelocity, knockDir, 120.0f, 80.0f);
                }
            }
        }

        if (mCheats.godMode) continue;

        for (auto& fb : boss->mFireballs) {
            if (!fb.active) continue;
            if (mPlayerPtr->mInvincibilityTimer <= 0) {
                Rectangle playerRect = mPlayerPtr->getRect();
                if (CheckCollisionCircleRec(fb.position, fb.radius, playerRect)) {
                    mPlayerPtr->mHealth -= 1;
                    mPlayerPtr->mInvincibilityTimer = 1.0f;
                    float knockDir = Physics::GetKnockbackDirection(fb.position.x, mPlayerPtr->mPosition.x);
                    Physics::ApplyKnockback(mPlayerPtr->mVelocity, knockDir, 150.0f, 100.0f);
                    fb.active = false;
                }
            }
        }

        if (boss->mState == MageBoss::LASER_FIRE) {
            if (mPlayerPtr->mInvincibilityTimer <= 0) {
                if (boss->checkLaserCollision(mPlayerPtr)) {
                    mPlayerPtr->mHealth -= 2;
                    mPlayerPtr->mInvincibilityTimer = 1.5f;
                    float knockDir = Physics::GetKnockbackDirection(boss->mPosition.x, mPlayerPtr->mPosition.x);
                    Physics::ApplyKnockback(mPlayerPtr->mVelocity, knockDir, 200.0f, 150.0f);
                }
            }
        }

        if (boss->checkAoeCollision(mPlayerPtr)) {
            if (mPlayerPtr->mInvincibilityTimer <= 0) {
                mPlayerPtr->mHealth -= 2;
                mPlayerPtr->mInvincibilityTimer = 1.5f;
                float dx = mPlayerPtr->mPosition.x - boss->mPosition.x;
                float dy = mPlayerPtr->mPosition.y - boss->mPosition.y;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0) {
                    mPlayerPtr->mVelocity.x = (dx / len) * 250.0f;
                    mPlayerPtr->mVelocity.y = (dy / len) * 250.0f - 100.0f;
                }
                boss->mAoeDealtDamage = true;
            }
        }
    }

    for (RabbitEnemy* rabbit : mRabbits) {
        if (!rabbit || !rabbit->mActive || rabbit->mIsDead) continue;

        if (mPlayerPtr->mIsAttacking) {
            if (CheckCollisionRecs(mPlayerPtr->mAttackArea, rabbit->getRect())) {
                if (!mPlayerPtr->hasHit(rabbit)) {
                    Vector2 enemyCenter = Physics::GetEntityCenter(rabbit);

                    mNearbyObstacles.clear();
                    mCollisionGrid.getNearby(playerCenter.x, playerCenter.y, mNearbyObstacles);

                    if (!Physics::IsPathBlocked(playerCenter, enemyCenter, mNearbyObstacles)) {
                        if (mPlayerPtr->mAttackDir == AttackDirection::DOWN) {
                            mPlayerPtr->pogoBounce();
                        }

                        mPlayerPtr->mHitEntities.push_back(rabbit);
                        int damage = mPlayerPtr->getAttackDamage();
                        rabbit->takeDamage(damage);

                        if (rabbit->mIsDead) {
                            rabbit->mActive = false;
                        }
                    }
                }
            }
        }

        if (mCheats.godMode) continue;

        if (!rabbit->mIsDead) {
            if (CheckCollisionRecs(mPlayerPtr->getRect(), rabbit->getRect())) {
                if (mPlayerPtr->mInvincibilityTimer <= 0) {
                    int damage = 1;
                    if (rabbit->mState >= RabbitEnemy::MONSTER_CHARGE &&
                        rabbit->mState <= RabbitEnemy::MONSTER_TURN) {
                        damage = 2;
                    }
                    mPlayerPtr->mHealth -= damage;
                    mPlayerPtr->mInvincibilityTimer = 1.0f;
                    float knockDir = Physics::GetKnockbackDirection(rabbit->mPosition.x, mPlayerPtr->mPosition.x);
                    Physics::ApplyKnockback(mPlayerPtr->mVelocity, knockDir, 100.0f, 60.0f);
                }
            }
        }
    }
}

void GameWrapper::updateCamera(float dt) {
    if (mPlayerPtr && mPlayerPtr->mActive) {
        mPlayerPtr->lateUpdate(dt);
        mSmoothCamera.update(mPlayerPtr->mPosition, dt);
        mCamera.target = mSmoothCamera.getPosition();
    }
}

void GameWrapper::drawWorld() {
    BeginTextureMode(mRenderTarget);

    ClearBackground({ 2, 2, 5, 255 });

    Color abyssTop = { 0, 0, 0, 255 };
    Color abyssBottom = { 10, 15, 30, 255 };

    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, abyssTop, abyssBottom);

    if (mBackground.id > 0) {
        float parallaxFactor = 0.15f;
        float offsetX = mCamera.target.x * parallaxFactor;

        Rectangle src = { offsetX, 0.0f, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT };
        Rectangle dest = { 0, 0, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT };
        Color bgTint = { 80, 80, 100, 255 };

        DrawTexturePro(mBackground, src, dest, { 0, 0 }, 0.0f, bgTint);
    }

    BeginMode2D(mCamera);

    int camL = (int)(mCamera.target.x - VIRTUAL_WIDTH / 2 - mGameMap.minX) / TILE_SIZE_PX - 1;
    int camR = (int)(mCamera.target.x + VIRTUAL_WIDTH / 2 - mGameMap.minX) / TILE_SIZE_PX + 2;
    int camT = (int)(mCamera.target.y - VIRTUAL_HEIGHT / 2 - mGameMap.minY) / TILE_SIZE_PX - 1;
    int camB = (int)(mCamera.target.y + VIRTUAL_HEIGHT / 2 - mGameMap.minY) / TILE_SIZE_PX + 2;

    for (int gy = std::max(0, camT); gy < std::min(mGameMap.height, camB); ++gy) {
        for (int gx = std::max(0, camL); gx < std::min(mGameMap.width, camR); ++gx) {
            int id = mGameMap.getTileAt(gx, gy);
            if (id == 0) continue;
            Rectangle src = {
                (float)(id % ATLAS_COLUMNS) * TILE_SIZE_PX,
                (float)(id / ATLAS_COLUMNS) * TILE_SIZE_PX,
                (float)TILE_SIZE_PX,
                (float)TILE_SIZE_PX
            };
            Vector2 pos = {
                (float)(gx * TILE_SIZE_PX + mGameMap.minX),
                (float)(gy * TILE_SIZE_PX + mGameMap.minY)
            };
            DrawTextureRec(mTileset, src, pos, WHITE);
        }
    }

    for (RabbitEnemy* rabbit : mRabbits) {
        if (rabbit && rabbit->mActive) {
            rabbit->draw();
        }
    }

    for (MageBoss* boss : mBosses) {
        if (boss && boss->mActive) {
            boss->draw();
        }
    }

    if (mPlayerPtr && mPlayerPtr->mActive) {
        mPlayerPtr->draw();

        if (mCheats.godMode) {
            DrawCircleLines(
                (int)(mPlayerPtr->mPosition.x + mPlayerPtr->mSize.x / 2),
                (int)(mPlayerPtr->mPosition.y + mPlayerPtr->mSize.y / 2),
                (int)(mPlayerPtr->mSize.x * 0.8f + sinf(GetTime() * 5) * 3),
                Fade(GOLD, 0.5f)
            );
        }

        if (mCheats.noclip) {
            DrawCircleLines(
                (int)(mPlayerPtr->mPosition.x + mPlayerPtr->mSize.x / 2),
                (int)(mPlayerPtr->mPosition.y + mPlayerPtr->mSize.y / 2),
                (int)(mPlayerPtr->mSize.x * 0.6f),
                Fade(SKYBLUE, 0.6f)
            );
        }
    }

    if (mShowDebug) {
        if (mPlayerPtr) {
            DrawRectangleLinesEx(mPlayerPtr->getRect(), 1, GREEN);
            if (mPlayerPtr->mIsAttacking) {
                DrawRectangleLinesEx(mPlayerPtr->mAttackArea, 1, YELLOW);
            }
            DrawText(TextFormat("%.0f,%.0f", mPlayerPtr->mPosition.x, mPlayerPtr->mPosition.y),
                (int)mPlayerPtr->mPosition.x - 15, (int)mPlayerPtr->mPosition.y - 20, 5, LIME);
        }

        for (RabbitEnemy* rabbit : mRabbits) {
            if (rabbit && rabbit->mActive) {
                DrawRectangleLinesEx(rabbit->getRect(), 1, ORANGE);
            }
        }

        for (MageBoss* boss : mBosses) {
            if (boss && boss->mActive) {
                DrawText(TextFormat("HP:%d/%d", boss->mHealth, boss->mMaxHealth),
                    (int)boss->mPosition.x, (int)boss->mPosition.y - 30, 6,
                    boss->mIsEnraged ? ORANGE : GREEN);
            }
        }
    }

    EndMode2D();
    EndTextureMode();
}

void GameWrapper::respawnAllBosses() {
    int bossIndex = 0;
    for (MageBoss* boss : mBosses) {
        if (boss && bossIndex < (int)mBossSpawnPoints.size()) {
            const auto& spawn = mBossSpawnPoints[bossIndex];

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
    mCheats.showMessage("BOSSES RESPAWNED!");
}

void GameWrapper::respawnPlayer() {
    if (mPlayerPtr) {
        mPlayerPtr->mPosition = mPlayerSpawnPoint;
        mPlayerPtr->mVelocity = { 0, 0 };
        mPlayerPtr->mHealth = mPlayerPtr->mMaxHealth;
        mPlayerPtr->mInvincibilityTimer = 1.0f;
        mPlayerPtr->mActive = true;
        mSmoothCamera.setPosition(mPlayerSpawnPoint);
    }
    mCheats.showMessage("PLAYER RESPAWNED!");
}

void GameWrapper::reset() {
    respawnPlayer();
    respawnAllBosses();
}

void GameWrapper::drawUI(int windowWidth, int windowHeight) {
    BeginDrawing();
    ClearBackground(BLACK);

    float scale = std::min((float)windowWidth / VIRTUAL_WIDTH, (float)windowHeight / VIRTUAL_HEIGHT);
    DrawTexturePro(
        mRenderTarget.texture,
        { 0, 0, (float)mRenderTarget.texture.width, -(float)mRenderTarget.texture.height },
        { (windowWidth - VIRTUAL_WIDTH * scale) / 2, (windowHeight - VIRTUAL_HEIGHT * scale) / 2,
          VIRTUAL_WIDTH * scale, VIRTUAL_HEIGHT * scale },
        { 0, 0 }, 0, WHITE
    );

    Color panelBg = Fade(BLACK, 0.75f);
    Color panelBorder = { 60, 70, 110, 255 };
    Color textNormal = { 200, 200, 220, 255 };
    Color textAccent = { 100, 180, 255, 255 };
    Color hpColor = { 180, 40, 60, 255 };

    if (mPlayerPtr) {
        WeaponStats currentWeapon = mPlayerPtr->getCurrentWeaponStats();
        int panelW = 220;
        int panelX = windowWidth - panelW - 20;
        int panelY = 20;
        int panelH = 100;

        DrawRectangle(panelX, panelY, panelW, panelH, panelBg);
        DrawRectangleLines(panelX, panelY, panelW, panelH, panelBorder);

        const char* weaponName = "???";
        WeaponType currentType = mPlayerPtr->getCurrentWeaponType();
        switch (currentType) {
        case WeaponType::SWORD_DEFAULT: weaponName = "VOID BLADE"; break;
        case WeaponType::DAGGER_SWIFT:  weaponName = "SHADOW DAGGER"; break;
        case WeaponType::AXE_HEAVY:     weaponName = "ABYSS AXE"; break;
        case WeaponType::SPEAR_LONG:    weaponName = "PIERCER"; break;
        case WeaponType::KATANA_BLOOD:  weaponName = "BLOOD KATANA"; break;
        }

        Color weaponColor = currentWeapon.slashColor;
        if (mWeaponUI.isAnimating()) {
            float pulse = sinf(mWeaponUI.changeTimer * 20.0f) * 0.5f + 0.5f;
            weaponColor = ColorAlpha(weaponColor, 0.5f + pulse * 0.5f);
        }

        DrawText(weaponName, panelX + 10, panelY + 10, 20, weaponColor);

        int statY = panelY + 35;
        DrawText(TextFormat("DMG: %.0f", currentWeapon.damage), panelX + 10, statY, 14, textNormal);
        statY += 18;
        DrawText(TextFormat("RNG: %.0f", currentWeapon.reachX), panelX + 10, statY, 14, textAccent);
        statY += 18;
        DrawText(TextFormat("SPD: %.2fs", currentWeapon.attackCooldown), panelX + 10, statY, 14, ORANGE);

        int iconX = panelX + 120;
        int iconY = panelY + 35;
        const WeaponType allWeapons[] = {
            WeaponType::SWORD_DEFAULT, WeaponType::DAGGER_SWIFT, WeaponType::AXE_HEAVY,
            WeaponType::SPEAR_LONG, WeaponType::KATANA_BLOOD
        };

        for (int i = 0; i < 5; i++) {
            WeaponStats ws = WeaponStats::GetStats(allWeapons[i]);
            bool isSelected = (allWeapons[i] == currentType);
            Color iconColor = isSelected ? ws.slashColor : Fade(ws.slashColor, 0.3f);
            int size = isSelected ? 10 : 8;
            DrawRectangle(iconX, iconY + i * 12, size, size, iconColor);
            DrawText(TextFormat("%d", i + 1), iconX + 14, iconY + i * 12, 10, isSelected ? WHITE : GRAY);
        }

        int statsY = panelY + panelH + 10;
        int statsH = 110;

        DrawRectangle(panelX, statsY, panelW, statsH, panelBg);
        DrawRectangleLines(panelX, statsY, panelW, statsH, panelBorder);

        DrawText("VITALITY", panelX + 10, statsY + 10, 10, GRAY);
        float hpPercent = (float)mPlayerPtr->mHealth / (float)mPlayerPtr->mMaxHealth;
        int barW = 140;
        int barH = 12;
        DrawRectangle(panelX + 10, statsY + 25, barW, barH, Fade(BLACK, 0.5f));
        DrawRectangle(panelX + 10, statsY + 25, (int)(barW * hpPercent), barH, mCheats.godMode ? GOLD : hpColor);
        DrawRectangleLines(panelX + 10, statsY + 25, barW, barH, Fade(WHITE, 0.2f));
        DrawText(TextFormat("%d / %d", mPlayerPtr->mHealth, mPlayerPtr->mMaxHealth), panelX + barW + 20, statsY + 25, 10, WHITE);

        if (mPlayerPtr->mInvincibilityTimer > 0) {
            DrawText("GHOST FORM", panelX + 10, statsY + 45, 10, GOLD);
            float invW = (mPlayerPtr->mInvincibilityTimer / 1.0f) * barW;
            if (invW > barW) invW = barW;
            DrawRectangle(panelX + 10, statsY + 56, (int)invW, 2, GOLD);
        }

        int jumpY = statsY + 70;
        DrawText("AGILITY", panelX + 10, jumpY, 10, GRAY);

        int jumps = mPlayerPtr->getMaxJumps() - mPlayerPtr->getJumpCount();
        for (int i = 0; i < mPlayerPtr->getMaxJumps(); i++) {
            Color jColor = (i < jumps) ? SKYBLUE : Fade(DARKBLUE, 0.3f);
            DrawRectangle(panelX + 10 + (i * 15), jumpY + 15, 10, 10, jColor);
        }
        DrawText("JUMPS", panelX + 45, jumpY + 15, 10, textNormal);

        Color dashColor = mPlayerPtr->canDash() ? LIME : Fade(MAROON, 0.5f);
        DrawRectangle(panelX + 100, jumpY + 15, 10, 10, dashColor);
        DrawText(mPlayerPtr->canDash() ? "DASH READY" : "RECHARGING", panelX + 115, jumpY + 15, 10, mPlayerPtr->canDash() ? textNormal : GRAY);
    }

    if (mCheats.hasActiveMessage()) {
        float alpha = std::min(mCheats.messageTimer, 1.0f);
        int textWidth = MeasureText(mCheats.message.c_str(), 24);
        DrawText(mCheats.message.c_str(),
            (windowWidth - textWidth) / 2,
            windowHeight / 2 - 50,
            24,
            Fade(YELLOW, alpha));
    }

    for (MageBoss* boss : mBosses) {
        if (boss && boss->mActive && boss->mState != MageBoss::INACTIVE) {
            float bossBarWidth = 400.0f;
            float bossBarX = (windowWidth - bossBarWidth) / 2;
            float bossBarY = 40.0f;
            float hpPercent = (float)boss->mHealth / (float)boss->mMaxHealth;

            DrawRectangle((int)bossBarX - 2, (int)bossBarY - 2, (int)bossBarWidth + 4, 14, BLACK);
            DrawRectangle((int)bossBarX, (int)bossBarY, (int)bossBarWidth, 10, Fade(DARKGRAY, 0.8f));
            DrawRectangle((int)bossBarX, (int)bossBarY, (int)(bossBarWidth * hpPercent), 10,
                boss->mIsEnraged ? RED : PURPLE);

            DrawText("ABYSS MAGE", (int)bossBarX, (int)bossBarY - 20, 20, textNormal);

            if (boss->mIsEnraged) {
                DrawText("FRENZY", (int)(bossBarX + bossBarWidth + 10), (int)bossBarY - 2, 12, RED);
            }
            break;
        }
    }

    int ctrlX = 20;
    int ctrlH = 140;
    int ctrlY = windowHeight - ctrlH - 20;
    int ctrlW = 180;

    DrawRectangle(ctrlX, ctrlY, ctrlW, ctrlH, panelBg);
    DrawRectangleLines(ctrlX, ctrlY, ctrlW, ctrlH, panelBorder);

    int txtY = ctrlY + 10;
    int gap = 18;
    int fontSize = 10;
    Color keyColor = YELLOW;
    Color descColor = textNormal;

    DrawText("CONTROLS", ctrlX + 10, txtY, 10, GRAY); txtY += 20;

    DrawText("[ARROWS]", ctrlX + 10, txtY, fontSize, keyColor);
    DrawText("Move / Climb", ctrlX + 70, txtY, fontSize, descColor); txtY += gap;

    DrawText("[SPACE]", ctrlX + 10, txtY, fontSize, keyColor);
    DrawText("Jump / Wall", ctrlX + 70, txtY, fontSize, descColor); txtY += gap;

    DrawText("[C]", ctrlX + 10, txtY, fontSize, keyColor);
    DrawText("Dash", ctrlX + 70, txtY, fontSize, descColor); txtY += gap;

    DrawText("[Z]", ctrlX + 10, txtY, fontSize, keyColor);
    DrawText("Attack", ctrlX + 70, txtY, fontSize, descColor); txtY += gap;

    DrawText("[1-5]", ctrlX + 10, txtY, fontSize, keyColor);
    DrawText("Weapon", ctrlX + 70, txtY, fontSize, descColor); txtY += gap;

    DrawText("[F3]", ctrlX + 10, txtY, fontSize, keyColor);
    DrawText("Debug Info", ctrlX + 70, txtY, fontSize, descColor);

    if (mShowDebug) {
        int dbgX = 20;
        int dbgY = 20;
        int dbgW = 180;
        int dbgH = 280;

        DrawRectangle(dbgX, dbgY, dbgW, dbgH, Fade(BLACK, 0.9f));
        DrawRectangleLines(dbgX, dbgY, dbgW, dbgH, LIME);

        int dy = dbgY + 10;
        int dGap = 15;

        DrawText(TextFormat("FPS: %d", GetFPS()), dbgX + 10, dy, 10, LIME); dy += dGap;
        DrawText(TextFormat("ENTITIES: %d", (int)mActiveEntities.size()), dbgX + 10, dy, 10, WHITE); dy += dGap;

        if (mPlayerPtr) {
            DrawText(TextFormat("POS: %.0f, %.0f", mPlayerPtr->mPosition.x, mPlayerPtr->mPosition.y), dbgX + 10, dy, 10, WHITE);
            dy += dGap * 2;
        }

        DrawText("STATUS:", dbgX + 10, dy, 10, YELLOW); dy += dGap;

        DrawText("GOD MODE:", dbgX + 10, dy, 10, WHITE);
        DrawText(mCheats.godMode ? "ON" : "OFF", dbgX + 90, dy, 10, mCheats.godMode ? GOLD : RED);
        dy += dGap;

        DrawText("NOCLIP:", dbgX + 10, dy, 10, WHITE);
        DrawText(mCheats.noclip ? "ON" : "OFF", dbgX + 90, dy, 10, mCheats.noclip ? SKYBLUE : RED);
        dy += dGap * 2;

        DrawText("CHEAT KEYS:", dbgX + 10, dy, 10, YELLOW); dy += dGap;
        DrawText("[G] God Mode", dbgX + 10, dy, 10, GRAY); dy += dGap;
        DrawText("[N] Noclip", dbgX + 10, dy, 10, GRAY); dy += dGap;
        DrawText("[H] Heal", dbgX + 10, dy, 10, GRAY); dy += dGap;
        DrawText("[T] TP to Boss", dbgX + 10, dy, 10, GRAY); dy += dGap;
        DrawText("[K] Kill Boss", dbgX + 10, dy, 10, GRAY); dy += dGap;
        DrawText("[R] Respawn", dbgX + 10, dy, 10, GRAY); dy += dGap;
        DrawText("[F1] Reset", dbgX + 10, dy, 10, GRAY); dy += dGap;
        DrawText("[F2] Enrage", dbgX + 10, dy, 10, GRAY);
    }

    EndDrawing();
}