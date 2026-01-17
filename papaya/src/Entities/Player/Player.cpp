#include "Player.h"
#include "Core/InputHelper.h"
#include "Core/MathUtils.h"
#include <algorithm>
#include <cmath>

using namespace PlayerConstants;
using namespace MathUtils;

Player::Player(float startX, float startY)
    : Entity({ startX, startY }, PLAYER)
    , mStartPosition{ startX, startY }
{
    mPosition = { startX, startY };
    mPrevPosition = mPosition;
    mSize = { HITBOX_WIDTH, HITBOX_HEIGHT };

    mSlashTexture = LoadTexture("assets/slash.png");
    mWingsTexture = LoadTexture("assets/jump_wings.png");

    mCurrentAnim = AnimationData::GetAnimationData(AnimState::IDLE);
}

Player::~Player() {
    UnloadTexture(mSlashTexture);
    UnloadTexture(mWingsTexture);
}

void Player::update(float deltaTime) {
    if (mHealth <= 0 || mPosition.y > 800.0f) {
        respawn();
        return;
    }

    saveFrameState();
    resetFrameState();

    updateTimers(deltaTime);
    handleDash(deltaTime);
    handleMovement(deltaTime);
    handleJump(deltaTime);
    handleAttack(deltaTime);
    applyPhysics(deltaTime);

    updateStamina(deltaTime);
    handleGhosts(deltaTime);
    updateWings(deltaTime);
}

void Player::lateUpdate(float deltaTime) {
    handleClimbing(deltaTime);
    updateAnimation(deltaTime);
}

Rectangle Player::getRect() {
    return { mPosition.x, mPosition.y, mSize.x, mSize.y };
}

void Player::onCollision(Entity* other) {
    if (other->mType == WALL) {
        handleWallCollision(other->getRect());
    }
}

void Player::draw() {
    drawGhosts();
    drawWings();
    drawSlash();
    drawPlayer();
    drawUI();
}

bool Player::hasHit(Entity* enemy) const {
    return std::find(mHitEntities.begin(), mHitEntities.end(), enemy) != mHitEntities.end();
}

int Player::getAttackDamage() const {
    WeaponStats stats = getCurrentWeaponStats();
    return (int)(stats.damage * (mComboCount >= 2 ? 1.5f : 1.0f));
}

WeaponStats Player::getCurrentWeaponStats() const {
    return WeaponStats::GetStats(mCurrentWeapon);
}

void Player::setWeapon(WeaponType type) {
    mCurrentWeapon = type;
    mIsAttacking = false;
    mAttackTimer = 0.0f;
    mAttackCooldown = 0.0f;
    mComboCount = 0;
    mComboWindowOpen = false;
}

void Player::pogoBounce() {
    mVelocity.y = -JUMP_FORCE * 0.9f;

    mIsPogoJump = true;
    mJumpCount = 1;
    mCanDash = true;
    mIsDashing = false;
}

inline void Player::saveFrameState() {
    mPrevPosition = mPosition;
    mWasTouchingWall = mTouchingWall;
    mPrevWallDir = mWallDir;
    mWasGrounded = mIsGrounded;
}

inline void Player::resetFrameState() {
    mTouchingWall = false;
    mWallDir = 0;
}

void Player::respawn() {
    mPosition = mStartPosition;
    mHealth = mMaxHealth;
    mVelocity = { 0, 0 };
    mStamina = MAX_STAMINA;

    mRecoilTimer = 0;
    mInvincibilityTimer = 0;
    mIsDashing = false;
    mIsAttacking = false;
    mAttackTimer = 0.0f;
    mAttackCooldown = 0.0f;
    mComboCount = 0;
    mComboWindowOpen = false;
    mCurrentState = AnimState::IDLE;
    mTouchingWall = false;
    mWallDir = 0;
    mIsClimbing = false;
    mWasGrounded = false;
    mWasTouchingWall = false;
}

