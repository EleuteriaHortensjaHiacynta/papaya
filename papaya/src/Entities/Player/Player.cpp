#include "Player.h"
#include "Core/InputHelper.h"
#include "Core/MathUtils.h"
#include <algorithm>

using namespace PlayerConstants;
using namespace MathUtils;

// ============================================================================
// KONSTRUKTOR / DESTRUKTOR
// ============================================================================

Player::Player(float startX, float startY)
    : Entity({ startX, startY }, PLAYER)
    , mStartPosition{ startX, startY }
{
    mPosition = { startX, startY };
    mPrevPosition = mPosition;
    mSize = { HITBOX_WIDTH, HITBOX_HEIGHT };

    mSlashTexture = LoadTexture("assets/slash.png");
    mWingsTexture = LoadTexture("assets/jump_wings.png");

    mCurrentAnim = GetAnimationData(AnimState::IDLE);
}

Player::~Player() {
    UnloadTexture(mSlashTexture);
    UnloadTexture(mWingsTexture);
}

// ============================================================================
// INTERFACE ENTITY
// ============================================================================

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
    handleSacrifice();
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

// ============================================================================
// METODY PUBLICZNE
// ============================================================================

bool Player::hasHit(Entity* enemy) const {
    return std::find(mHitEntities.begin(), mHitEntities.end(), enemy) != mHitEntities.end();
}

int Player::getAttackDamage() const {
    return (mComboCount >= 2) ? 2 : 1;
}

WeaponStats Player::getCurrentWeaponStats() const {
    return WeaponStats::GetStats(mCurrentWeapon);
}

// ============================================================================
// STAN KLATKI
// ============================================================================

void Player::saveFrameState() {
    mPrevPosition = mPosition;
    mWasTouchingWall = mTouchingWall;
    mPrevWallDir = mWallDir;
    mWasGrounded = mIsGrounded;
}

void Player::resetFrameState() {
    mTouchingWall = false;
    mWallDir = 0;
}

void Player::respawn() {
    mPosition = mStartPosition;
    mHealth = mMaxHealth;
    mVelocity = { 0, 0 };
    mStamina = MAX_STAMINA;

    // Reset wszystkich stanów
    mRecoilTimer = 0;
    mInvincibilityTimer = 0;
    mIsDashing = false;
    mIsAttacking = false;
    mComboCount = 0;
    mComboWindowOpen = false;
    mCurrentState = AnimState::IDLE;
    mTouchingWall = false;
    mWallDir = 0;
    mIsClimbing = false;
    mWasGrounded = false;
    mWasTouchingWall = false;
}

// ============================================================================
// TIMERY
// ============================================================================

