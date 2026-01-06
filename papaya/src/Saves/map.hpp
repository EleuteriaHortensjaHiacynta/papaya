#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include "../Entities/Wall.h"

#define BLOCK_SIZE sizeof(uint64_t)

enum class Layers : uint8_t {
    GUI = 0,
    PLAYER = 1,
    ENEMIES = 2,
    BACKGROUND = 3,
};

enum class ExtraData : uint8_t {
    NONE = 0,
    COLLIDABLE = 1,
    DAMAGING = 2,
    INTERACTIVE = 3,
};

struct Block {
    uint16_t x;
    uint16_t y;
    uint8_t x_length;
    uint8_t y_length;
    int8_t textureID;
    Layers layer;
    ExtraData extraData;
};

uint64_t createBlock(Block block);
Block decodeBlock(uint64_t blockData);  

// Class to save blocks to a file
class MapSaver {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit MapSaver(std::fstream& f);
    void addBlock(Block block);
    void sortBlocks();
    ~MapSaver();
};

// Class to load blocks from a file
class MapLoader {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit MapLoader(std::fstream& f);
    std::vector<Wall> getAll();
    ~MapLoader();
};