#pragma once
#include "save_data.h"
#include "../Entities/Player.h"
#include <fstream>

class SaveManager {
public:
    // Zbierz dane z gracza
    static void collect(SaveData& save, const Player& player) {
        save.hasDoubleJump = player.mCanDoubleJump;
        save.hasWaveDash = player.mCanWavedash;
    }

    // Aplikuj dane do gracza
    static void apply(const SaveData& save, Player& player) {
        player.mCanDoubleJump = save.hasDoubleJump;
        player.mCanWavedash = save.hasWaveDash;

        if (save.checkpointX != 0 || save.checkpointY != 0) {
            player.mPosition = { save.checkpointX, save.checkpointY };
            player.mStartPosition = { save.checkpointX, save.checkpointY };
        }
    }

    // Czy wróg martwy?
    static bool isEnemyDead(const SaveData& save, int id) {
        for (int i : save.deadEnemies) if (i == id) return true;
        return false;
    }

    // Oznacz wroga jako martwego
    static void killEnemy(SaveData& save, int id) {
        if (!isEnemyDead(save, id)) save.deadEnemies.push_back(id);
    }

    // Ustaw checkpoint
    static void setCheckpoint(SaveData& save, float x, float y, const std::string& room) {
        save.checkpointX = x;
        save.checkpointY = y;
        save.checkpointRoom = room;
    }

    // ========== ZAPIS DO PLIKU ==========
    static bool saveToFile(const SaveData& save, const std::string& filename = "save.dat") {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Checkpoint
        file.write((char*)&save.checkpointX, sizeof(float));
        file.write((char*)&save.checkpointY, sizeof(float));

        // Unlocks
        file.write((char*)&save.hasDoubleJump, sizeof(bool));
        file.write((char*)&save.hasWaveDash, sizeof(bool));

        // Boss
        file.write((char*)&save.boss1Defeated, sizeof(bool));

        // Dead enemies - najpierw iloœæ, potem dane
        int enemyCount = (int)save.deadEnemies.size();
        file.write((char*)&enemyCount, sizeof(int));
        for (int id : save.deadEnemies) {
            file.write((char*)&id, sizeof(int));
        }

        // Collected items
        int itemCount = (int)save.collectedItems.size();
        file.write((char*)&itemCount, sizeof(int));
        for (int id : save.collectedItems) {
            file.write((char*)&id, sizeof(int));
        }

        file.close();
        return true;
    }

    // Odczyt z pliku
    static bool loadFromFile(SaveData& save, const std::string& filename = "save.dat") {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        // Checkpoint
        file.read((char*)&save.checkpointX, sizeof(float));
        file.read((char*)&save.checkpointY, sizeof(float));

        // Unlocks
        file.read((char*)&save.hasDoubleJump, sizeof(bool));
        file.read((char*)&save.hasWaveDash, sizeof(bool));

        // Boss
        file.read((char*)&save.boss1Defeated, sizeof(bool));

        // Dead enemies
        int enemyCount = 0;
        file.read((char*)&enemyCount, sizeof(int));
        save.deadEnemies.clear();
        for (int i = 0; i < enemyCount; i++) {
            int id;
            file.read((char*)&id, sizeof(int));
            save.deadEnemies.push_back(id);
        }

        // Collected items
        int itemCount = 0;
        file.read((char*)&itemCount, sizeof(int));
        save.collectedItems.clear();
        for (int i = 0; i < itemCount; i++) {
            int id;
            file.read((char*)&id, sizeof(int));
            save.collectedItems.push_back(id);
        }

        file.close();
        return true;
    }

    // Usuñ save
    static bool deleteSave(const std::string& filename = "save.dat") {
        return std::remove(filename.c_str()) == 0;
    }

    // Czy save istnieje?
    static bool saveExists(const std::string& filename = "save.dat") {
        std::ifstream file(filename);
        return file.good();
    }
};