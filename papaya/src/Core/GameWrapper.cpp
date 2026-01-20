#include "GameWrapper.hpp"
#include "Physics.hpp"
#include "Audio/AudioManager.hpp"

#include "../Saves/saves.hpp"
#include "../Entities/Entity.h"
#include "../Entities/Player/Player.h"
#include "../Entities/Environment/Wall.h"
#include "../Entities/Enemies/mage_boss.h"
#include "../Entities/Enemies/rabbit.h"

#include <iostream>
#include <algorithm>

using namespace GameConstants;

// =============================================================================
// MAPA I KAMERA
// =============================================================================

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

// =============================================================================
// GAME WRAPPER - LOGIKA GRY
// =============================================================================

GameWrapper::GameWrapper(bool& shouldQuit, int& state)
    : mShouldQuit(shouldQuit)
    , mState(state)
{
    std::cout << "[GameWrapper] Inicjalizacja..." << std::endl;

    AudioManager::getInstance()->init();

    {
        Saves tempSaves(currentSavePath);
        if (tempSaves.getMap().getRenderData().empty()) {
            std::cout << "[SYSTEM] Wykryto pusty save startowy. Próba importu..." << std::endl;
            try {
                tempSaves.loadFromEditorDir(EDITOR_PATH);
            }
            catch (const std::exception& e) {
                std::cout << "[ERROR] B³¹d importu: " << e.what() << std::endl;
            }
        }
    }

    loadTextures();
    loadAudio();
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
    std::cout << "[GameWrapper] Gotowy!" << std::endl;
}

GameWrapper::~GameWrapper() {
    std::cout << "[GameWrapper] Zamykanie (zapis stanu)..." << std::endl;
    saveGame();
    unloadTextures();
    AudioManager::getInstance()->shutdown();
    UnloadRenderTexture(mRenderTarget);
}

void GameWrapper::loadAudio() {
    AudioManager* am = AudioManager::getInstance();

    am->loadSound("jump", "assets/audio/jump.wav");
    am->loadSound("dash", "assets/audio/jump.wav");
    am->loadSound("attack", "assets/audio/attack.wav");
    am->loadSound("hurt", "assets/audio/hurt.wav");
    am->loadSound("death", "assets/audio/death.wav");

    //am->loadMusic("bgm1", "assets/audio/Abyss.ogg");
    //am->loadMusic("bgm2", "assets/audio/Abyss_deep.ogg");

    mCurrentMusicTrack = 1;
    am->playMusic("bgm1", false);
}

void GameWrapper::updateBackgroundMusic() {
    AudioManager* am = AudioManager::getInstance();

    if (!am->isMusicPlaying()) {

        if (mCurrentMusicTrack == 1) {
            mCurrentMusicTrack = 2;
            am->playMusic("bgm2", false);
            std::cout << "[AUDIO] Track 1 koniec -> Gramy Abyss_deep" << std::endl;
        }
        else {
            mCurrentMusicTrack = 1;
            am->playMusic("bgm1", false);
            std::cout << "[AUDIO] Track 2 koniec -> Gramy Abyss" << std::endl;
        }
    }
}

void GameWrapper::reloadSave() {

    std::cout << "[GameWrapper] Prze³¹czanie na slot: " << currentSavePath << std::endl;

    mActiveEntities.clear();
    mWallEntities.clear();
    mBosses.clear();
    mRabbits.clear();
    mBossSpawnPoints.clear();
    mLiquidRects.clear();
    mNearbyObstacles.clear();
    mPlayerPtr = nullptr;
    mCollisionGrid.clear();

    {
        Saves tempSaves(currentSavePath);
        if (tempSaves.getMap().getRenderData().empty()) {
            std::cout << "[SYSTEM] Nowy slot pusty - import z edytora..." << std::endl;
            try {
                tempSaves.loadFromEditorDir(EDITOR_PATH);
            }
            catch (const std::exception& e) {
                std::cout << "[ERROR] B³¹d importu: " << e.what() << std::endl;
            }
        }
    }

    loadMap();
    loadEntities();

    if (mPlayerPtr) {
        std::cout << "[RELOAD] Gracz ustawiony na: " << mPlayerPtr->mPosition.x << ", " << mPlayerPtr->mPosition.y << std::endl;
        mSmoothCamera.setPosition(mPlayerPtr->mPosition);
        mCamera.target = mSmoothCamera.getPosition();
        mCheats.showMessage("GAME LOADED");
    }
    else {
        std::cout << "[RELOAD] B£¥D: Gracz nie zosta³ stworzony!" << std::endl;
    }
}

