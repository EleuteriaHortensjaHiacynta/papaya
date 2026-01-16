#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>

const int TILE_SIZE = 8;

enum class Layers : uint8_t {
    BACKGROUND = 0,
    MIDGROUND = 1,
    FOREGROUND = 2
};

enum class ExtraData : uint8_t {
    NONE = 0,
    COLLIDABLE = 1,
    DAMAGING = 2
};

struct Block {
    int16_t x;
    int16_t y;
    uint8_t x_length;
    uint8_t y_length;
    uint16_t textureID;
    ExtraData extraData;
    Layers layer;
};

// ZMIANA NAZWY: Wall -> CollisionRect
struct CollisionRect {
    int x, y, w, h;
    CollisionRect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
};

struct RenderTile {
    int x, y;
    uint16_t textureID;
};

// Funkcje pomocnicze
uint64_t createBlock(Block block);
Block decodeBlock(uint64_t blockData);

class MapSaver {
private:
    std::fstream* fileStream;

public:
    explicit MapSaver(std::fstream& f);
    ~MapSaver() = default;

    void addBlock(Block block);
    void sortBlocks();
    void fromEditor(const std::string& jsonStr);
};

class MapLoader {
private:
    std::fstream* fileStream;

public:
    explicit MapLoader(std::fstream& f);
    ~MapLoader() = default;

    // ZMIANA: Zwraca CollisionRect zamiast Wall
    std::vector<CollisionRect> getCollisions();
    std::vector<RenderTile> getRenderData();
};