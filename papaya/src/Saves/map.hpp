#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include "../Entities/Wall.h"

#define BLOCK_SIZE sizeof(uint64_t)

// string[64][64]
typedef std::array<std::array<std::string, 64>, 64> chunkTiles;

struct Chunk {
    int chunkX;
    int chunkY;
    chunkTiles data;
};

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
    int16_t x;
    int16_t y;
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
    void fromEditor(std::string json, int chunkX, int chunkY);
    ~MapSaver();
};

// Class to load blocks from a file
class MapLoader {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit MapLoader(std::fstream& f);
    std::vector<Wall> getAll();
    std::vector<Chunk> toChunks();
    std::string toEditorJson();

    ~MapLoader();
};