void GameWrapper::saveGame() {
    if (!mPlayerPtr) return;

    Saves saves(currentSavePath);
    saves.clearSaveData();

    EntityS pData;
    pData.x = (int16_t)(mPlayerPtr->mPosition.x / TILE_SIZE_PX);
    pData.y = (int16_t)(mPlayerPtr->mPosition.y / TILE_SIZE_PX);
    pData.entityType = PLAYER;
    pData.health = (uint8_t)mPlayerPtr->mHealth;

    saves.saveEntityToMap().addEntityS(pData);
    std::cout << "[GAME] Zapisano stan w: " << currentSavePath << std::endl;
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
    Saves saves(currentSavePath);

    mGameMap.init(saves.getMap().getRenderData());
    mCollisionGrid.init(mGameMap.width * TILE_SIZE_PX + 1000, mGameMap.height * TILE_SIZE_PX + 1500, mGameMap.minX - 500, mGameMap.minY - 1000);

    auto collisions = saves.getMap().getCollisions();
    for (const auto& col : collisions) {
        auto wall = std::make_unique<Wall>((float)col.x, (float)col.y, (float)col.w, (float)col.h);
        mCollisionGrid.insertStatic(wall.get());
        mWallEntities.push_back(wall.get());
        mActiveEntities.push_back(std::move(wall));
    }

    mLiquidRects = saves.getMap().getDamagingZones();
}

void GameWrapper::loadEntities() {
    Saves saves(currentSavePath);

    Vector2 spawnPos = mPlayerSpawnPoint;
    int spawnHP = 5;

    try {
        auto savedData = saves.getSaveData().getAll();
        for (const auto& s : savedData) {
            if (s.entityType == PLAYER) {
                spawnPos.x = (float)(s.x * TILE_SIZE_PX);
                spawnPos.y = (float)(s.y * TILE_SIZE_PX);
                spawnHP = s.health;
                std::cout << "[LOAD] Znaleziono zapis gracza: " << spawnPos.x << ", " << spawnPos.y << std::endl;
                break;
            }
        }
    }
    catch (...) {}

    {
        auto p = std::make_unique<Player>(spawnPos.x, spawnPos.y);
        p->mTexture = mPlayerTexture;
        p->mHealth = spawnHP;
        mPlayerPtr = p.get();
        mActiveEntities.push_back(std::move(p));
    }

    auto rawEntities = saves.getEntities().getAll();
    for (const auto& e : rawEntities) {
        if (e.entityType == PLAYER) continue;
        float px = (float)(e.x * TILE_SIZE_PX);
        float py = (float)(e.y * TILE_SIZE_PX);

        if (e.entityType == MAGE_BOSS) {
            auto boss = std::make_unique<MageBoss>(px, py, mBossTexture, mPlayerPtr);
            Rectangle arena = { boss->mPosition.x - 116, boss->mPosition.y - 50, 232, 100 };
            boss->setArenaBounds(arena.x, arena.y, arena.width, arena.height);
            mBossSpawnPoints.push_back({ {px, py}, arena, boss->mMaxHealth });
            mBosses.push_back(boss.get());
            mActiveEntities.push_back(std::move(boss));
        }
        else if (e.entityType == RABBIT) {
            auto r = std::make_unique<RabbitEnemy>(px, py, mRabbitTexture, mPlayerPtr);
            if (e.health > 0) r->mHealth = e.health;
            mRabbits.push_back(r.get());
            mActiveEntities.push_back(std::move(r));
        }
    }

    if (mBosses.empty() && mRabbits.empty()) createTestEnemies();
}