void Player::updateTimers(float dt) {
    mJumpBufferCounter = Input::JumpPressed() ? JUMP_BUFFER_TIME : mJumpBufferCounter - dt;
    mCoyoteTimeCounter = mIsGrounded ? COYOTE_TIME : mCoyoteTimeCounter - dt;

    if (mVelocity.y > 0) mIsPogoJump = false;
    if (mDashCooldownTimer > 0) mDashCooldownTimer -= dt;
    if (mInvincibilityTimer > 0) mInvincibilityTimer -= dt;
    if (mMomentumTimer > 0) mMomentumTimer -= dt;
    if (mAttackCooldown > 0) mAttackCooldown -= dt;

    if (mRecoilTimer > 0) {
        mRecoilTimer -= dt;
        float absVelX = std::abs(mVelocity.x);
        if (absVelX > MAX_SPEED)
            mVelocity.x = Sign(mVelocity.x) * MAX_SPEED;
    }

    if (mComboWindowOpen) {
        mComboWindowTimer -= dt;
        if (mComboWindowTimer <= 0) {
            mComboWindowOpen = false;
            mComboCount = 0;
        }
    }

    if (mIsAttacking) {
        mAttackTimer -= dt;
        if (mAttackTimer <= 0) {
            mIsAttacking = false;
            mCurrentState = AnimState::IDLE;
            mAttackArea = { 0, 0, 0, 0 };
            mHitEntities.clear();

            if (mComboCount < MAX_COMBO) {
                mComboWindowOpen = true;
                mComboWindowTimer = COMBO_WINDOW;
            }
            else {
                mComboCount = 0;
                mComboWindowOpen = false;
            }
        }
    }
}

inline void Player::updateStamina(float dt) {
    if (mIsClimbing) return;

    if (mStaminaRegenDelay > 0)
        mStaminaRegenDelay -= dt;
    else if (mStamina < MAX_STAMINA)
        mStamina = std::min(mStamina + STAMINA_REGEN_SPEED * dt, MAX_STAMINA);
}

void Player::handleMovement(float dt) {
    // --- POPRAWKA NOCLIP: LATANIE ---
    if (mNoclip) {
        float dirX = Input::GetHorizontalAxis();
        float dirY = Input::GetVerticalAxis();
        float flySpeed = 300.0f;

        mVelocity.x = dirX * flySpeed;
        mVelocity.y = dirY * flySpeed;

        if (dirX != 0) mIsFacingRight = (dirX > 0);
        return;
    }
    // --------------------------------

    if (mIsDashing || mIsClimbing) return;

    float moveDir = Input::GetHorizontalAxis();
    bool isSuperSpeed = std::abs(mVelocity.x) > MAX_SPEED;

    if (mIsGrounded) {
        if (isSuperSpeed) {
            float drag = MOMENTUM_DRAG_GROUND;
            if (Sign(mVelocity.x) == moveDir && moveDir != 0) drag *= 0.5f;
            mVelocity.x = Approach(mVelocity.x, moveDir * MAX_SPEED, drag * dt);
        }
        else {
            if (moveDir != 0)
                mVelocity.x = Approach(mVelocity.x, moveDir * MAX_SPEED, GROUND_ACCEL * dt);
            else
                mVelocity.x = Approach(mVelocity.x, 0, GROUND_FRICTION * dt);
        }
    }
    else {
        if (isSuperSpeed) {
            mVelocity.x = Approach(mVelocity.x, moveDir * MAX_SPEED, AIR_DRAG * dt);
        }
        else {
            if (moveDir != 0) {
                bool isTurning = Sign(mVelocity.x) != 0 && Sign(mVelocity.x) != moveDir;
                float accel = isTurning ? AIR_TURN_ACCEL : AIR_ACCEL;
                mVelocity.x = Approach(mVelocity.x, moveDir * MAX_SPEED, accel * dt);
            }
            else {
                mVelocity.x = Approach(mVelocity.x, 0, AIR_DRAG * dt);
            }
        }
    }
}

