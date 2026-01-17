#include "map.hpp"
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
    struct EditorBlock {
        int textureID = 0;
        bool collision = false;
        bool damage = false;
        int layer = 0;
    };

    Block editorBlockToBlock(const EditorBlock& eBlock, uint16_t x, uint16_t y) {
        Block block;
        block.x = static_cast<int16_t>(x);
        block.y = static_cast<int16_t>(y);
        block.x_length = 1;
        block.y_length = 1;
        block.textureID = static_cast<uint16_t>(eBlock.textureID);
        block.layer = static_cast<Layers>(eBlock.layer);

        if (eBlock.collision) {
            block.extraData = ExtraData::COLLIDABLE;
        }
        else if (eBlock.damage) {
            block.extraData = ExtraData::DAMAGING;
        }
        else {
            block.extraData = ExtraData::NONE;
        }

        return block;
    }

    bool sortByX(uint64_t a, uint64_t b) {
        uint16_t ax = static_cast<uint16_t>((a >> 48) & 0xFFFF);
        uint16_t bx = static_cast<uint16_t>((b >> 48) & 0xFFFF);
        return ax < bx;
    }
}

uint64_t createBlock(Block block) {
    if (block.x_length > 63) block.x_length = 63;
    if (block.y_length > 63) block.y_length = 63;
    if (block.textureID > 4095) block.textureID = 4095;

    uint64_t data = 0;
    data |= (static_cast<uint64_t>(static_cast<uint16_t>(block.x)) & 0xFFFFULL) << 48;
    data |= (static_cast<uint64_t>(static_cast<uint16_t>(block.y)) & 0xFFFFULL) << 32;
    data |= (static_cast<uint64_t>(block.textureID) & 0xFFFULL) << 20;
    data |= (static_cast<uint64_t>(block.x_length) & 0x3FULL) << 14;
    data |= (static_cast<uint64_t>(block.y_length) & 0x3FULL) << 8;
    data |= (static_cast<uint64_t>(block.extraData) & 0xFULL) << 4;
    data |= (static_cast<uint64_t>(block.layer) & 0xFULL);

    return data;
}

Block decodeBlock(uint64_t data) {
    Block block;
    block.x = static_cast<int16_t>((data >> 48) & 0xFFFF);
    block.y = static_cast<int16_t>((data >> 32) & 0xFFFF);
    block.textureID = static_cast<uint16_t>((data >> 20) & 0xFFF);
    block.x_length = static_cast<uint8_t>((data >> 14) & 0x3F);
    block.y_length = static_cast<uint8_t>((data >> 8) & 0x3F);
    block.extraData = static_cast<ExtraData>((data >> 4) & 0x0F);
    block.layer = static_cast<Layers>(data & 0x0F);

    return block;
}

MapSaver::MapSaver(std::fstream& f) : fileStream(&f) {}

void MapSaver::addBlock(Block block) {
    uint64_t blockData = createBlock(block);
    fileStream->seekp(0, std::ios::end);
    fileStream->write(reinterpret_cast<const char*>(&blockData), sizeof(blockData));
}

void MapSaver::sortBlocks() {
    fileStream->flush();
    fileStream->seekg(0, std::ios::end);
    std::streamsize fileSize = fileStream->tellg();

    if (fileSize <= 0) return;

    size_t blockCount = static_cast<size_t>(fileSize) / sizeof(uint64_t);
    std::vector<uint64_t> blocks(blockCount);

    fileStream->seekg(0, std::ios::beg);
    fileStream->read(reinterpret_cast<char*>(blocks.data()), fileSize);

    std::sort(blocks.begin(), blocks.end(), sortByX);

    fileStream->seekp(0, std::ios::beg);
    fileStream->write(reinterpret_cast<const char*>(blocks.data()),
        static_cast<std::streamsize>(blocks.size() * sizeof(uint64_t)));
    fileStream->flush();
}

