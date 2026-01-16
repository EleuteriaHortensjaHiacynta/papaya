#pragma once

namespace PlayerConstants {
    // === FIZYKA ===
    constexpr float MAX_SPEED = 100.0f;
    constexpr float ACCELERATION = 810.0f;
    constexpr float FRICTION = 790.0f;
    constexpr float GRAVITY = 500.0f;
    constexpr float JUMP_FORCE = 200.0f;
    constexpr float RECOIL_FRICTION = 60.0f;

    // === RUCH NAZIEMNY ===
    constexpr float GROUND_ACCEL = 2000.0f;
    constexpr float GROUND_FRICTION = 3000.0f;
    constexpr float MOMENTUM_DRAG_GROUND = 1000.0f;

    // === RUCH W POWIETRZU ===
    constexpr float AIR_ACCEL = 1200.0f;
    constexpr float AIR_TURN_ACCEL = 3000.0f;
    constexpr float AIR_DRAG = 500.0f;

    // === SKOK ===
    constexpr int MAX_JUMPS = 2;
    constexpr float JUMP_BUFFER_TIME = 0.1f;
    constexpr float COYOTE_TIME = 0.15f;
    constexpr float WALL_JUMP_KICK = 130.0f;

    // === DASH ===
    constexpr float DASH_SPEED = 360.0f;
    constexpr float DASH_DURATION = 0.15f;
    constexpr float DASH_COOLDOWN = 0.5f;
    constexpr float WAVE_DASH_BOOST = 1.3f;

    // === WSPINACZKA ===
    constexpr float MAX_STAMINA = 100.0f;
    constexpr float STAMINA_REGEN_SPEED = 50.0f;
    constexpr float STAMINA_DELAY_TIME = 0.5f;
    constexpr float COST_CLIMB = 30.0f;
    constexpr float CLIMB_SPEED = 50.0f;
    constexpr float WALL_SLIDE_SPEED = 30.0f;

    // === AUTO-STEP ===
    constexpr float AUTO_STEP_HEIGHT = 16.0f;

    // === COMBO ===
    constexpr float COMBO_WINDOW = 0.45f;
    constexpr int MAX_COMBO = 2;

    // === ANIMACJA ===
    constexpr float WINGS_SPEED = 0.08f;
    constexpr int WINGS_NUM_FRAMES = 6;
    constexpr float GHOST_SPAWN_INTERVAL = 0.02f;
    constexpr float GHOST_FADE_SPEED = 3.0f;

    // === SPRITE ===
    constexpr float FRAME_WIDTH = 16.0f;
    constexpr float FRAME_HEIGHT = 16.0f;
    constexpr float HITBOX_WIDTH = 10.0f;
    constexpr float HITBOX_HEIGHT = 14.0f;
}