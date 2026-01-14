#include <string>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <nlohmann/json.hpp>

#include "map.hpp"

const int TILE_SIZE = 8;

// TEJ STRUKTURY UŻYWAŁEŚ BŁĘDNIE WCZEŚNIEJ
struct EditorBlock {
    bool collision;
    bool damage;
    int layer;
    int textureID; // ZMIANA: uint8_t -> int (żeby przyjąć ID z JSON np. 300)
};

Block EditorBlockToBlock(const EditorBlock& eBlock, uint16_t x, uint16_t y, uint8_t x_length, uint8_t y_length) {
    Block block;
    block.x = x;
    block.y = y;
    block.x_length = x_length;
    block.y_length = y_length;
    block.textureID = static_cast<uint16_t>(eBlock.textureID); // Bezpieczne rzutowanie
    block.layer = Layers::BACKGROUND; // Domyślnie
    if (eBlock.collision) {
        block.extraData = ExtraData::COLLIDABLE;
    }
    else if (eBlock.damage) {
        block.extraData = ExtraData::DAMAGING;
    }
    else {
        block.extraData = ExtraData::NONE;
    }
    // Nadpisz layer jeśli jest w edytorze
    block.layer = static_cast<Layers>(eBlock.layer);
    return block;
}

uint64_t createBlock(Block block) {
    // Zabezpieczenia (opcjonalne, ale warto wiedzieć o limitach)
    if (block.x_length > 63) block.x_length = 63; // Limit 6 bitów
    if (block.y_length > 63) block.y_length = 63; // Limit 6 bitów
    if (block.textureID > 4095) block.textureID = 4095; // Limit 12 bitów

    uint64_t blockData = 0;

    // NOWY UKŁAD BITÓW (Suma 64 bity)
    // X (16) | Y (16) | TexID (12) | X_Len (6) | Y_Len (6) | Extra (4) | Layer (4)

    blockData |= (static_cast<uint64_t>(static_cast<int16_t>(block.x)) & 0xFFFFULL) << 48; // bity 48-63
    blockData |= (static_cast<uint64_t>(static_cast<int16_t>(block.y)) & 0xFFFFULL) << 32; // bity 32-47

    blockData |= (static_cast<uint64_t>(block.textureID) & 0xFFFULL) << 20; // bity 20-31 (12 bitów!)

    blockData |= (static_cast<uint64_t>(block.x_length) & 0x3FULL) << 14;   // bity 14-19 (6 bitów)
    blockData |= (static_cast<uint64_t>(block.y_length) & 0x3FULL) << 8;    // bity 8-13 (6 bitów)

    blockData |= (static_cast<uint64_t>(block.extraData) & 0xFULL) << 4;    // bity 4-7
    blockData |= (static_cast<uint64_t>(block.layer) & 0xFULL) << 0;        // bity 0-3

    return blockData;
}

Block decodeBlock(uint64_t blockData) {
    Block block;
    block.x = static_cast<int16_t>((blockData >> 48) & 0xFFFF);
    block.y = static_cast<int16_t>((blockData >> 32) & 0xFFFF);

    block.textureID = static_cast<uint16_t>((blockData >> 20) & 0xFFF); // Odkoduj 12 bitów

    block.x_length = static_cast<uint8_t>((blockData >> 14) & 0x3F);    // Odkoduj 6 bitów
    block.y_length = static_cast<uint8_t>((blockData >> 8) & 0x3F);     // Odkoduj 6 bitów

    block.extraData = static_cast<ExtraData>((blockData >> 4) & 0x0F);
    block.layer = static_cast<Layers>(blockData & 0x0F);

    return block;
}

bool sortByX(uint64_t a, uint64_t b) {
    int16_t ax = static_cast<int16_t>((a >> 48) & 0xFFFF);
    int16_t bx = static_cast<int16_t>((b >> 48) & 0xFFFF);
    return ax < bx;
}

MapSaver::MapSaver(std::fstream& f) : fileStream(&f) {}

