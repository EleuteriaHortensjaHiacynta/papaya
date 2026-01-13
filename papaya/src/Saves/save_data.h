#pragma once
#include <vector>
#include <string>

struct SaveData {
    // Pozycja checkpointu (respawn point)
    float checkpointX = 0.0f;
    float checkpointY = 0.0f;
    std::string checkpointRoom = "";

    // Unlockables
    bool hasDoubleJump = false;
    bool hasWaveDash = false;

    // Postêp
    std::vector<int> deadEnemies;      // ID zabitych wrogów (którzy nie respawnuj¹)
    std::vector<int> collectedItems;   // ID zebranych przedmiotów

    // Bossy
    bool boss1Defeated = false;
};