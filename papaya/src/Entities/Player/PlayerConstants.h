#pragma once

namespace PlayerConstants {
    // === WYMIARY ===
    constexpr float HITBOX_WIDTH = 16.0f;
    constexpr float HITBOX_HEIGHT = 16.0f;
    constexpr float FRAME_WIDTH = 16.0f;
    constexpr float FRAME_HEIGHT = 16.0f;

    // === FIZYKA ===
    constexpr float GRAVITY = 450.0f;
    constexpr float MAX_SPEED = 100.0f;
    constexpr float GROUND_ACCEL = 1200.0f;
    constexpr float GROUND_FRICTION = 1400.0f;
    constexpr float AIR_ACCEL = 650.0f;
    constexpr float AIR_TURN_ACCEL = 800.0f;
    constexpr float AIR_DRAG = 600.0f;

    // === SKOK ===
    constexpr float JUMP_FORCE = 185.0f;
    constexpr int MAX_JUMPS = 2;
    constexpr float JUMP_BUFFER_TIME = 0.1f;
    constexpr float COYOTE_TIME = 0.15f;
    constexpr float WALL_JUMP_KICK = 180.0f;

    // === DASH ===
    constexpr float DASH_SPEED = 280.0f;
    constexpr float DASH_DURATION = 0.15f;
    constexpr float DASH_COOLDOWN = 0.4f;
    constexpr float WAVE_DASH_BOOST = 1.3f;
    constexpr float MOMENTUM_DRAG_GROUND = 400.0f;

    // === ŒCIANA ===
    constexpr float WALL_SLIDE_SPEED = 50.0f;
    constexpr float CLIMB_SPEED = 90.0f;
    constexpr float AUTO_STEP_HEIGHT = 8.0f;

    // === STAMINA ===
    constexpr float MAX_STAMINA = 100.0f;
    constexpr float STAMINA_REGEN_SPEED = 50.0f;
    constexpr float COST_CLIMB = 30.0f;
    constexpr float STAMINA_DELAY_TIME = 0.5f;

    // === WALKA ===
    constexpr int MAX_COMBO = 2;
    constexpr float COMBO_WINDOW = 0.5f;

    // === EFEKTY WIZUALNE ===
    constexpr float GHOST_SPAWN_INTERVAL = 0.05f;
    constexpr float GHOST_FADE_SPEED = 2.5f;
    constexpr int WINGS_NUM_FRAMES = 4;
    constexpr float WINGS_SPEED = 0.05f;
}