void MapSaver::addBlock(Block block) {
    uint64_t blockData = createBlock(block);
    fileStream->write(reinterpret_cast<const char*>(&blockData), sizeof(blockData));
    if (!(*fileStream)) throw std::runtime_error("Failed to write block data to file.");
}

void MapSaver::sortBlocks() {
    fileStream->seekg(0, std::ios::end);
    std::streamsize fileSize = fileStream->tellg();
    if (fileSize <= 0) return;
    size_t blockCount = static_cast<size_t>(fileSize) / sizeof(uint64_t);
    fileStream->seekg(0, std::ios::beg);
    std::vector<uint64_t> blocks(blockCount);
    fileStream->read(reinterpret_cast<char*>(blocks.data()), fileSize);

    std::sort(blocks.begin(), blocks.end(), sortByX);

    fileStream->clear();
    fileStream->seekp(0, std::ios::beg);
    fileStream->write(reinterpret_cast<const char*>(blocks.data()), static_cast<std::streamsize>(blocks.size() * sizeof(uint64_t)));
}

void MapSaver::fromEditor(std::string jsonStr) {
    using jsonn = nlohmann::json;
    auto doc = jsonn::parse(jsonStr);

    if (!doc.contains("chunkData")) return;

    auto arr = doc["chunkData"]["array"];
    int chunkX = doc["chunkData"]["x"].get<int>();
    int chunkY = doc["chunkData"]["y"].get<int>();

    if (arr.is_null() || !arr.is_array() || arr.size() == 0) return;

    // i = indeks wiersza (Y)
    for (long unsigned int i = 0; i < arr.size(); ++i) {
        // j = indeks kolumny (X)
        for (long unsigned int j = 0; j < arr[i].size(); ++j) {
            auto item = arr[i][j];

            EditorBlock eBlock;
            eBlock.collision = item.value("collision", false);
            eBlock.damage = item.value("damage", false);
            eBlock.layer = item.value("layer", 0);
            eBlock.textureID = item.value("textureID", 0);

            // --- TU BYŁ BŁĄD: ZAMIANA X z Y ---
            // Poprawnie: j to X (kolumna), i to Y (wiersz)
            uint16_t x = j + static_cast<uint16_t>(chunkX * 64);
            uint16_t y = i + static_cast<uint16_t>(chunkY * 64);
            // ----------------------------------

            Block block = EditorBlockToBlock(eBlock, x, y, 1, 1);

            if (block.textureID == 0 && block.extraData == ExtraData::NONE) continue;

            this->addBlock(block);
        }
    }
}

MapSaver::~MapSaver() {
    try { this->sortBlocks(); }
    catch (...) {}
}

MapLoader::MapLoader(std::fstream& f) : fileStream(&f) {}
MapLoader::~MapLoader() {}

std::vector<Wall> MapLoader::getAll() {
    fileStream->clear();
    fileStream->seekg(0, std::ios::beg);
    std::vector<Wall> walls;
    uint64_t blockData;
    while (fileStream->read(reinterpret_cast<char*>(&blockData), sizeof(blockData))) {
        Block block = decodeBlock(blockData);
        if (block.extraData == ExtraData::COLLIDABLE) {
            walls.push_back(Wall(block.x * TILE_SIZE, block.y * TILE_SIZE, block.x_length * TILE_SIZE, block.y_length * TILE_SIZE));
        }
    }
    return walls;
}

std::vector<RenderTile> MapLoader::getRenderData() {
    fileStream->clear();
    fileStream->seekg(0, std::ios::beg);
    std::vector<RenderTile> tiles;
    uint64_t blockData;
    while (fileStream->read(reinterpret_cast<char*>(&blockData), sizeof(blockData))) {
        Block block = decodeBlock(blockData);
        if (block.textureID > 0) {
            RenderTile tile;
            tile.x = block.x * TILE_SIZE;
            tile.y = block.y * TILE_SIZE;
            tile.textureID = block.textureID;
            tiles.push_back(tile);
        }
    }
    return tiles;
}