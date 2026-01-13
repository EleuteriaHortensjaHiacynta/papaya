#pragma once
#include <raylib.h>
#include "Saves/game_data.hpp"
#include "Saves/save_system.hpp"

class CheckpointEntity {
public:
    Rectangle rect;
    bool isActive;

    CheckpointEntity(float x, float y) {
        rect = { x, y, 20.0f, 10.0f };
        isActive = false;
    }

    void Update(Rectangle playerRect, GameData& data) {
        // sprawdzamy kolizjê z graczem
        if (CheckCollisionRecs(playerRect, rect)) {
            if (!isActive) {
                // ustawiamy ten punkt jako nowy spawn
                data.respawnX = rect.x;
                data.respawnY = rect.y;
                isActive = true;
                // gra zapisuje siê automatycznie przye checkpointach
                SaveSystem::SaveGame(data);
            }
        }
        else {
            if (data.respawnX != rect.x || data.respawnY != rect.y) {
                isActive = false;
            }
        }
    }

    void Draw() {
        // rysowanie: Zielony jeœli aktywny, Czerwony jeœli nieaktywny
        Color color = isActive ? GREEN : RED;

        DrawRectangleRec(rect, color);

        // Jakiœ prosty tekst dla debugu
        if (isActive) DrawText("ACTIVE SPAWN", (int)rect.x - 10, (int)rect.y - 20, 10, GREEN);
        else DrawText("CHECKPOINT", (int)rect.x, (int)rect.y - 20, 10, GRAY);
    }
};