void Player::handleJump(float dt) {
    bool wantsJump = Input::JumpPressed() || mJumpBufferCounter > 0;
    if (!wantsJump) {
        if (!Input::JumpHeld() && mVelocity.y < -50.0f && mIsJumping && !mIsPogoJump) {
            mVelocity.y = -50.0f;
        }
        return;
    }

    bool jumped = false;

    if (mWasTouchingWall && !mIsGrounded) {
        mVelocity.y = -JUMP_FORCE;
        mVelocity.x = -mPrevWallDir * WALL_JUMP_KICK;
        mIsFacingRight = (mPrevWallDir == -1);
        mIsClimbing = false;
        mJumpCount = 1;
        jumped = true;
    }
    else if (mCoyoteTimeCounter > 0) {
        mVelocity.y = -JUMP_FORCE;
        mJumpCount = 1;
        jumped = true;
    }
    else if (mJumpCount < MAX_JUMPS) {
        mVelocity.y = -JUMP_FORCE;
        mJumpCount++;
        mMomentumTimer = 0.0f;
        mWingsActive = true;
        mWingsFrame = 0;
        mWingsTimer = 0.0f;
        jumped = true;
    }

    if (jumped) {
        mCoyoteTimeCounter = 0;
        mJumpBufferCounter = 0;
        mIsGrounded = false;
        mIsJumping = true;
        mIsPogoJump = false;
        mIsDashing = false;
        if (mMomentumTimer > 0) mMomentumTimer = 0.6f;
    }
}

void Player::handleDash(float dt) {
    if (Input::DashPressed() && mCanDash && !mIsDashing && mDashCooldownTimer <= 0) {
        mIsDashing = true;
        mCanDash = false;
        mDashTimer = DASH_DURATION;
        mDashCooldownTimer = DASH_COOLDOWN;
        mCurrentState = AnimState::DASH;
        mIsJumping = false;

        float dirX = Input::GetHorizontalAxis();
        float dirY = Input::GetVerticalAxis();

        if (dirX == 0.0f && dirY == 0.0f)
            dirX = mIsFacingRight ? 1.0f : -1.0f;

        if (dirX != 0.0f && dirY != 0.0f) {
            constexpr float DIAGONAL = 0.7071f;
            dirX *= DIAGONAL;
            dirY *= DIAGONAL;
        }

        mVelocity.x = dirX * DASH_SPEED;
        mVelocity.y = dirY * DASH_SPEED;
    }

    if (mIsDashing) {
        mDashTimer -= dt;
        if (mDashTimer <= 0) {
            mIsDashing = false;
            if (std::abs(mVelocity.y) > MAX_SPEED)
                mVelocity.y *= 0.6f;
        }
    }
}

void Player::handleClimbing(float dt) {
    if (!mTouchingWall || mStamina <= 0 || mVelocity.y < -100.0f) {
        mIsClimbing = false;
        return;
    }

    bool hasClimbInput = Input::IsMovingUp() || Input::IsMovingDown();
    bool isGripping = (mWallDir == 1 && Input::IsMovingRight()) ||
        (mWallDir == -1 && Input::IsMovingLeft());

    if (mIsGrounded && !Input::IsMovingUp()) {
        mIsClimbing = false;
        return;
    }

    if (hasClimbInput && isGripping) {
        mCurrentState = AnimState::CLIMB;
        mIsClimbing = true;

        if (mWallDir != 0) mIsFacingRight = (mWallDir == 1);

        float climbDir = Input::GetVerticalAxis();
        mStamina -= COST_CLIMB * dt;
        mStaminaRegenDelay = STAMINA_DELAY_TIME;
        mVelocity.y = climbDir * CLIMB_SPEED;
        mVelocity.x = mWallDir * 100.0f;
    }
    else if (!mIsGrounded && isGripping) {
        mStamina -= (COST_CLIMB * 0.5f) * dt;
        mStaminaRegenDelay = STAMINA_DELAY_TIME;
        mCurrentState = AnimState::WALL_SLIDE;
        mIsClimbing = true;

        if (mWallDir != 0) mIsFacingRight = (mWallDir == 1);

        mVelocity.y = WALL_SLIDE_SPEED;
        mVelocity.x = mWallDir * 100.0f;
    }
    else {
        mIsClimbing = false;
    }
}

void Player::applyPhysics(float dt) {
    if (!mIsDashing && !mIsClimbing && !mNoclip)
        mVelocity.y += GRAVITY * dt;

    mIsGrounded = false;
    mPosition.x += mVelocity.x * dt;
    mPosition.y += mVelocity.y * dt;
}

