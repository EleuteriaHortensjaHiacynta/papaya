#pragma once
#include "raylib.h"

// === EFEKTY WIZUALNE ===
struct Ghost {
    Vector2 position;
    Rectangle frameRec;
    bool facingRight;
    float alpha;
};

// === BRONIE ===
enum class WeaponType {
    SWORD_DEFAULT,
    KATANA,
    SPEAR
};

struct WeaponStats {
    float damage;
    float reachX;
    float reachY;
    float duration;
    float recoilSelf;
    float pogoForce;
    Color debugColor;

    static WeaponStats GetStats(WeaponType type) {
        switch (type) {
        case WeaponType::KATANA:
            return { 2.0f, 30.0f, 15.0f, 0.25f, 200.0f, -200.0f, PURPLE };
        case WeaponType::SPEAR:
            return { 1.5f, 40.0f, 10.0f, 0.4f, 150.0f, -180.0f, BLUE };
        default:
            return { 1.0f, 25.0f, 20.0f, 0.33f, 300.0f, -260.0f, WHITE };
        }
    }
};

// === STANY ANIMACJI ===
enum class AnimState {
    IDLE,
    WALK,
    JUMP_UP,
    JUMP_DOWN,
    TURN,
    ATTACK,
    DASH,
    CLIMB,
    WALL_SLIDE
};

// === KIERUNKI ATAKU ===
enum class AttackDirection {
    HORIZONTAL = 0,
    UP = 1,
    DOWN = 2
};

// === DANE ANIMACJI ===
struct AnimationData {
    int startFrame;
    int frameCount;
    float frameSpeed;
    bool loop;
};

// Tabela animacji
inline AnimationData GetAnimationData(AnimState state, float velocityY = 0.0f, AttackDirection attackDir = AttackDirection::HORIZONTAL) {
    switch (state) {
    case AnimState::IDLE:
        return { 12, 6, 0.15f, true };
    case AnimState::WALK:
        return { 0, 6, 0.1f, true };
    case AnimState::JUMP_UP:
        return { 6, 3, 0.1f, false };
    case AnimState::JUMP_DOWN:
        return { 9, 3, 0.1f, false };
    case AnimState::DASH:
        return { 6, 1, 0.5f, false };
    case AnimState::TURN:
        return { 18, 3, 0.06f, false };
    case AnimState::CLIMB:
        if (velocityY < 0) return { 39, 2, 0.15f, true };
        if (velocityY > 0) return { 41, 2, 0.15f, true };
        return { 39, 1, 0.15f, true };
    case AnimState::WALL_SLIDE:
        return { 43, 2, 0.15f, true };
    case AnimState::ATTACK:
        switch (attackDir) {
        case AttackDirection::UP:   return { 28, 5, 0.06f, false };
        case AttackDirection::DOWN: return { 34, 5, 0.06f, false };
        default:                    return { 22, 5, 0.06f, false };
        }
    default:
        return { 12, 6, 0.15f, true };
    }
}