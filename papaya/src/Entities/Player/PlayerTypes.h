#pragma once
#include "raylib.h"

// === TYPY BRONI ===
enum class WeaponType {
    SWORD_DEFAULT,
    DAGGER_SWIFT,
    AXE_HEAVY,
    SPEAR_LONG,
    KATANA_BLOOD
};

// === KIERUNKI ATAKU ===
enum class AttackDirection {
    HORIZONTAL,
    UP,
    DOWN
};

// === STATY BRONI ===
struct WeaponStats {
    float damage;
    float reachX;
    float reachY;
    float duration;         // Ile czasu trwa animacja ataku (im więcej = wolniej)
    float attackCooldown;   // Ile czasu trzeba czekać na kolejny atak
    Color slashColor;
    float rotationSpeed;
    float baseRotation;

    static WeaponStats GetStats(WeaponType type) {
        switch (type) {

            // 🗡️ MIECZ DOMYŚLNY (Zbalansowany)
        case WeaponType::SWORD_DEFAULT:
            return {
                2.0f,       // damage (było 15.0)
                30.0f,      // reachX
                25.0f,      // reachY
                0.45f,      // duration (zwolnione z 0.35)
                0.50f,      // cooldown (zwolnione z 0.35)
                Color{200, 200, 255, 255},
                0.3f,
                0.0f
            };

            // 🗡️ SZTYLET (Szybki, mały dmg - baza 1)
        case WeaponType::DAGGER_SWIFT:
            return {
                1.0f,       // damage (było 8.0)
                22.0f,      // reachX
                18.0f,      // reachY
                0.30f,      // duration (zwolnione z 0.2)
                0.25f,      // cooldown (nadal szybki, ale bez przesady)
                Color{150, 255, 150, 255},
                0.6f,
                -5.0f
            };

            // 🪓 TOPÓR (Bardzo wolny, potężny dmg)
        case WeaponType::AXE_HEAVY:
            return {
                5.0f,       // damage (było 28.0) - Zgodnie z prośbą max
                35.0f,      // reachX
                30.0f,      // reachY
                0.80f,      // duration (bardzo wolny zamach - było 0.5)
                0.90f,      // cooldown (duża kara za chybienie)
                Color{255, 120, 60, 255},
                0.2f,
                10.0f
            };

            // 🔱 WŁÓCZNIA (Duży zasięg, mały dmg)
        case WeaponType::SPEAR_LONG:
            return {
                1.5f,       // damage (było 12.0)
                55.0f,      // reachX (podbity zasięg, żeby miała sens przy małym dmg)
                16.0f,      // reachY
                0.60f,      // duration (wolniejsze dźgnięcie - było 0.38)
                0.55f,      // cooldown
                Color{255, 255, 100, 255},
                0.1f,
                0.0f
            };

            // ⚔️ KATANA (Dobre obrażenia, średnia prędkość)
        case WeaponType::KATANA_BLOOD:
            return {
                3.5f,       // damage (było 20.0)
                38.0f,      // reachX
                21.0f,      // reachY
                0.40f,      // duration (zwolnione z 0.28)
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

// === ANIMACJE ===
enum class AnimState {
    IDLE, WALK, TURN,
    JUMP_UP, JUMP_DOWN,
    DASH, CLIMB, WALL_SLIDE,
    ATTACK
};

// === DANE ANIMACJI (NAPRAWIONE) ===
struct AnimationData {
    int startFrame;
    int frameCount;
    float frameSpeed;
    bool loop;

    static AnimationData GetAnimationData(AnimState state, float velocityY = 0, AttackDirection attackDir = AttackDirection::HORIZONTAL) {
        switch (state) {

            // IDLE & WALK (Poprawione indeksy)
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
            if (velocityY < 0) return { 39, 2, 0.15f, true }; // Góra
            if (velocityY > 0) return { 41, 2, 0.15f, true }; // Dół
            return { 39, 1, 0.15f, true };                    // Zwis

        case AnimState::WALL_SLIDE: return { 43, 2, 0.15f, true };

                                  // ATTACK (Poprawione klatki)
        case AnimState::ATTACK:
            switch (attackDir) {
            case AttackDirection::UP:    return { 28, 5, 0.07f, false }; // Nieco wolniejsza animacja (0.07)
            case AttackDirection::DOWN:  return { 34, 5, 0.07f, false };
            default:                     return { 22, 5, 0.07f, false };
            }

        default:                    return { 12, 6, 0.15f, true };
        }
    }
};

// === GHOST EFFECT ===
struct Ghost {
    Vector2 position;
    Rectangle frameRec;
    bool facingRight;
    float alpha;
};