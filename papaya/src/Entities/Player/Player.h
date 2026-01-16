#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Entity.h"
#include "PlayerTypes.h"
#include "PlayerConstants.h"

class Player : public Entity {
public:
    // === KONSTRUKTOR/DESTRUKTOR ===
    Player(float startX, float startY);
    ~Player();

    // === INTERFACE ENTITY ===
    void update(float deltaTime) override;
    void draw() override;
    Rectangle getRect() override;
    void onCollision(Entity* other) override;

    // === METODY PUBLICZNE ===
    void lateUpdate(float deltaTime);
    bool hasHit(Entity* enemy) const;
    int getAttackDamage() const;
    WeaponStats getCurrentWeaponStats() const;

    // === STAN PUBLICZNY (do odczytu przez inne systemy) ===
    Vector2 mVelocity = { 0, 0 };
    Vector2 mPrevPosition = { 0, 0 };

    bool mIsAttacking = false;
    Rectangle mAttackArea = { 0, 0, 0, 0 };
    std::vector<Entity*> mHitEntities;

    float mInvincibilityTimer = 0.0f;

    // === ZDROWIE ===
    int mMaxHealth = 20;
    int mHealth = 20;

private:
    // === POZYCJA I FIZYKA ===
    Vector2 mStartPosition;

    // === STANY POPRZEDNIEJ KLATKI ===
    bool mWasGrounded = false;
    bool mWasTouchingWall = false;
    int mPrevWallDir = 0;

    // === STANY BIEŻĄCE ===
    bool mIsGrounded = false;
    bool mIsFacingRight = true;
    bool mIsJumping = false;
    bool mIsPogoJump = false;
    bool mIsClimbing = false;
    bool mIsDashing = false;
    bool mCanDash = true;
    bool mTouchingWall = false;
    int mWallDir = 0;

    // === SKOK ===
    int mJumpCount = 0;
    float mJumpBufferCounter = 0.0f;
    float mCoyoteTimeCounter = 0.0f;

    // === DASH ===
    float mDashTimer = 0.0f;
    float mDashCooldownTimer = 0.0f;
    float mMomentumTimer = 0.0f;

    // === WSPINACZKA ===
    float mStamina = PlayerConstants::MAX_STAMINA;
    float mStaminaRegenDelay = 0.0f;

    // === WALKA ===
    WeaponType mCurrentWeapon = WeaponType::SWORD_DEFAULT;
    float mAttackTimer = 0.0f;
    AttackDirection mAttackDir = AttackDirection::HORIZONTAL;
    int mComboCount = 0;
    float mComboWindowTimer = 0.0f;
    bool mComboWindowOpen = false;
    float mRecoilTimer = 0.0f;

    // === ANIMACJA ===
    AnimState mCurrentState = AnimState::IDLE;
    Rectangle mFrameRec = { 0, 0, 16, 16 };
    int mCurrentFrameRelative = 0;
    float mFrameTimer = 0.0f;
    AnimationData mCurrentAnim;

    // === TEKSTURY ===
    Texture2D mSlashTexture;
    Texture2D mWingsTexture;

    // === EFEKTY WIZUALNE ===
    std::vector<Ghost> mGhosts;
    float mGhostSpawnTimer = 0.0f;
    bool mWingsActive = false;
    int mWingsFrame = 0;
    float mWingsTimer = 0.0f;

    // === METODY PRYWATNE - GŁÓWNE SYSTEMY ===
    void respawn();
    void saveFrameState();
    void resetFrameState();

    // === FIZYKA I RUCH ===
    void handleMovement(float dt);
    void handleJump(float dt);
    void handleDash(float dt);
    void handleClimbing(float dt);
    void applyPhysics(float dt);
    void handleWallCollision(Rectangle wallRect);

    // === WALKA ===
    void handleAttack(float dt);
    void handleSacrifice();
    void createAttackHitbox();

    // === TIMERY ===
    void updateTimers(float dt);
    void updateStamina(float dt);

    // === ANIMACJA I GRAFIKA ===
    void updateAnimation(float dt);
    void updateWings(float dt);
    void handleGhosts(float dt);
    void determineAnimationState();

    // === RENDERING ===
    void drawGhosts();
    void drawWings();
    void drawSlash();
    void drawPlayer();
    void drawUI();
};