void Player::updateTimers(float dt) {
    // Jump buffer
    mJumpBufferCounter = Input::JumpPressed() ? JUMP_BUFFER_TIME : mJumpBufferCounter - dt;

    // Coyote time
    mCoyoteTimeCounter = mIsGrounded ? COYOTE_TIME : mCoyoteTimeCounter - dt;

    // Reset pogo przy spadaniu
    if (mVelocity.y > 0) mIsPogoJump = false;

    // Cooldowny
    if (mDashCooldownTimer > 0) mDashCooldownTimer -= dt;
    if (mInvincibilityTimer > 0) mInvincibilityTimer -= dt;
    if (mMomentumTimer > 0) mMomentumTimer -= dt;

    // Recoil
    if (mRecoilTimer > 0) {
        mRecoilTimer -= dt;
        float absVelX = std::abs(mVelocity.x);
        if (absVelX > MAX_SPEED) {
            mVelocity.x = Sign(mVelocity.x) * MAX_SPEED;
        }
    }

    // Combo window
    if (mComboWindowOpen) {
        mComboWindowTimer -= dt;
        if (mComboWindowTimer <= 0) {
            mComboWindowOpen = false;
            mComboCount = 0;
        }
    }

    // Attack timer
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

void Player::updateStamina(float dt) {
    if (mIsClimbing) return;

    if (mStaminaRegenDelay > 0) {
        mStaminaRegenDelay -= dt;
    }
    else if (mStamina < MAX_STAMINA) {
        mStamina = std::min(mStamina + STAMINA_REGEN_SPEED * dt, MAX_STAMINA);
    }
}

// ============================================================================
// RUCH
// ============================================================================

void Player::handleMovement(float dt) {
    if (mIsDashing || mIsClimbing) return;

    float moveDir = Input::GetHorizontalAxis();
    bool isSuperSpeed = std::abs(mVelocity.x) > MAX_SPEED;

    if (mIsGrounded) {
        if (isSuperSpeed) {
            float drag = MOMENTUM_DRAG_GROUND;
            // Mniejszy drag gdy idziemy w tym samym kierunku
            if (Sign(mVelocity.x) == moveDir && moveDir != 0) {
                drag *= 0.5f;
            }
            mVelocity.x = Approach(mVelocity.x, moveDir * MAX_SPEED, drag * dt);
        }
        else {
            if (moveDir != 0) {
                mVelocity.x = Approach(mVelocity.x, moveDir * MAX_SPEED, GROUND_ACCEL * dt);
            }
            else {
                mVelocity.x = Approach(mVelocity.x, 0, GROUND_FRICTION * dt);
            }
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

    if (wantsJump) {
        bool jumped = false;

        // Wall jump
        if (mWasTouchingWall && !mIsGrounded) {
            mVelocity.y = -JUMP_FORCE;
            mVelocity.x = -mPrevWallDir * WALL_JUMP_KICK;
            mIsFacingRight = (mPrevWallDir == -1);
            mIsClimbing = false;
            mJumpCount = 1;
            jumped = true;
        }
        // Ground jump (z coyote time)
        else if (mCoyoteTimeCounter > 0) {
            mVelocity.y = -JUMP_FORCE;
            mJumpCount = 1;
            jumped = true;
        }
        // Double jump
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

    // Variable jump height
    if (!Input::JumpHeld() && mVelocity.y < -50.0f && mIsJumping && !mIsPogoJump) {
        mVelocity.y = -50.0f;
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

        // Domyœlny kierunek
        if (dirX == 0.0f && dirY == 0.0f) {
            dirX = mIsFacingRight ? 1.0f : -1.0f;
        }

        // Normalizacja dla diagonali
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
            if (std::abs(mVelocity.y) > MAX_SPEED) {
                mVelocity.y *= 0.6f;
            }
        }
    }
}

void Player::handleClimbing(float dt) {
    if (!mTouchingWall || mStamina <= 0) {
        mIsClimbing = false;
        return;
    }

    // Jeœli w³aœnie skoczy³, nie nadpisuj velocity
    if (mVelocity.y < -100.0f) {
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
        mVelocity.y = WALL_SLIDE_SPEED;
        mVelocity.x = mWallDir * 100.0f;
    }
    else {
        mIsClimbing = false;
    }
}

void Player::applyPhysics(float dt) {
    if (!mIsDashing && !mIsClimbing) {
        mVelocity.y += GRAVITY * dt;
    }

    mIsGrounded = false;
    mPosition.x += mVelocity.x * dt;
    mPosition.y += mVelocity.y * dt;
}

// ============================================================================
// KOLIZJE
// ============================================================================

void Player::handleWallCollision(Rectangle wall) {
    float prevBottom = mPrevPosition.y + mSize.y;
    constexpr float THRESHOLD = 4.0f;

    // 1. Kolizja od góry (l¹dowanie)
    if (prevBottom <= wall.y + THRESHOLD) {
        mPosition.y = wall.y - mSize.y;
        mVelocity.y = 0;
        mIsGrounded = true;
        mCanDash = true;
        mIsJumping = false;
        mJumpCount = 0;

        // Wave dash
        if (mIsDashing) {
            mIsDashing = false;
            float dashDir = Sign(mVelocity.x);
            if (dashDir == 0) {
                dashDir = Input::IsMovingRight() ? 1.0f :
                    (Input::IsMovingLeft() ? -1.0f :
                        (mIsFacingRight ? 1.0f : -1.0f));
            }
            mVelocity.x = dashDir * DASH_SPEED * WAVE_DASH_BOOST;
        }
        return;
    }

    // 2. Kolizja od do³u (sufit)
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

    // 3. Kolizja boczna z auto-step
    float playerCenterX = mPrevPosition.x + mSize.x / 2.0f;
    float wallCenterX = wall.x + wall.width / 2.0f;
    float playerBottom = mPosition.y + mSize.y;
    float stepHeight = playerBottom - wall.y;

    bool movingTowardsWall = (playerCenterX < wallCenterX && Input::IsMovingRight()) ||
        (playerCenterX > wallCenterX && Input::IsMovingLeft());

    bool canAutoStep = (mWasGrounded || mIsGrounded) &&
        stepHeight > 0.0f &&
        stepHeight <= AUTO_STEP_HEIGHT &&
        movingTowardsWall &&
        mVelocity.y >= -20.0f &&
        mVelocity.y <= 100.0f &&
        !mIsDashing &&
        !mIsClimbing &&
        std::abs(mVelocity.x) > 5.0f;

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

// ============================================================================
// WALKA
// ============================================================================

void Player::handleAttack(float dt) {
    if (!Input::AttackPressed() || mIsAttacking) return;

    // Combo logic
    if (mComboWindowOpen) {
        mComboCount = std::min(mComboCount + 1, MAX_COMBO);
    }
    else {
        mComboCount = 0;
    }
    mComboWindowOpen = false;

    WeaponStats stats = getCurrentWeaponStats();
    mIsAttacking = true;
    mCurrentState = AnimState::ATTACK;

    // Czas trwania zale¿ny od combo
    float durationMult = (mComboCount == 0) ? 1.0f :
        (mComboCount == 1) ? 0.85f : 1.2f;
    mAttackTimer = stats.duration * durationMult;

    createAttackHitbox();

    // Impuls przy ataku poziomym na ziemi
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

void Player::handleSacrifice() {
    if (Input::SacrificePressed() && mCurrentWeapon != WeaponType::SWORD_DEFAULT) {
        if (mHealth < mMaxHealth) {
            mHealth++;
        }
        mCurrentWeapon = WeaponType::SWORD_DEFAULT;
    }
}

// ============================================================================
// ANIMACJA
// ============================================================================

void Player::determineAnimationState() {
    float moveInput = Input::GetHorizontalAxis();
    bool isMoving = std::abs(mVelocity.x) > 1.0f || moveInput != 0;

    // Aktualizacja kierunku
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

    if (mIsAttacking) {
        newState = AnimState::ATTACK;
    }
    else if (mIsDashing) {
        newState = AnimState::DASH;
    }
    else if (mIsClimbing) {
        // Zachowaj CLIMB lub WALL_SLIDE
        return;
    }
    else if (!mIsGrounded) {
        newState = (mVelocity.y < -50.0f) ? AnimState::JUMP_UP : AnimState::JUMP_DOWN;
    }
    else if (!isTurning) {
        newState = isMoving ? AnimState::WALK : AnimState::IDLE;
    }

    if (mCurrentState != newState) {
        mCurrentState = newState;
        mCurrentFrameRelative = 0;
        mFrameTimer = 0.0f;
    }
}

void Player::updateAnimation(float dt) {
    determineAnimationState();

    // Pobierz dane animacji
    mCurrentAnim = GetAnimationData(mCurrentState, mVelocity.y, mAttackDir);

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

                // Specjalna obs³uga TURN
                if (mCurrentState == AnimState::TURN) {
                    float moveInput = Input::GetHorizontalAxis();
                    mCurrentState = (moveInput != 0 || std::abs(mVelocity.x) > 1.0f)
                        ? AnimState::WALK : AnimState::IDLE;
                    mCurrentFrameRelative = 0;
                }
            }
        }
    }

    mFrameRec.x = (float)(mCurrentAnim.startFrame + mCurrentFrameRelative) * FRAME_WIDTH;
}

void Player::updateWings(float dt) {
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
    bool isSuperSpeed = std::abs(mVelocity.x) > speedThreshold;
    bool shouldSpawn = mIsDashing || (isSuperSpeed && mRecoilTimer <= 0);

    if (shouldSpawn) {
        mGhostSpawnTimer -= dt;
        if (mGhostSpawnTimer <= 0) {
            mGhostSpawnTimer = GHOST_SPAWN_INTERVAL;
            mGhosts.push_back({ mPosition, mFrameRec, mIsFacingRight, 0.5f });
        }
    }
    else {
        mGhostSpawnTimer = 0;
    }

    // Usuwanie starych duchów
    for (int i = (int)mGhosts.size() - 1; i >= 0; i--) {
        mGhosts[i].alpha -= GHOST_FADE_SPEED * dt;
        if (mGhosts[i].alpha <= 0.0f) {
            mGhosts.erase(mGhosts.begin() + i);
        }
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void Player::drawGhosts() {
    for (const auto& ghost : mGhosts) {
        float width = ghost.facingRight ? ghost.frameRec.width : -ghost.frameRec.width;
        Rectangle src = { ghost.frameRec.x, ghost.frameRec.y, width, ghost.frameRec.height };
        Vector2 pos = { std::floor(ghost.position.x - 3), std::floor(ghost.position.y - 2) };
        Rectangle dest = { pos.x, pos.y, FRAME_WIDTH, FRAME_HEIGHT };
        DrawTexturePro(mTexture, src, dest, { 0, 0 }, 0.0f, Fade(WHITE, ghost.alpha));
    }
}

void Player::drawWings() {
    if (!mWingsActive) return;

    constexpr float WING_W = 32.0f;
    constexpr float WING_H = 16.0f;

    Rectangle src = { mWingsFrame * WING_W, 0.0f, mIsFacingRight ? WING_W : -WING_W, WING_H };
    Vector2 center = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };
    Vector2 pos = { center.x - WING_W / 2.0f, center.y - WING_H / 2.0f + 1.0f };

    DrawTextureRec(mWingsTexture, src, pos, Fade(WHITE, 0.5f));
}

void Player::drawSlash() {
    if (!mIsAttacking) return;

    WeaponStats stats = getCurrentWeaponStats();
    float duration = stats.duration * ((mComboCount == 2) ? 1.2f : (mComboCount == 1) ? 0.85f : 1.0f);
    float progress = 1.0f - (mAttackTimer / duration);

    int frame = (progress > 0.7f) ? 2 : (progress > 0.3f) ? 1 : 0;
    float spriteSize = (mComboCount == 2) ? 80.0f : 64.0f;

    Rectangle src = { frame * 64.0f, 0.0f, mIsFacingRight ? 64.0f : -64.0f, 64.0f };
    Vector2 center = { mPosition.x + mSize.x / 2.0f, mPosition.y + mSize.y / 2.0f };

    // Rotacja zale¿na od combo
    float comboRot = (mComboCount == 0) ? 15.0f : (mComboCount == 1) ? -25.0f : 40.0f;
    if (!mIsFacingRight) comboRot = -comboRot;

    Vector2 offset = { 0, 0 };
    float rotation = 0;

    switch (mAttackDir) {
    case AttackDirection::HORIZONTAL:
        offset = { mIsFacingRight ? 10.0f + (mComboCount == 2 ? 4.0f : 0) : -10.0f - (mComboCount == 2 ? 4.0f : 0), 0 };
        rotation = comboRot;
        break;
    case AttackDirection::UP:
        offset = { 0, -10 };
        rotation = mIsFacingRight ? -90.0f + comboRot * 0.5f : 90.0f - comboRot * 0.5f;
        break;
    case AttackDirection::DOWN:
        offset = { 0, 10 };
        rotation = mIsFacingRight ? 90.0f + comboRot * 0.5f : -90.0f - comboRot * 0.5f;
        break;
    }

    Rectangle dest = { center.x + offset.x, center.y + offset.y, spriteSize, spriteSize };
    Vector2 origin = { spriteSize / 2.0f, spriteSize / 2.0f };

    // Kolor zale¿ny od combo
    Color color = (mComboCount == 0) ? WHITE :
        (mComboCount == 1) ? Color{ 200, 200, 255, 255 } :
        Color{ 255, 180, 50, 255 };

    if (mComboCount == 2 && progress < 0.3f) {
        color = { 255, 255, 180, 255 };
    }

    DrawTexturePro(mSlashTexture, src, dest, origin, rotation, color);

    // Finisher ghost effect
    if (mComboCount == 2) {
        Rectangle ghostDest = { dest.x, dest.y, dest.width * 1.3f, dest.height * 1.3f };
        Vector2 ghostOrigin = { ghostDest.width / 2.0f, ghostDest.height / 2.0f };
        DrawTexturePro(mSlashTexture, src, ghostDest, ghostOrigin, rotation - 15.0f, { 255, 150, 50, 80 });
    }
}

void Player::drawPlayer() {
    Vector2 drawPos = { mPosition.x - 3, mPosition.y - 2 };

    if (mCurrentState == AnimState::CLIMB || mCurrentState == AnimState::WALL_SLIDE) {
        drawPos.x += mIsFacingRight ? 2.0f : -2.0f;
    }

    float srcWidth = mIsFacingRight ? mFrameRec.width : -mFrameRec.width;
    Rectangle src = { mFrameRec.x, mFrameRec.y, srcWidth, mFrameRec.height };
    Rectangle dest = { std::floor(drawPos.x), std::floor(drawPos.y), FRAME_WIDTH, FRAME_HEIGHT };

    Color color = WHITE;
    if (mInvincibilityTimer > 0.0f && (int)(mInvincibilityTimer * 20.0f) % 2 == 0) {
        color = RED;
    }

    DrawTexturePro(mTexture, src, dest, { 0, 0 }, 0.0f, color);
}

void Player::drawUI() {
    // Stamina bar
    if (mStamina < MAX_STAMINA) {
        constexpr float BAR_W = 20.0f;
        constexpr float BAR_H = 2.0f;
        float barX = mPosition.x + mSize.x / 2 - BAR_W / 2;
        float barY = mPosition.y - 10.0f;
        float percent = mStamina / MAX_STAMINA;

        if (percent < 0.5f) {
            DrawRectangle((int)barX, (int)barY, (int)(BAR_W * percent), (int)BAR_H,
                ColorAlpha(RED, percent));
        }
    }

    // Combo indicator
    if (mIsAttacking || mComboWindowOpen) {
        const char* text = (mComboCount == 0) ? "1" : (mComboCount == 1) ? "2" : "3!";
        Color color = (mComboCount == 0) ? WHITE : (mComboCount == 1) ? SKYBLUE : GOLD;

        DrawText(text, (int)mPosition.x + 2, (int)mPosition.y - 18, 8, color);

        if (mComboWindowOpen) {
            float progress = mComboWindowTimer / COMBO_WINDOW;
            DrawRectangle((int)mPosition.x - 3, (int)mPosition.y - 12, (int)(16.0f * progress), 2, YELLOW);
        }
    }
}