void Player::handleWallCollision(Rectangle wall) {
    float prevBottom = mPrevPosition.y + mSize.y;
    constexpr float THRESHOLD = 4.0f;

    if (prevBottom <= wall.y + THRESHOLD) {
        mPosition.y = wall.y - mSize.y;
        mVelocity.y = 0;
        mIsGrounded = true;
        mCanDash = true;
        mIsJumping = false;
        mJumpCount = 0;

        if (mIsDashing) {
            mIsDashing = false;
            float dashDir = Sign(mVelocity.x);
            if (dashDir == 0)
                dashDir = Input::IsMovingRight() ? 1.0f :
                (Input::IsMovingLeft() ? -1.0f :
                    (mIsFacingRight ? 1.0f : -1.0f));
            mVelocity.x = dashDir * DASH_SPEED * WAVE_DASH_BOOST;
        }
        return;
    }

    if (mPrevPosition.y >= wall.y + wall.height - THRESHOLD) {
        float playerCenter = mPosition.x + mSize.x / 2.0f;
        float wallCenter = wall.x + wall.width / 2.0f;
        float halfW = (mSize.x + wall.width) / 2.0f;
        float overlapX = halfW - std::abs(playerCenter - wallCenter);

        if (overlapX > 5.0f) {
            mPosition.y = wall.y + wall.height;
            mVelocity.y = 0;
            return;
        }
    }

    float playerCenterX = mPrevPosition.x + mSize.x / 2.0f;
    float wallCenterX = wall.x + wall.width / 2.0f;
    float playerBottom = mPosition.y + mSize.y;
    float stepHeight = playerBottom - wall.y;

    bool movingTowardsWall = (playerCenterX < wallCenterX && Input::IsMovingRight()) ||
        (playerCenterX > wallCenterX && Input::IsMovingLeft());

    constexpr float MAX_STEP_HEIGHT = 12.0f;

    bool canAutoStep = (mWasGrounded || mIsGrounded) &&
        stepHeight > 0.0f &&
        stepHeight <= MAX_STEP_HEIGHT &&
        movingTowardsWall &&
        mVelocity.y >= -50.0f &&
        mVelocity.y <= 150.0f &&
        !mIsDashing &&
        !mIsClimbing &&
        std::abs(mVelocity.x) > 0.1f;

    if (canAutoStep) {
        mPosition.y = wall.y - mSize.y;
        mIsGrounded = true;
        mVelocity.y = 0;
        mCanDash = true;
        mJumpCount = 0;
    }
    else {
        if (playerCenterX < wallCenterX) {
            mPosition.x = wall.x - mSize.x;
            mWallDir = 1;
        }
        else {
            mPosition.x = wall.x + wall.width;
            mWallDir = -1;
        }
        mTouchingWall = true;
        mVelocity.x = 0;
    }
}

void Player::handleAttack(float dt) {
    if (!Input::AttackPressed() || mIsAttacking || mAttackCooldown > 0.0f)
        return;

    if (mComboWindowOpen)
        mComboCount = std::min(mComboCount + 1, MAX_COMBO);
    else
        mComboCount = 0;

    mComboWindowOpen = false;

    WeaponStats stats = getCurrentWeaponStats();
    mIsAttacking = true;
    mCurrentState = AnimState::ATTACK;

    float durationMult = (mComboCount == 0) ? 1.0f :
        (mComboCount == 1) ? 0.85f : 1.2f;
    mAttackTimer = stats.duration * durationMult;

    float cooldownMult = (mComboCount < MAX_COMBO) ? 0.8f : 1.2f;
    mAttackCooldown = stats.attackCooldown * cooldownMult;

    createAttackHitbox();

    if (mAttackDir == AttackDirection::HORIZONTAL && mIsGrounded) {
        float pushForce = (mComboCount == 2) ? 50.0f : 25.0f;
        mVelocity.x += mIsFacingRight ? pushForce : -pushForce;
    }
}