void MapSaver::fromEditor(const std::string& jsonStr) {
    using json = nlohmann::json;

    try {
        auto doc = json::parse(jsonStr);

        if (!doc.contains("chunkData")) {
            std::cerr << "[ERROR] Brak klucza 'chunkData' w JSON" << std::endl;
            return;
        }

        const auto& chunkData = doc["chunkData"];
        const auto& arr = chunkData["array"];
        int chunkX = chunkData["x"].get<int>();
        int chunkY = chunkData["y"].get<int>();

        if (!arr.is_array() || arr.empty()) {
            std::cerr << "[WARNING] Pusta tablica w chunku (" << chunkX << "," << chunkY << ")" << std::endl;
            return;
        }

        int blocksAdded = 0;

        for (size_t row = 0; row < arr.size(); ++row) {
            for (size_t col = 0; col < arr[row].size(); ++col) {
                const auto& item = arr[row][col];

                EditorBlock eBlock;
                eBlock.textureID = item.value("textureID", item.value("id", 0));
                eBlock.collision = item.value("collision", false);
                eBlock.damage = item.value("damage", false);
                eBlock.layer = item.value("layer", 0);

                if (eBlock.textureID == 0 && !eBlock.collision && !eBlock.damage) {
                    continue;
                }

                int globalX = static_cast<int>(col) + (chunkX * 64);
                int globalY = static_cast<int>(row) + (chunkY * 64);

                Block block = editorBlockToBlock(eBlock,
                    static_cast<uint16_t>(globalX),
                    static_cast<uint16_t>(globalY));

                addBlock(block);
                blocksAdded++;
            }
        }

        fileStream->flush();
        std::cout << "[MAP] Chunk (" << chunkX << "," << chunkY << ") -> " << blocksAdded << " bloków" << std::endl;

    }
    catch (const std::exception& e) {
        std::cerr << "[ERROR] fromEditor: " << e.what() << std::endl;
    }
}

MapLoader::MapLoader(std::fstream& f) : fileStream(&f) {}

std::vector<CollisionRect> MapLoader::getCollisions() {
    fileStream->clear();
    fileStream->seekg(0, std::ios::beg);

    std::vector<CollisionRect> collisions;
    uint64_t blockData;

    while (fileStream->read(reinterpret_cast<char*>(&blockData), sizeof(blockData))) {
        Block block = decodeBlock(blockData);
        if (block.extraData == ExtraData::COLLIDABLE) {
            collisions.emplace_back(
                block.x * TILE_SIZE,
                block.y * TILE_SIZE,
                block.x_length * TILE_SIZE,
                block.y_length * TILE_SIZE
            );
        }
    }

    fileStream->clear();
    return collisions;
}

std::vector<CollisionRect> MapLoader::getDamagingZones() {
    fileStream->clear();
    fileStream->seekg(0, std::ios::beg);

    std::vector<CollisionRect> zones;
    uint64_t blockData;

    while (fileStream->read(reinterpret_cast<char*>(&blockData), sizeof(blockData))) {
        Block block = decodeBlock(blockData);
        if (block.extraData == ExtraData::DAMAGING) {
            zones.emplace_back(
                block.x * TILE_SIZE,
                block.y * TILE_SIZE,
                block.x_length * TILE_SIZE,
                block.y_length * TILE_SIZE
            );
        }
    }

    fileStream->clear();
    return zones;
}

std::vector<RenderTile> MapLoader::getRenderData() {
    fileStream->clear();
    fileStream->seekg(0, std::ios::beg);

    std::vector<RenderTile> tiles;
    uint64_t blockData;

    while (fileStream->read(reinterpret_cast<char*>(&blockData), sizeof(blockData))) {
        Block block = decodeBlock(blockData);
        if (block.textureID > 0) {
            tiles.push_back({
                block.x * TILE_SIZE,
                block.y * TILE_SIZE,
                block.textureID
                });
        }
    }

    fileStream->clear();
    return tiles;
}