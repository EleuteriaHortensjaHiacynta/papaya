#pragma once
#include "raylib.h"

enum class WeaponType {
    SWORD_DEFAULT,
    DAGGER_SWIFT,
    AXE_HEAVY,
    SPEAR_LONG,
    KATANA_BLOOD
};

enum class AttackDirection {
    HORIZONTAL,
    UP,
    DOWN
};

struct WeaponStats {
    float damage;
    float reachX;
    float reachY;
    float duration;         // Attack duration
    float attackCooldown;
    Color slashColor;
    float rotationSpeed;
    float baseRotation;

    static WeaponStats GetStats(WeaponType type) {
        switch (type) {
        case WeaponType::SWORD_DEFAULT:
            return {
                2.0f,       // damage
                30.0f,      // reachX
                25.0f,      // reachY
                0.45f,      // duration
                0.50f,      // cooldown
                Color{200, 200, 255, 255},
                0.3f,
                0.0f
            };

        case WeaponType::DAGGER_SWIFT:
            return {
                1.0f,       // damage
                22.0f,      // reachX
                18.0f,      // reachY
                0.30f,      // duration
                0.25f,      // cooldown
                Color{150, 255, 150, 255},
                0.6f,
                -5.0f
            };

        case WeaponType::AXE_HEAVY:
            return {
                5.0f,       // damage
                35.0f,      // reachX
                30.0f,      // reachY
                0.80f,      // duration
                0.90f,      // cooldown
                Color{255, 120, 60, 255},
                0.2f,
                10.0f
            };

        case WeaponType::SPEAR_LONG:
            return {
                1.5f,       // damage
                55.0f,      // reachX
                16.0f,      // reachY
                0.60f,      // duration
                0.55f,      // cooldown
                Color{255, 255, 100, 255},
                0.1f,
                0.0f
            };

        case WeaponType::KATANA_BLOOD:
            return {
                3.5f,       // damage
                38.0f,      // reachX
                21.0f,      // reachY
                0.40f,      // duration
                0.45f,      // cooldown
                Color{255, 30, 60, 255},
                0.5f,
                5.0f
            };

        default:
            return GetStats(WeaponType::SWORD_DEFAULT);
        }
    }

    static const char* GetName(WeaponType type) {
        switch (type) {
        case WeaponType::SWORD_DEFAULT: return "Miecz";
        case WeaponType::DAGGER_SWIFT: return "Sztylet";
        case WeaponType::AXE_HEAVY: return "Topor";
        case WeaponType::SPEAR_LONG: return "Wlocnia";
        case WeaponType::KATANA_BLOOD: return "Katana";
        default: return "???";
        }
    }
};

enum class AnimState {
    IDLE, WALK, TURN,
    JUMP_UP, JUMP_DOWN,
    DASH, CLIMB, WALL_SLIDE,
    ATTACK
};

struct AnimationData {
    int startFrame;
    int frameCount;
    float frameSpeed;
    bool loop;

    static AnimationData GetAnimationData(AnimState state, float velocityY = 0, AttackDirection attackDir = AttackDirection::HORIZONTAL) {
        switch (state) {
            // IDLE & WALK
        case AnimState::IDLE:       return { 12, 6, 0.15f, true };
        case AnimState::WALK:       return { 0, 6, 0.1f, true };

                            // JUMP & AIR
        case AnimState::JUMP_UP:    return { 6, 3, 0.1f, false };
        case AnimState::JUMP_DOWN:  return { 9, 3, 0.1f, false };

                                 // ACTION
        case AnimState::DASH:       return { 6, 1, 0.5f, false };
        case AnimState::TURN:       return { 18, 3, 0.06f, false };

                            // CLIMBING
        case AnimState::CLIMB:
            if (velocityY < 0) return { 39, 2, 0.15f, true }; // Up
            if (velocityY > 0) return { 41, 2, 0.15f, true }; // Down
            return { 39, 1, 0.15f, true };                    // Hang

        case AnimState::WALL_SLIDE: return { 43, 2, 0.15f, true };

                                  // ATTACK
        case AnimState::ATTACK:
            switch (attackDir) {
            case AttackDirection::UP:    return { 28, 5, 0.07f, false };
            case AttackDirection::DOWN:  return { 34, 5, 0.07f, false };
            default:                     return { 22, 5, 0.07f, false };
            }

        default:                    return { 12, 6, 0.15f, true };
        }
    }
};

struct Ghost {
    Vector2 position;
    Rectangle frameRec;
    bool facingRight;
    float alpha;
};