void Player::createAttackHitbox() {
    WeaponStats stats = getCurrentWeaponStats();
    float reachX = stats.reachX;
    float reachY = stats.reachY;

    if (mComboCount == 2) {
        reachX *= 1.4f;
        reachY *= 1.3f;
    }

    if (Input::IsMovingUp()) {
        mAttackDir = AttackDirection::UP;
        mAttackArea = {
            mPosition.x - (reachY - mSize.x) / 2,
            mPosition.y - reachX,
            reachY,
            reachX
        };
    }
    else if (Input::IsMovingDown() && !mIsGrounded) {
        mAttackDir = AttackDirection::DOWN;
        mAttackArea = {
            mPosition.x - (reachY - mSize.x) / 2,
            mPosition.y + mSize.y,
            reachY,
            reachX
        };
    }
    else {
        mAttackDir = AttackDirection::HORIZONTAL;
        float yOffset = (mSize.y - reachY) / 2;

        if (mIsFacingRight) {
            mAttackArea = { mPosition.x + mSize.x, mPosition.y + yOffset, reachX, reachY };
        }
        else {
            mAttackArea = { mPosition.x - reachX, mPosition.y + yOffset, reachX, reachY };
        }
    }
}

void Player::determineAnimationState() {
    float moveInput = Input::GetHorizontalAxis();
    bool isMoving = std::abs(mVelocity.x) > 1.0f || moveInput != 0;

    if (!mIsAttacking && moveInput != 0) {
        bool wantsRight = moveInput > 0;
        if (mIsFacingRight != wantsRight) {
            mIsFacingRight = wantsRight;
            if (mIsGrounded && mCurrentState != AnimState::DASH) {
                mCurrentState = AnimState::TURN;
                mCurrentFrameRelative = 0;
                mFrameTimer = 0.0f;
                return;
            }
        }
    }

    AnimState newState = mCurrentState;
    bool isTurning = (mCurrentState == AnimState::TURN &&
        mCurrentFrameRelative < mCurrentAnim.frameCount - 1);

    if (mIsAttacking)
        newState = AnimState::ATTACK;
    else if (mIsDashing)
        newState = AnimState::DASH;
    else if (mIsClimbing)
        return;
    else if (!mIsGrounded)
        newState = (mVelocity.y < -50.0f) ? AnimState::JUMP_UP : AnimState::JUMP_DOWN;
    else if (!isTurning)
        newState = isMoving ? AnimState::WALK : AnimState::IDLE;

    if (mCurrentState != newState) {
        mCurrentState = newState;
        mCurrentFrameRelative = 0;
        mFrameTimer = 0.0f;
    }
}

void Player::updateAnimation(float dt) {
    determineAnimationState();
    mCurrentAnim = AnimationData::GetAnimationData(mCurrentState, mVelocity.y, mAttackDir);
    mFrameTimer += dt;

    if (mFrameTimer >= mCurrentAnim.frameSpeed) {
        mFrameTimer = 0.0f;
        mCurrentFrameRelative++;

        if (mCurrentFrameRelative >= mCurrentAnim.frameCount) {
            if (mCurrentAnim.loop) {
                mCurrentFrameRelative = 0;
            }
            else {
                mCurrentFrameRelative = mCurrentAnim.frameCount - 1;
                if (mCurrentState == AnimState::TURN) {
                    float moveInput = Input::GetHorizontalAxis();
                    mCurrentState = (moveInput != 0 || std::abs(mVelocity.x) > 1.0f) ?
                        AnimState::WALK : AnimState::IDLE;
                    mCurrentFrameRelative = 0;
                }
            }
        }
    }

    mFrameRec.x = (float)(mCurrentAnim.startFrame + mCurrentFrameRelative) * FRAME_WIDTH;
    mFrameRec.y = 0.0f;
    mFrameRec.width = FRAME_WIDTH;
    mFrameRec.height = FRAME_HEIGHT;
}

inline void Player::updateWings(float dt) {
    if (!mWingsActive) return;

    mWingsTimer += dt;
    if (mWingsTimer >= WINGS_SPEED) {
        mWingsTimer = 0.0f;
        mWingsFrame++;
        if (mWingsFrame >= WINGS_NUM_FRAMES) {
            mWingsActive = false;
            mWingsFrame = 0;
        }
    }
}