void GameWrapper::createTestEnemies() {
    auto testBoss = std::make_unique<MageBoss>(666.0f, -376.0f, mBossTexture, mPlayerPtr);
    Rectangle arena = { 550.0f, -448.0f, 232.0f, 100.0f };
    testBoss->setArenaBounds(arena.x, arena.y, arena.width, arena.height);
    mBossSpawnPoints.push_back({ {666.0f, -376.0f}, arena, testBoss->mMaxHealth });
    mBosses.push_back(testBoss.get());
    mActiveEntities.push_back(std::move(testBoss));

    auto testRabbit = std::make_unique<RabbitEnemy>(270.0f, 322.0f, mRabbitTexture, mPlayerPtr);
    mRabbits.push_back(testRabbit.get());
    mActiveEntities.push_back(std::move(testRabbit));
}

void GameWrapper::runFrame(int windowWidth, int windowHeight) {
    float dt = std::min(GetFrameTime(), 0.033f);

    AudioManager::getInstance()->updateMusic();

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
        saveGame();
        mState = 0;
        return;
    }
    if (IsKeyPressed(KEY_F3)) mShowDebug = !mShowDebug;

    if (mPlayerPtr) {
        bool weaponChanged = false;
        if (IsKeyPressed(KEY_ONE)) { mPlayerPtr->setWeapon(WeaponType::SWORD_DEFAULT); weaponChanged = true; }
        else if (IsKeyPressed(KEY_TWO)) { mPlayerPtr->setWeapon(WeaponType::DAGGER_SWIFT); weaponChanged = true; }
        else if (IsKeyPressed(KEY_THREE)) { mPlayerPtr->setWeapon(WeaponType::AXE_HEAVY); weaponChanged = true; }
        else if (IsKeyPressed(KEY_FOUR)) { mPlayerPtr->setWeapon(WeaponType::SPEAR_LONG); weaponChanged = true; }
        else if (IsKeyPressed(KEY_FIVE)) { mPlayerPtr->setWeapon(WeaponType::KATANA_BLOOD); weaponChanged = true; }
        if (weaponChanged) mWeaponUI.triggerChange();
    }

    if (IsKeyPressed(KEY_G)) {
        mCheats.godMode = !mCheats.godMode;
        mCheats.showMessage(mCheats.godMode ? "GODMODE: ON" : "GODMODE: OFF");
    }
    if (IsKeyPressed(KEY_N)) {
        mCheats.noclip = !mCheats.noclip;
        if (mPlayerPtr) mPlayerPtr->mNoclip = mCheats.noclip;
        mCheats.showMessage(mCheats.noclip ? "NOCLIP: ON" : "NOCLIP: OFF");
    }
    if (IsKeyPressed(KEY_R)) respawnAllBosses();
    if (IsKeyPressed(KEY_T)) {
        if (mPlayerPtr && !mBosses.empty()) {
            mPlayerPtr->mPosition = { mBosses[0]->mPosition.x - 60.0f, mBosses[0]->mPosition.y };
            mPlayerPtr->mVelocity = { 0,0 };
            mSmoothCamera.setPosition(mPlayerPtr->mPosition);
            mCheats.showMessage("TELEPORTED TO BOSS!");
        }
    }
    if (IsKeyPressed(KEY_H) && mPlayerPtr) {
        mPlayerPtr->mHealth = mPlayerPtr->mMaxHealth;
        mCheats.showMessage("HEALTH RESTORED!");
    }
    if (IsKeyPressed(KEY_K)) {
        for (auto* b : mBosses) if (b && b->mActive) b->takeDamage(9999);
        mCheats.showMessage("BOSSES KILLED!");
    }
    if (IsKeyPressed(KEY_L)) {
        for (auto* b : mBosses) if (b && b->mActive) { b->takeDamage(10); break; }
    }
    if (IsKeyPressed(KEY_F1)) {
        respawnPlayer();
        respawnAllBosses();
        mCheats.showMessage("FULL RESET!");
    }
    if (IsKeyPressed(KEY_F2)) {
        for (auto* b : mBosses) if (b && b->mActive) { b->mHealth = (int)(b->mMaxHealth * 0.4f); b->checkEnrage(); break; }
        mCheats.showMessage("BOSS ENRAGED!");
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
    for (auto* b : mBosses) if (b && b->mActive) b->updateBossLogic(dt, mWallEntities);
    for (auto* r : mRabbits) if (r && r->mActive) r->update(dt);
}

