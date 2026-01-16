#pragma once
#include <cmath>

namespace MathUtils {
    inline float Approach(float current, float target, float delta) {
        if (current < target) {
            return std::min(current + delta, target);
        }
        return std::max(current - delta, target);
    }

    inline float Sign(float value) {
        if (value > 0.0f) return 1.0f;
        if (value < 0.0f) return -1.0f;
        return 0.0f;
    }

    inline float Clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    inline Vector2 Normalize(Vector2 v) {
        float length = std::sqrt(v.x * v.x + v.y * v.y);
        if (length > 0.0f) {
            return { v.x / length, v.y / length };
        }
        return { 0.0f, 0.0f };
    }
}