void Player::handleGhosts(float dt) {
    float speedThreshold = MAX_SPEED * WAVE_DASH_BOOST;
    bool shouldSpawn = mIsDashing || (std::abs(mVelocity.x) > speedThreshold && mRecoilTimer <= 0);

    if (shouldSpawn) {
        mGhostSpawnTimer -= dt;
        if (mGhostSpawnTimer <= 0) {
            mGhostSpawnTimer = GHOST_SPAWN_INTERVAL;

            float offsetX = (FRAME_WIDTH - HITBOX_WIDTH) / 2.0f;
            float offsetY = (FRAME_HEIGHT - HITBOX_HEIGHT) / 2.0f;

            Vector2 visualPos = {
                mPosition.x - offsetX,
                mPosition.y - offsetY
            };

            if (mCurrentState == AnimState::CLIMB || mCurrentState == AnimState::WALL_SLIDE) {
                float climbOffset = 5.0f;
                visualPos.x += mIsFacingRight ? climbOffset : -climbOffset;
            }

            mGhosts.push_back({ visualPos, mFrameRec, mIsFacingRight, 0.5f });
        }
    }
    else {
        mGhostSpawnTimer = 0;
    }

    for (int i = (int)mGhosts.size() - 1; i >= 0; i--) {
        mGhosts[i].alpha -= GHOST_FADE_SPEED * dt;
        if (mGhosts[i].alpha <= 0.0f)
            mGhosts.erase(mGhosts.begin() + i);
    }
}

void Player::drawGhosts() {
    for (const auto& ghost : mGhosts) {
        float width = ghost.facingRight ? ghost.frameRec.width : -ghost.frameRec.width;
        Rectangle src = { ghost.frameRec.x, ghost.frameRec.y, width, ghost.frameRec.height };

        Vector2 pos = { std::floor(ghost.position.x), std::floor(ghost.position.y) };

        Rectangle dest = { pos.x, pos.y, FRAME_WIDTH, FRAME_HEIGHT };
        DrawTexturePro(mTexture, src, dest, { 0, 0 }, 0.0f, Fade(WHITE, ghost.alpha));
    }
}

void Player::drawWings() {
    if (!mWingsActive) return;

    constexpr float WING_W = 32.0f;
    constexpr float WING_H = 16.0f;

    Rectangle src = {
        mWingsFrame * WING_W,
        0.0f,
        mIsFacingRight ? WING_W : -WING_W,
        WING_H
    };

    Vector2 center = {
        mPosition.x + mSize.x / 2.0f,
        mPosition.y + mSize.y / 2.0f
    };

    Vector2 pos = {
        center.x - WING_W / 2.0f,
        center.y - WING_H / 2.0f + 1.0f
    };

    DrawTextureRec(mWingsTexture, src, pos, Fade(WHITE, 0.5f));
}