void GameWrapper::updateCollisions() {
    for (auto& ent : mActiveEntities) {
        if (!ent || !ent->mActive || ent->mType == WALL) continue;
        if (mCheats.noclip && ent.get() == mPlayerPtr) continue;

        mNearbyObstacles.clear();
        mCollisionGrid.getNearby(ent->mPosition.x, ent->mPosition.y, mNearbyObstacles);

        for (Entity* obs : mNearbyObstacles) {
            if (CheckCollisionRecs(ent->getRect(), obs->getRect())) {
                ent->onCollision(obs);
            }
        }
    }
}

void GameWrapper::updateLiquids(float dt) {
    if (!mPlayerPtr || !mPlayerPtr->mActive) return;
    Rectangle pRect = mPlayerPtr->getRect();

    for (const auto& liq : mLiquidRects) {
        if (CheckCollisionRecs(pRect, { (float)liq.x, (float)liq.y, (float)liq.w, (float)liq.h })) {
            if (mPlayerPtr->mInvincibilityTimer <= 0) {
                mPlayerPtr->mHealth -= 1;
                mPlayerPtr->mInvincibilityTimer = 1.0f;
                if (mPlayerPtr->mVelocity.y > 0) mPlayerPtr->mVelocity.y = -50.0f;
            }
            mPlayerPtr->mVelocity.x *= 0.9f;
            float push = 900.0f;
            if (mPlayerPtr->mVelocity.y > 0) mPlayerPtr->mVelocity.y -= push * 1.2f * dt;
            else mPlayerPtr->mVelocity.y -= push * 0.4f * dt;
            if (mPlayerPtr->mVelocity.y > 60.0f) mPlayerPtr->mVelocity.y = 60.0f;
        }
    }
}

