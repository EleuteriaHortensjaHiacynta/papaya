#pragma once

#include "raylib.h"
#include <vector>

namespace Physics {

    inline bool CheckCollisionLineRec(Vector2 startPos, Vector2 endPos, Rectangle rec) {
        Vector2 collisionPoint;

        Vector2 tl = { rec.x, rec.y };
        Vector2 tr = { rec.x + rec.width, rec.y };
        Vector2 bl = { rec.x, rec.y + rec.height };
        Vector2 br = { rec.x + rec.width, rec.y + rec.height };

        if (CheckCollisionLines(startPos, endPos, tl, tr, &collisionPoint)) return true;
        if (CheckCollisionLines(startPos, endPos, bl, br, &collisionPoint)) return true;
        if (CheckCollisionLines(startPos, endPos, tl, bl, &collisionPoint)) return true;
        if (CheckCollisionLines(startPos, endPos, tr, br, &collisionPoint)) return true;
        if (CheckCollisionPointRec(startPos, rec)) return true;

        return false;
    }

    template<typename T>
    inline bool IsPathBlocked(Vector2 start, Vector2 end, const std::vector<T*>& walls) {
        for (const auto* wall : walls) {
            Rectangle wallRect = {
                wall->mPosition.x,
                wall->mPosition.y,
                wall->mSize.x,
                wall->mSize.y
            };
            if (CheckCollisionLineRec(start, end, wallRect)) {
                return true;
            }
        }
        return false;
    }

    template<typename T>
    inline Vector2 GetEntityCenter(const T* entity) {
        return {
            entity->mPosition.x + entity->mSize.x / 2.0f,
            entity->mPosition.y + entity->mSize.y / 2.0f
        };
    }

    inline float GetKnockbackDirection(float sourceX, float targetX) {
        return (targetX < sourceX) ? -1.0f : 1.0f;
    }

    inline void ApplyKnockback(Vector2& velocity, float direction, float horizontalForce, float verticalForce) {
        velocity.x = direction * horizontalForce;
        velocity.y = -verticalForce;
    }
}