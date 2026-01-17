#pragma once

#include "raylib.h"
#include "GameConstants.hpp"
#include "../Map/map.hpp"

#include <vector>
#include <string>
#include <memory>
#include <cmath>
#include <algorithm>

class Entity;
class Player;
class MageBoss;
class RabbitEnemy;
class Saves;

struct GameMap {
    int width = 0;
    int height = 0;
    int minX = 0;
    int minY = 0;
    std::vector<int> tiles;

    void init(const std::vector<struct RenderTile>& rawTiles);
    int getTileAt(int gridX, int gridY) const;
};

struct SpatialGrid {
    std::vector<std::vector<Entity*>> cells;
    int cellSize = 64;
    int width = 0;
    int height = 0;
    int offsetX = 0;
    int offsetY = 0;

    void init(int mapW, int mapH, int minX, int minY);
    void clear();
    void insertStatic(Entity* e);
    void getNearby(float x, float y, std::vector<Entity*>& outResult) const;
};

struct SmoothCamera {
    Vector2 currentPos = { 0, 0 };
    float smoothSpeed = 12.0f;
    float deadZoneX = 15.0f;
    float deadZoneY = 5.0f;

    void update(Vector2 target, float dt);
    Vector2 getPosition() const;
    void setPosition(Vector2 pos);
};

struct BossSpawnData {
    Vector2 position;
    Rectangle arenaBounds;
    int maxHealth;
};

struct CheatSystem {
    bool godMode = false;
    bool noclip = false;
    float messageTimer = 0.0f;
    std::string message = "";

    void showMessage(const std::string& msg);
    void update(float dt);
    bool hasActiveMessage() const;
};

struct WeaponUIState {
    float changeTimer = 0.0f;
    static constexpr float CHANGE_ANIM_TIME = 0.3f;

    void triggerChange();
    void update(float dt);
    bool isAnimating() const;
};

class GameWrapper {
public:
    GameWrapper(bool& shouldQuit, int& state);
    ~GameWrapper();

    void runFrame(int windowWidth, int windowHeight);
    void reset();
    void reloadSave();

private:
    bool& mShouldQuit;
    int& mState;

    Texture2D mTileset;
    Texture2D mPlayerTexture;
    Texture2D mBossTexture;
    Texture2D mRabbitTexture;
    Texture2D mBackground;

    RenderTexture2D mRenderTarget;
    Camera2D mCamera;

    GameMap mGameMap;
    SpatialGrid mCollisionGrid;
    SmoothCamera mSmoothCamera;
    CheatSystem mCheats;
    WeaponUIState mWeaponUI;

    std::vector<std::unique_ptr<Entity>> mActiveEntities;
    std::vector<Entity*> mWallEntities;
    Player* mPlayerPtr = nullptr;
    std::vector<MageBoss*> mBosses;
    std::vector<RabbitEnemy*> mRabbits;
    std::vector<BossSpawnData> mBossSpawnPoints;

    std::vector<Entity*> mNearbyObstacles;
    std::vector<CollisionRect> mLiquidRects;

    Vector2 mPlayerSpawnPoint = { 464.5f, 442.0f };
    bool mShowDebug = false;
    bool mInitialized = false;

    void loadTextures();
    void unloadTextures();
    void loadMap();
    void loadEntities();
    void createTestEnemies();

    void handleInput(float dt);
    void updateEntities(float dt);
    void updateCollisions();
    void updateLiquids(float dt);
    void updateCombat();
    void updateCamera(float dt);

    void drawWorld();
    void drawUI(int windowWidth, int windowHeight);

    void respawnAllBosses();
    void respawnPlayer();

    void saveGame();
};