#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include "../Entities/Wall.h"
#include "../external_headers/json.hpp"

using json = nlohmann::json;

#define BLOCK_SIZE sizeof(uint64_t)

typedef std::array<std::array<std::string, 64>, 64> chunkTiles;

struct RenderTile {
    int x, y;
    uint16_t textureID; // ZMIANA: uint8_t -> uint16_t (mieœci do 65535)
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
    uint16_t textureID; // ZMIANA: uint8_t -> uint16_t
    Layers layer;
    ExtraData extraData;
};

uint64_t createBlock(Block block);
Block decodeBlock(uint64_t blockData);

class MapSaver {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit MapSaver(std::fstream& f);
    void addBlock(Block block);
    void sortBlocks();
    void fromEditor(std::string json);
    ~MapSaver();
};

class MapLoader {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit MapLoader(std::fstream& f);
    std::vector<Wall> getAll();
    std::vector<RenderTile> getRenderData();
    ~MapLoader();
};