void GameWrapper::updateCombat() {
    if (!mPlayerPtr || !mPlayerPtr->mActive) return;
    Vector2 pc = Physics::GetEntityCenter(mPlayerPtr);

    for (auto* boss : mBosses) {
        if (!boss || !boss->mActive) continue;
        if (mPlayerPtr->mIsAttacking && CheckCollisionRecs(mPlayerPtr->mAttackArea, boss->getRect())) {
            if (!mPlayerPtr->hasHit(boss)) {
                Vector2 ec = Physics::GetEntityCenter(boss);
                mNearbyObstacles.clear();
                mCollisionGrid.getNearby(pc.x, pc.y, mNearbyObstacles);
                if (!Physics::IsPathBlocked(pc, ec, mNearbyObstacles)) {
                    if (mPlayerPtr->mAttackDir == AttackDirection::DOWN) mPlayerPtr->pogoBounce();
                    mPlayerPtr->mHitEntities.push_back(boss);
                    boss->takeDamage(mPlayerPtr->getAttackDamage());
                }
            }
        }

        // Zderzenie z bossem (obra¿enia)
        if (boss->mState != MageBoss::VULNERABLE && boss->mState != MageBoss::DYING && boss->mState != MageBoss::INACTIVE) {
            if (CheckCollisionRecs(mPlayerPtr->getRect(), boss->getRect()) && mPlayerPtr->mInvincibilityTimer <= 0) {
                mPlayerPtr->mHealth -= 1;
                mPlayerPtr->mInvincibilityTimer = 1.0f;

                AudioManager::getInstance()->playSound("hurt");
                float dir = Physics::GetKnockbackDirection(boss->mPosition.x, mPlayerPtr->mPosition.x);
                Physics::ApplyKnockback(mPlayerPtr->mVelocity, dir, 120.0f, 80.0f);
            }
        }

        if (mCheats.godMode) continue;

        // Fireballs
        for (auto& fb : boss->mFireballs) {
            if (fb.active && mPlayerPtr->mInvincibilityTimer <= 0 && CheckCollisionCircleRec(fb.position, fb.radius, mPlayerPtr->getRect())) {
                mPlayerPtr->mHealth -= 1;
                mPlayerPtr->mInvincibilityTimer = 1.0f;
                AudioManager::getInstance()->playSound("hurt");
                float dir = Physics::GetKnockbackDirection(fb.position.x, mPlayerPtr->mPosition.x);
                Physics::ApplyKnockback(mPlayerPtr->mVelocity, dir, 150.0f, 100.0f);
                fb.active = false;
            }
        }

        // Laser
        if (boss->mState == MageBoss::LASER_FIRE && mPlayerPtr->mInvincibilityTimer <= 0 && boss->checkLaserCollision(mPlayerPtr)) {
            mPlayerPtr->mHealth -= 2;
            mPlayerPtr->mInvincibilityTimer = 1.5f;
            AudioManager::getInstance()->playSound("hurt");
            float dir = Physics::GetKnockbackDirection(boss->mPosition.x, mPlayerPtr->mPosition.x);
            Physics::ApplyKnockback(mPlayerPtr->mVelocity, dir, 200.0f, 150.0f);
        }

        // AOE
        if (boss->checkAoeCollision(mPlayerPtr) && mPlayerPtr->mInvincibilityTimer <= 0) {
            mPlayerPtr->mHealth -= 2;
            mPlayerPtr->mInvincibilityTimer = 1.5f;
            AudioManager::getInstance()->playSound("hurt");
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

    for (auto* r : mRabbits) {
        if (!r || !r->mActive || r->mIsDead) continue;
        if (mPlayerPtr->mIsAttacking && CheckCollisionRecs(mPlayerPtr->mAttackArea, r->getRect())) {
            if (!mPlayerPtr->hasHit(r)) {
                Vector2 ec = Physics::GetEntityCenter(r);
                mNearbyObstacles.clear();
                mCollisionGrid.getNearby(pc.x, pc.y, mNearbyObstacles);
                if (!Physics::IsPathBlocked(pc, ec, mNearbyObstacles)) {
                    if (mPlayerPtr->mAttackDir == AttackDirection::DOWN) mPlayerPtr->pogoBounce();
                    mPlayerPtr->mHitEntities.push_back(r);
                    r->takeDamage(mPlayerPtr->getAttackDamage());
                    if (r->mIsDead) r->mActive = false;
                }
            }
        }
        if (mCheats.godMode) continue;
        if (!r->mIsDead && CheckCollisionRecs(mPlayerPtr->getRect(), r->getRect()) && mPlayerPtr->mInvincibilityTimer <= 0) {
            int dmg = (r->mState >= RabbitEnemy::MONSTER_CHARGE && r->mState <= RabbitEnemy::MONSTER_TURN) ? 2 : 1;
            mPlayerPtr->mHealth -= dmg;
            mPlayerPtr->mInvincibilityTimer = 1.0f;
            AudioManager::getInstance()->playSound("hurt");
            float dir = Physics::GetKnockbackDirection(r->mPosition.x, mPlayerPtr->mPosition.x);
            Physics::ApplyKnockback(mPlayerPtr->mVelocity, dir, 100.0f, 60.0f);
        }
    }

    if (mPlayerPtr && mPlayerPtr->mHealth <= 0 && mPlayerPtr->mActive) {
        AudioManager::getInstance()->playSound("death");
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
    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, { 0, 0, 0, 255 }, { 10, 15, 30, 255 });

    if (mBackground.id > 0) {
        float px = mCamera.target.x * 0.15f;
        DrawTexturePro(mBackground, { px, 0, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT },
            { 0, 0, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT }, { 0,0 }, 0.0f, { 80, 80, 100, 255 });
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
            Rectangle src = { (float)(id % ATLAS_COLUMNS) * TILE_SIZE_PX, (float)(id / ATLAS_COLUMNS) * TILE_SIZE_PX, (float)TILE_SIZE_PX, (float)TILE_SIZE_PX };
            DrawTextureRec(mTileset, src, { (float)(gx * TILE_SIZE_PX + mGameMap.minX), (float)(gy * TILE_SIZE_PX + mGameMap.minY) }, WHITE);
        }
    }

    for (auto* r : mRabbits) if (r && r->mActive) r->draw();
    for (auto* b : mBosses) if (b && b->mActive) b->draw();

    if (mPlayerPtr && mPlayerPtr->mActive) {
        mPlayerPtr->draw();
        if (mCheats.godMode) DrawCircleLines((int)(mPlayerPtr->mPosition.x + 8), (int)(mPlayerPtr->mPosition.y + 8), 12, Fade(GOLD, 0.5f));
        if (mCheats.noclip) DrawCircleLines((int)(mPlayerPtr->mPosition.x + 8), (int)(mPlayerPtr->mPosition.y + 8), 10, Fade(SKYBLUE, 0.6f));
    }

    if (mShowDebug) {
        if (mPlayerPtr) {
            DrawRectangleLinesEx(mPlayerPtr->getRect(), 1, GREEN);
            if (mPlayerPtr->mIsAttacking) DrawRectangleLinesEx(mPlayerPtr->mAttackArea, 1, YELLOW);
        }
        for (auto* r : mRabbits) if (r && r->mActive) DrawRectangleLinesEx(r->getRect(), 1, ORANGE);
        for (auto* b : mBosses) if (b && b->mActive) DrawText(TextFormat("HP:%d", b->mHealth), (int)b->mPosition.x, (int)b->mPosition.y - 20, 6, GREEN);
    }

    EndMode2D();
    EndTextureMode();
}

void GameWrapper::drawUI(int windowWidth, int windowHeight) {
    BeginDrawing();
    ClearBackground(BLACK);

    float scale = std::min((float)windowWidth / VIRTUAL_WIDTH, (float)windowHeight / VIRTUAL_HEIGHT);
    DrawTexturePro(mRenderTarget.texture, { 0, 0, (float)mRenderTarget.texture.width, -(float)mRenderTarget.texture.height },
        { (windowWidth - VIRTUAL_WIDTH * scale) / 2, (windowHeight - VIRTUAL_HEIGHT * scale) / 2, VIRTUAL_WIDTH * scale, VIRTUAL_HEIGHT * scale },
        { 0, 0 }, 0, WHITE);

    Color panelBg = Fade(BLACK, 0.75f);
    Color panelBorder = { 60, 70, 110, 255 };
    Color textNormal = { 200, 200, 220, 255 };
    Color textAccent = { 100, 180, 255, 255 };
    Color hpColor = { 180, 40, 60, 255 };

    if (mPlayerPtr) {
        int px = windowWidth - 240, py = 20;
        DrawRectangle(px, py, 220, 100, panelBg);
        DrawRectangleLines(px, py, 220, 100, panelBorder);

        WeaponStats ws = mPlayerPtr->getCurrentWeaponStats();
        Color wc = ws.slashColor;
        if (mWeaponUI.isAnimating()) wc = ColorAlpha(wc, 0.5f + sinf(mWeaponUI.changeTimer * 20.0f) * 0.5f);

        const char* wName = "WEAPON";
        switch (mPlayerPtr->getCurrentWeaponType()) {
        case WeaponType::SWORD_DEFAULT: wName = "VOID BLADE"; break;
        case WeaponType::DAGGER_SWIFT: wName = "SHADOW DAGGER"; break;
        case WeaponType::AXE_HEAVY: wName = "ABYSS AXE"; break;
        case WeaponType::SPEAR_LONG: wName = "PIERCER"; break;
        case WeaponType::KATANA_BLOOD: wName = "BLOOD KATANA"; break;
        }
        DrawText(wName, px + 10, py + 10, 20, wc);

        std::string sName = "SAVE: ???";
        if (currentSavePath.find("main") != std::string::npos) sName = "SAVE: 1";
        else if (currentSavePath.find("secondary") != std::string::npos) sName = "SAVE: 2";
        else if (currentSavePath.find("tertiary") != std::string::npos) sName = "SAVE: 3";
        DrawText(sName.c_str(), px + 150, py + 12, 10, GRAY);

        DrawText(TextFormat("DMG: %.0f  RNG: %.0f", ws.damage, ws.reachX), px + 10, py + 35, 14, textNormal);
        DrawText(TextFormat("SPD: %.2fs", ws.attackCooldown), px + 10, py + 55, 14, textAccent);

        for (int i = 0; i < 5; i++) {
            bool sel = ((int)mPlayerPtr->getCurrentWeaponType() == i);
            WeaponStats s = WeaponStats::GetStats((WeaponType)i);
            DrawRectangle(px + 120, py + 35 + i * 12, 10, 10, sel ? s.slashColor : Fade(s.slashColor, 0.3f));
            DrawText(TextFormat("%d", i + 1), px + 135, py + 35 + i * 12, 10, sel ? WHITE : GRAY);
        }

        int sy = py + 110;
        DrawRectangle(px, sy, 220, 110, panelBg);
        DrawRectangleLines(px, sy, 220, 110, panelBorder);
        DrawText("VITALITY", px + 10, sy + 10, 10, GRAY);

        float hpPct = (float)mPlayerPtr->mHealth / (float)mPlayerPtr->mMaxHealth;
        DrawRectangle(px + 10, sy + 25, 140, 12, Fade(BLACK, 0.5f));
        DrawRectangle(px + 10, sy + 25, (int)(140 * hpPct), 12, mCheats.godMode ? GOLD : hpColor);
        DrawRectangleLines(px + 10, sy + 25, 140, 12, Fade(WHITE, 0.2f));
        DrawText(TextFormat("%d/%d", mPlayerPtr->mHealth, mPlayerPtr->mMaxHealth), px + 160, sy + 25, 10, WHITE);

        if (mPlayerPtr->mInvincibilityTimer > 0) DrawText("GHOST FORM", px + 10, sy + 45, 10, GOLD);

        DrawText("AGILITY", px + 10, sy + 70, 10, GRAY);
        int jumps = mPlayerPtr->getMaxJumps() - mPlayerPtr->getJumpCount();
        for (int i = 0; i < mPlayerPtr->getMaxJumps(); i++) DrawRectangle(px + 10 + i * 15, sy + 85, 10, 10, (i < jumps) ? SKYBLUE : DARKBLUE);
        DrawText(mPlayerPtr->canDash() ? "DASH READY" : "RECHARGE", px + 60, sy + 85, 10, mPlayerPtr->canDash() ? LIME : MAROON);
    }

    for (auto* b : mBosses) {
        if (b && b->mActive && b->mState != MageBoss::INACTIVE) {
            int bx = windowWidth / 2 - 200, by = 40;
            DrawRectangle(bx - 2, by - 2, 404, 14, BLACK);
            DrawRectangle(bx, by, 400, 10, DARKGRAY);
            DrawRectangle(bx, by, (int)(400 * ((float)b->mHealth / b->mMaxHealth)), 10, b->mIsEnraged ? RED : PURPLE);
            DrawText("ABYSS MAGE", bx, by - 20, 20, textNormal);
            if (b->mIsEnraged) DrawText("FRENZY", bx + 410, by - 2, 12, RED);
            break;
        }
    }

    int cx = 20, cy = windowHeight - 160;
    DrawRectangle(cx, cy, 180, 140, panelBg);
    DrawRectangleLines(cx, cy, 180, 140, panelBorder);
    int ty = cy + 10;
    DrawText("CONTROLS", cx + 10, ty, 10, GRAY); ty += 20;
    DrawText("[ARROWS]", cx + 10, ty, 10, YELLOW); DrawText("Move", cx + 70, ty, 10, textNormal); ty += 18;
    DrawText("[SPACE]", cx + 10, ty, 10, YELLOW); DrawText("Jump", cx + 70, ty, 10, textNormal); ty += 18;
    DrawText("[C]", cx + 10, ty, 10, YELLOW); DrawText("Dash", cx + 70, ty, 10, textNormal); ty += 18;
    DrawText("[Z]", cx + 10, ty, 10, YELLOW); DrawText("Attack", cx + 70, ty, 10, textNormal); ty += 18;
    DrawText("[F3]", cx + 10, ty, 10, YELLOW); DrawText("Debug", cx + 70, ty, 10, textNormal);

    if (mCheats.hasActiveMessage()) {
        const char* txt = mCheats.message.c_str();
        DrawText(txt, (windowWidth - MeasureText(txt, 24)) / 2, windowHeight / 2 - 50, 24, Fade(YELLOW, std::min(mCheats.messageTimer, 1.0f)));
    }

    if (mShowDebug) {
        int dx = 20, dy = 20;
        DrawRectangle(dx, dy, 180, 280, Fade(BLACK, 0.9f));
        DrawRectangleLines(dx, dy, 180, 280, LIME);
        int dty = dy + 10;
        DrawText(TextFormat("FPS: %d", GetFPS()), dx + 10, dty, 10, LIME); dty += 15;
        DrawText(TextFormat("ENTITIES: %d", (int)mActiveEntities.size()), dx + 10, dty, 10, WHITE); dty += 15;
        if (mPlayerPtr) DrawText(TextFormat("POS: %.0f, %.0f", mPlayerPtr->mPosition.x, mPlayerPtr->mPosition.y), dx + 10, dty, 10, WHITE); dty += 30;

        DrawText("CHEATS:", dx + 10, dty, 10, YELLOW); dty += 15;
        DrawText("GOD:", dx + 10, dty, 10, WHITE); DrawText(mCheats.godMode ? "ON" : "OFF", dx + 60, dty, 10, mCheats.godMode ? GOLD : RED); dty += 15;
        DrawText("NOCLIP:", dx + 10, dty, 10, WHITE); DrawText(mCheats.noclip ? "ON" : "OFF", dx + 60, dty, 10, mCheats.noclip ? SKYBLUE : RED); dty += 30;

        DrawText("KEYS:", dx + 10, dty, 10, YELLOW); dty += 15;
        DrawText("[G] God Mode", dx + 10, dty, 10, GRAY); dty += 15;
        DrawText("[N] Noclip", dx + 10, dty, 10, GRAY); dty += 15;
        DrawText("[H] Heal", dx + 10, dty, 10, GRAY); dty += 15;
        DrawText("[T] TP Boss", dx + 10, dty, 10, GRAY); dty += 15;
        DrawText("[K] Kill Boss", dx + 10, dty, 10, GRAY); dty += 15;
        DrawText("[R] Respawn", dx + 10, dty, 10, GRAY); dty += 15;
        DrawText("[F1] Reset", dx + 10, dty, 10, GRAY);
    }

    EndDrawing();
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
            boss->setArenaBounds(spawn.arenaBounds.x, spawn.arenaBounds.y, spawn.arenaBounds.width, spawn.arenaBounds.height);
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