void Player::drawSlash() {
    if (!mIsAttacking) return;

    WeaponStats stats = getCurrentWeaponStats();
    float duration = stats.duration * ((mComboCount == 2) ? 1.2f :
        (mComboCount == 1) ? 0.85f : 1.0f);
    float progress = 1.0f - (mAttackTimer / duration);
    int frame = (progress > 0.7f) ? 2 : (progress > 0.3f) ? 1 : 0;

    float baseSize = std::max(stats.reachX, stats.reachY) * 2.2f;
    float spriteSize = (mComboCount == 2) ? baseSize * 1.4f : baseSize;

    Rectangle src = {
        frame * 64.0f,
        0.0f,
        mIsFacingRight ? 64.0f : -64.0f,
        64.0f
    };

    Vector2 center = {
        mPosition.x + mSize.x / 2.0f,
        mPosition.y + mSize.y / 2.0f
    };

    float comboRot = stats.baseRotation;
    if (mComboCount == 1) comboRot -= 10.0f;
    else if (mComboCount == 2) comboRot += 15.0f;
    comboRot += progress * 30.0f * stats.rotationSpeed;

    if (!mIsFacingRight) comboRot = -comboRot;

    Vector2 offset = { 0, 0 };
    float rotation = 0;
    float forwardOffset = stats.reachX * 0.5f;

    switch (mAttackDir) {
    case AttackDirection::HORIZONTAL:
        offset = { mIsFacingRight ? forwardOffset : -forwardOffset, 0 };
        rotation = comboRot;
        break;
    case AttackDirection::UP:
        offset = { 0, -forwardOffset * 0.8f };
        rotation = mIsFacingRight ? -90.0f + comboRot * 0.3f : 90.0f - comboRot * 0.3f;
        break;
    case AttackDirection::DOWN:
        offset = { 0, forwardOffset * 0.8f };
        rotation = mIsFacingRight ? 90.0f + comboRot * 0.3f : -90.0f - comboRot * 0.3f;
        break;
    }

    Rectangle dest = {
        center.x + offset.x,
        center.y + offset.y,
        spriteSize,
        spriteSize
    };
    Vector2 origin = { spriteSize / 2.0f, spriteSize / 2.0f };

    Color color = stats.slashColor;
    if (mComboCount == 1) {
        color = Color{
            (unsigned char)std::min(color.r + 40, 255),
            (unsigned char)std::min(color.g + 40, 255),
            (unsigned char)std::min(color.b + 40, 255),
            255
        };
    }
    else if (mComboCount == 2) {
        color = Color{
            (unsigned char)std::min(color.r + 80, 255),
            (unsigned char)std::min(color.g + 60, 255),
            (unsigned char)std::min(color.b + 20, 255),
            255
        };
    }

    DrawTexturePro(mSlashTexture, src, dest, origin, rotation, color);

    if (mComboCount == 2) {
        Rectangle ghostDest = {
            dest.x, dest.y,
            dest.width * 1.4f, dest.height * 1.4f
        };
        Vector2 ghostOrigin = {
            ghostDest.width / 2.0f,
            ghostDest.height / 2.0f
        };
        DrawTexturePro(mSlashTexture, src, ghostDest, ghostOrigin,
            rotation - 10.0f * stats.rotationSpeed,
            Fade(GOLD, 0.5f));
    }

    if (stats.attackCooldown < 0.1f && frame >= 1) {
        DrawTexturePro(mSlashTexture, src, dest, origin,
            rotation + 8.0f,
            Fade(color, 0.3f));
    }
}

void Player::drawPlayer() {
    float offsetX = (FRAME_WIDTH - HITBOX_WIDTH) / 2.0f;
    float offsetY = (FRAME_HEIGHT - HITBOX_HEIGHT) / 2.0f;

    Vector2 drawPos = {
        mPosition.x - offsetX,
        mPosition.y - offsetY
    };

    if (mCurrentState == AnimState::CLIMB || mCurrentState == AnimState::WALL_SLIDE) {
        float climbOffset = 5.0f;
        drawPos.x += mIsFacingRight ? climbOffset : -climbOffset;
    }

    float srcWidth = mIsFacingRight ? mFrameRec.width : -mFrameRec.width;
    Rectangle src = { mFrameRec.x, mFrameRec.y, srcWidth, mFrameRec.height };

    Rectangle dest = {
        std::floor(drawPos.x),
        std::floor(drawPos.y),
        16.0f,
        16.0f
    };

    Color color = WHITE;
    if (mInvincibilityTimer > 0.0f && (int)(mInvincibilityTimer * 20.0f) % 2 == 0)
        color = RED;

    DrawTexturePro(mTexture, src, dest, { 0, 0 }, 0.0f, color);
}

void Player::drawUI() {
    if (mStamina < MAX_STAMINA) {
        constexpr float BAR_W = 20.0f;
        constexpr float BAR_H = 2.0f;
        float barX = mPosition.x + mSize.x / 2 - BAR_W / 2;
        float barY = mPosition.y - 10.0f;
        float percent = mStamina / MAX_STAMINA;

        if (percent < 0.5f) {
            DrawRectangle((int)barX, (int)barY,
                (int)(BAR_W * percent), (int)BAR_H,
                ColorAlpha(RED, percent));
        }
    }

    if (mIsAttacking || mComboWindowOpen) {
        float progress = mComboWindowTimer / COMBO_WINDOW;
        DrawRectangle((int)mPosition.x - 3, (int)mPosition.y - 12,
            (int)(16.0f * progress), 2, YELLOW);
    }
}