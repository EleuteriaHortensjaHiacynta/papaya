#pragma once
#include <raylib.h>
#include "Saves/game_data.hpp"

// Typ przedmiotu
enum class ItemType {
    DOUBLE_JUMP,
    WAVEDASH
};

class ItemEntity {
public:
    Rectangle rect;
    ItemType type;
    bool collected;

    ItemEntity(float x, float y, ItemType itemType) {
        rect = { x, y, 16.0f, 16.0f }; // rozmiar 32x32
        type = itemType;
        collected = false;
    }

    // rysowanie przedmiotu
    void Draw() {
        if (collected) return;

        Color color = (type == ItemType::DOUBLE_JUMP) ? GOLD : PURPLE;
        DrawRectangleRec(rect, color);
    }

    // Logika kolizji i odblokowania
    void Update(Rectangle playerRect, GameData& data) {
        if (collected) return;

        if (CheckCollisionRecs(playerRect, rect)) {
            collected = true; // przedmiot znika

            // odblokowujemy w game_data.hpp
            if (type == ItemType::DOUBLE_JUMP) {
                data.hasDoubleJump = true;
                // dŸwiêk zebrania ?
            }
            else if (type == ItemType::WAVEDASH) {
                data.hasWavedash = true;
            }
        }
    }
};