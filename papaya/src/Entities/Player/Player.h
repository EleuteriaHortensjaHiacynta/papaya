#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Entity.h"
#include "PlayerTypes.h"
#include "PlayerConstants.h"

class Player : public Entity {
public:
    Player(float startX, float startY);
    ~Player();

    void update(float deltaTime) override;
    void draw() override;
    Rectangle getRect() override;
    void onCollision(Entity* other) override;

    void lateUpdate(float deltaTime);
    bool hasHit(Entity* enemy) const;
    int getAttackDamage() const;
    WeaponStats getCurrentWeaponStats() const;
    void setWeapon(WeaponType type);

    Vector2 mVelocity = { 0, 0 };
    Vector2 mPrevPosition = { 0, 0 };
    bool mIsAttacking = false;
    Rectangle mAttackArea = { 0, 0, 0, 0 };
    std::vector<Entity*> mHitEntities;
    float mInvincibilityTimer = 0.0f;
    int mMaxHealth = 5;
    int mHealth = 5;

private:
    Vector2 mStartPosition;

    bool mWasGrounded = false;
    bool mWasTouchingWall = false;
    int mPrevWallDir = 0;

    bool mIsGrounded = false;
    bool mIsFacingRight = true;
    bool mIsJumping = false;
    bool mIsPogoJump = false;
    bool mIsClimbing = false;
    bool mIsDashing = false;
    bool mCanDash = true;
    bool mTouchingWall = false;
    int mWallDir = 0;

    int mJumpCount = 0;
    float mJumpBufferCounter = 0.0f;
    float mCoyoteTimeCounter = 0.0f;

    float mDashTimer = 0.0f;
    float mDashCooldownTimer = 0.0f;
    float mMomentumTimer = 0.0f;

    float mStamina = PlayerConstants::MAX_STAMINA;
    float mStaminaRegenDelay = 0.0f;

    WeaponType mCurrentWeapon = WeaponType::SWORD_DEFAULT;
    float mAttackTimer = 0.0f;
    float mAttackCooldown = 0.0f;
    AttackDirection mAttackDir = AttackDirection::HORIZONTAL;
    int mComboCount = 0;
    float mComboWindowTimer = 0.0f;
    bool mComboWindowOpen = false;
    float mRecoilTimer = 0.0f;

    AnimState mCurrentState = AnimState::IDLE;
    Rectangle mFrameRec = { 0, 0, 16, 16 };
    int mCurrentFrameRelative = 0;
    float mFrameTimer = 0.0f;
    AnimationData mCurrentAnim;

    Texture2D mSlashTexture;
    Texture2D mWingsTexture;

    std::vector<Ghost> mGhosts;
    float mGhostSpawnTimer = 0.0f;
    bool mWingsActive = false;
    int mWingsFrame = 0;
    float mWingsTimer = 0.0f;

    void respawn();
    inline void saveFrameState();
    inline void resetFrameState();

    void handleMovement(float dt);
    void handleJump(float dt);
    void handleDash(float dt);
    void handleClimbing(float dt);
    void applyPhysics(float dt);
    void handleWallCollision(Rectangle wallRect);

    void handleAttack(float dt);
    void createAttackHitbox();

    void updateTimers(float dt);
    inline void updateStamina(float dt);

    void updateAnimation(float dt);
    inline void updateWings(float dt);
    void handleGhosts(float dt);
    void determineAnimationState();

    void drawGhosts();
    void drawWings();
    void drawSlash();
    void drawPlayer();
    void drawUI();
};