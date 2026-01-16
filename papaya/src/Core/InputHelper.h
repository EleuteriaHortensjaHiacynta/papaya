#pragma once
#include "raylib.h"

namespace Input {
    inline bool IsMovingRight() {
        return IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
    }

    inline bool IsMovingLeft() {
        return IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    }

    inline bool IsMovingUp() {
        return IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    }

    inline bool IsMovingDown() {
        return IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    }

    inline float GetHorizontalAxis() {
        float dir = 0.0f;
        if (IsMovingRight()) dir += 1.0f;
        if (IsMovingLeft()) dir -= 1.0f;
        return dir;
    }

    inline float GetVerticalAxis() {
        float dir = 0.0f;
        if (IsMovingDown()) dir += 1.0f;
        if (IsMovingUp()) dir -= 1.0f;
        return dir;
    }

    inline bool JumpPressed() {
        return IsKeyPressed(KEY_SPACE);
    }

    inline bool JumpHeld() {
        return IsKeyDown(KEY_SPACE);
    }

    inline bool DashPressed() {
        return IsKeyPressed(KEY_C);
    }

    inline bool AttackPressed() {
        return IsKeyPressed(KEY_Z);
    }

    inline bool SacrificePressed() {
        return IsKeyPressed(KEY_H);
    }
}