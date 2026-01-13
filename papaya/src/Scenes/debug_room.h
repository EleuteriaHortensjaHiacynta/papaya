#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <cmath>

#include "Entities/Player.h"
#include "Entities/Wall.h"

inline void debugRoom(int windowWidth, int windowHeight) {
    const int gameWidth = 320;
    const int gameHeight = 180;

    RenderTexture2D target = LoadRenderTexture(gameWidth, gameHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    Player player(50.0f, 100.0f);
    std::vector<Wall> walls;

    // ========== BUDOWANIE ARENY TESTOWEJ ==========

    // Pod³oga g³ówna
    walls.push_back(Wall(0, 160, 500.0f, 20.0f));

    // Platformy do testowania skoków
    walls.push_back(Wall(50, 130, 40.0f, 8.0f));   // Niska
    walls.push_back(Wall(110, 100, 40.0f, 8.0f));  // Œrednia
    walls.push_back(Wall(170, 70, 40.0f, 8.0f));   // Wysoka
    walls.push_back(Wall(230, 40, 40.0f, 8.0f));   // Bardzo wysoka (wymaga double jump)

    // Œciany do testowania wall climb
    walls.push_back(Wall(300, 60, 16.0f, 100.0f)); // Prawa œciana
    walls.push_back(Wall(0, 80, 16.0f, 80.0f));    // Lewa œciana

    // Tunel (sufit + pod³oga)
    walls.push_back(Wall(350, 160, 100.0f, 20.0f)); // Pod³oga tunelu
    walls.push_back(Wall(350, 130, 100.0f, 10.0f)); // Sufit tunelu

    // Kolce (damaging) - czerwone
    walls.push_back(Wall(450, 152, 30.0f, 8.0f, false, true));

    // Platforma nad kolcami (bezpieczna)
    walls.push_back(Wall(440, 120, 50.0f, 8.0f));

    // Schody do testowania p³ynnoœci
    for (int i = 0; i < 5; i++) {
        walls.push_back(Wall(500.0f + i * 20.0f, 150.0f - i * 15.0f, 25.0f, 8.0f));
    }

    // Du¿a platforma na koñcu
    walls.push_back(Wall(600, 80, 80.0f, 8.0f));

    // Œciana do odbicia (wall jump test)
    walls.push_back(Wall(680, 20, 16.0f, 68.0f));

    Camera2D camera = { 0 };
    camera.offset = { gameWidth / 2.0f, gameHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    bool showHitboxes = true;
    bool showStats = true;
    bool slowMotion = false;
    bool godMode = false;
    bool freezePlayer = false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ESCAPE)) break;

        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // ========== DEBUG CONTROLS ==========

        // F1 - Hitboxy
        if (IsKeyPressed(KEY_F1)) showHitboxes = !showHitboxes;

        // F2 - Statystyki
        if (IsKeyPressed(KEY_F2)) showStats = !showStats;

        // F3 - Slow motion
        if (IsKeyPressed(KEY_F3)) slowMotion = !slowMotion;
        if (slowMotion) dt *= 0.25f;

        // F4 - God mode
        if (IsKeyPressed(KEY_F4)) {
            godMode = !godMode;
            if (godMode) player.mInvincibilityTimer = 9999.0f;
            else player.mInvincibilityTimer = 0.0f;
        }

        // F5 - Freeze player
        if (IsKeyPressed(KEY_F5)) freezePlayer = !freezePlayer;

        // R - Reset pozycji
        if (IsKeyPressed(KEY_R)) {
            player.mPosition = { 50.0f, 100.0f };
            player.mVelocity = { 0, 0 };
            player.mHealth = player.mMaxHealth;
            player.mStamina = player.mMaxStamina;
        }

        // T - Teleport do kursora (relatywnie do kamery)
        if (IsKeyPressed(KEY_T)) {
            Vector2 mouseScreen = GetMousePosition();
            float scaleX = (float)gameWidth / windowWidth;
            float scaleY = (float)gameHeight / windowHeight;
            player.mPosition.x = camera.target.x - gameWidth / 2 + mouseScreen.x * scaleX;
            player.mPosition.y = camera.target.y - gameHeight / 2 + mouseScreen.y * scaleY;
            player.mVelocity = { 0, 0 };
        }

        // H - Heal
        if (IsKeyPressed(KEY_H)) {
            player.mHealth = player.mMaxHealth;
            player.mStamina = player.mMaxStamina;
        }

        // G - Give damage (test)
        if (IsKeyPressed(KEY_G)) {
            player.takeDamage(1);
        }

        // 1-3 - Zmiana broni
        if (IsKeyPressed(KEY_ONE)) player.mCurrentSpecialWeapon = SWORD_DEFAULT;
        if (IsKeyPressed(KEY_TWO)) player.mCurrentSpecialWeapon = KATANA;
        if (IsKeyPressed(KEY_THREE)) player.mCurrentSpecialWeapon = SPEAR;

        // ========== UPDATE ==========

        if (!freezePlayer) {
            player.update(dt);

            // Kolizja
            player.mPosition.x += player.mVelocity.x * dt;
            Rectangle pRect = player.getRect();

            for (const auto& wall : walls) {
                if (!wall.mCollidable) continue;
                Rectangle wallRect = wall.getRect();
                if (CheckCollisionRecs(pRect, wallRect)) {
                    if (player.mVelocity.x > 0) {
                        player.mPosition.x = wallRect.x - player.mSize.x;
                        player.mTouchingWall = true;
                        player.mWallDir = 1;
                    }
                    else if (player.mVelocity.x < 0) {
                        player.mPosition.x = wallRect.x + wallRect.width;
                        player.mTouchingWall = true;
                        player.mWallDir = -1;
                    }
                    player.mVelocity.x = 0;
                    pRect = player.getRect();
                }
            }

            player.mPosition.y += player.mVelocity.y * dt;
            pRect = player.getRect();

            for (const auto& wall : walls) {
                if (!wall.mCollidable) continue;
                Rectangle wallRect = wall.getRect();
                if (CheckCollisionRecs(pRect, wallRect)) {
                    if (player.mVelocity.y > 0) {
                        player.mPosition.y = wallRect.y - player.mSize.y;
                        player.onGroundHit();
                    }
                    else if (player.mVelocity.y < 0) {
                        player.mPosition.y = wallRect.y + wallRect.height;
                    }
                    player.mVelocity.y = 0;
                    pRect = player.getRect();
                }
            }

            // Damage tiles
            if (!godMode) {
                for (const auto& wall : walls) {
                    if (wall.mDamaging && CheckCollisionRecs(player.getRect(), wall.getRect())) {
                        player.takeDamage(1);
                        break;
                    }
                }
            }
        }

        // Kamera
        camera.target.x = std::floor(player.mPosition.x);
        camera.target.y = std::floor(player.mPosition.y);

        // ========== DRAW GAME ==========

        BeginTextureMode(target);
        ClearBackground({ 40, 44, 52, 255 }); // Ciemne t³o
        BeginMode2D(camera);

        // Siatka (grid)
        if (showHitboxes) {
            for (int x = -500; x < 1000; x += 16) {
                DrawLine(x, -200, x, 300, Fade(GRAY, 0.2f));
            }
            for (int y = -200; y < 300; y += 16) {
                DrawLine(-500, y, 1000, y, Fade(GRAY, 0.2f));
            }
        }

        // Œciany
        for (const auto& wall : walls) {
            Color wallColor = DARKGRAY;
            if (wall.mDamaging) wallColor = MAROON;  // Kolce s¹ czerwone

            DrawRectangleRec(wall.getRect(), wallColor);

            if (showHitboxes) {
                Color outlineColor = wall.mDamaging ? RED : GRAY;
                DrawRectangleLinesEx(wall.getRect(), 1, outlineColor);
            }
        }

        // Gracz
        player.draw();

        // Hitboxy
        if (showHitboxes) {
            // Hitbox gracza
            DrawRectangleLinesEx(player.getRect(), 1, GREEN);

            // Attack area
            if (player.mIsAttacking && player.mAttackArea.width > 0) {
                DrawRectangleLinesEx(player.mAttackArea, 1, RED);
                DrawRectangleRec(player.mAttackArea, Fade(RED, 0.3f));
            }
        }

        // Punkt spawn
        DrawCircle(50, 100, 3, YELLOW);

        // Etykiety stref
        DrawText("SPAWN", 40, 85, 5, YELLOW);
        DrawText("PLATFORMS", 100, 55, 5, WHITE);
        DrawText("WALL CLIMB", 302, 50, 5, WHITE);
        DrawText("SPIKES!", 445, 140, 5, RED);
        DrawText("STAIRS", 520, 135, 5, WHITE);

        EndMode2D();
        EndTextureMode();

        // ========== DRAW UI ==========

        BeginDrawing();
        ClearBackground(BLACK);

        // Renderuj grê
        DrawTexturePro(target.texture,
            { 0, 0, (float)target.texture.width, -(float)target.texture.height },
            { 0, 0, (float)windowWidth, (float)windowHeight },
            { 0, 0 }, 0.0f, WHITE);

        // Panel debug (prawa strona)
        if (showStats) {
            int panelX = windowWidth - 250;
            int panelY = 10;
            int lineHeight = 18;

            DrawRectangle(panelX - 10, panelY - 5, 250, 400, Fade(BLACK, 0.7f));
            DrawText("=== DEBUG PANEL ===", panelX, panelY, 16, YELLOW);
            panelY += lineHeight + 5;

            // Pozycja
            DrawText(TextFormat("Pos: %.1f, %.1f", player.mPosition.x, player.mPosition.y), panelX, panelY, 14, WHITE);
            panelY += lineHeight;

            // Prêdkoœæ
            DrawText(TextFormat("Vel: %.1f, %.1f", player.mVelocity.x, player.mVelocity.y), panelX, panelY, 14, WHITE);
            panelY += lineHeight;

            // Stan
            const char* stateNames[] = { "IDLE", "WALK", "JUMP_UP", "JUMP_DOWN", "TURN", "ATTACK", "DASH", "CLIMB", "WALL_SLIDE" };
            DrawText(TextFormat("State: %s", stateNames[player.mCurrentState]), panelX, panelY, 14, LIME);
            panelY += lineHeight;

            // HP / Stamina
            DrawText(TextFormat("HP: %d / %d", player.mHealth, player.mMaxHealth), panelX, panelY, 14, RED);
            panelY += lineHeight;
            DrawText(TextFormat("Stamina: %.0f / %.0f", player.mStamina, player.mMaxStamina), panelX, panelY, 14, ORANGE);
            panelY += lineHeight;

            // Flagi
            DrawText(TextFormat("Grounded: %s", player.mIsGrounded ? "YES" : "NO"), panelX, panelY, 14, player.mIsGrounded ? GREEN : GRAY);
            panelY += lineHeight;
            DrawText(TextFormat("Dashing: %s", player.mIsDashing ? "YES" : "NO"), panelX, panelY, 14, player.mIsDashing ? GREEN : GRAY);
            panelY += lineHeight;
            DrawText(TextFormat("Attacking: %s", player.mIsAttacking ? "YES" : "NO"), panelX, panelY, 14, player.mIsAttacking ? GREEN : GRAY);
            panelY += lineHeight;
            DrawText(TextFormat("Climbing: %s", player.mIsClimbing ? "YES" : "NO"), panelX, panelY, 14, player.mIsClimbing ? GREEN : GRAY);
            panelY += lineHeight;
            DrawText(TextFormat("TouchWall: %s (dir: %d)", player.mTouchingWall ? "YES" : "NO", player.mWallDir), panelX, panelY, 14, player.mTouchingWall ? GREEN : GRAY);
            panelY += lineHeight;

            // Combo
            DrawText(TextFormat("Combo: %d", player.mComboHit), panelX, panelY, 14, PURPLE);
            panelY += lineHeight;

            // Broñ
            const char* weaponNames[] = { "SWORD", "KATANA", "SPEAR" };
            DrawText(TextFormat("Weapon: %s", weaponNames[player.mCurrentSpecialWeapon]), panelX, panelY, 14, SKYBLUE);
            panelY += lineHeight;

            // Jumps
            DrawText(TextFormat("Jumps: %d / %d", player.mJumpCount, player.mMaxJumps), panelX, panelY, 14, WHITE);
            panelY += lineHeight;

            // Timery
            panelY += 10;
            DrawText("--- Timers ---", panelX, panelY, 12, GRAY);
            panelY += 14;
            DrawText(TextFormat("Invincibility: %.2f", player.mInvincibilityTimer), panelX, panelY, 12, GRAY);
            panelY += 14;
            DrawText(TextFormat("Dash CD: %.2f", player.mDashCooldownTimer), panelX, panelY, 12, GRAY);
            panelY += 14;
            DrawText(TextFormat("Combo Timer: %.2f", player.mComboTimer), panelX, panelY, 12, GRAY);
            panelY += 14;

            // Tryby debug
            panelY += 10;
            DrawText("--- Modes ---", panelX, panelY, 12, GRAY);
            panelY += 14;
            DrawText(TextFormat("SlowMo: %s", slowMotion ? "ON" : "OFF"), panelX, panelY, 12, slowMotion ? YELLOW : GRAY);
            panelY += 14;
            DrawText(TextFormat("GodMode: %s", godMode ? "ON" : "OFF"), panelX, panelY, 12, godMode ? YELLOW : GRAY);
            panelY += 14;
            DrawText(TextFormat("Freeze: %s", freezePlayer ? "ON" : "OFF"), panelX, panelY, 12, freezePlayer ? YELLOW : GRAY);
        }

        // Kontrolki (lewa strona)
        int helpY = 10;
        DrawText("=== CONTROLS ===", 10, helpY, 14, YELLOW);
        helpY += 20;
        DrawText("ARROWS - Move", 10, helpY, 12, WHITE); helpY += 14;
        DrawText("SPACE - Jump", 10, helpY, 12, WHITE); helpY += 14;
        DrawText("C - Dash", 10, helpY, 12, WHITE); helpY += 14;
        DrawText("Z - Attack", 10, helpY, 12, WHITE); helpY += 14;
        helpY += 10;
        DrawText("=== DEBUG ===", 10, helpY, 14, YELLOW);
        helpY += 20;
        DrawText("F1 - Toggle Hitboxes", 10, helpY, 12, LIME); helpY += 14;
        DrawText("F2 - Toggle Stats", 10, helpY, 12, LIME); helpY += 14;
        DrawText("F3 - Slow Motion", 10, helpY, 12, LIME); helpY += 14;
        DrawText("F4 - God Mode", 10, helpY, 12, LIME); helpY += 14;
        DrawText("F5 - Freeze Player", 10, helpY, 12, LIME); helpY += 14;
        DrawText("R - Reset Position", 10, helpY, 12, LIME); helpY += 14;
        DrawText("T - Teleport to Mouse", 10, helpY, 12, LIME); helpY += 14;
        DrawText("H - Heal", 10, helpY, 12, LIME); helpY += 14;
        DrawText("G - Take Damage", 10, helpY, 12, LIME); helpY += 14;
        DrawText("1/2/3 - Change Weapon", 10, helpY, 12, LIME); helpY += 14;
        DrawText("ESC - Exit", 10, helpY, 12, RED);

        DrawFPS(windowWidth - 80, windowHeight - 25);

        EndDrawing();
    }

    UnloadRenderTexture(target);
}