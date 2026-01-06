#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "map.hpp"

uint64_t createBlock(Block block) {
    if (block.layer < Layers::GUI || block.layer > Layers::BACKGROUND) {
        throw std::invalid_argument("Invalid layer value.");
    }
    if (block.extraData < ExtraData::NONE || block.extraData > ExtraData::INTERACTIVE) {
        throw std::invalid_argument("extraData must be a valid ExtraData value.");
    }
    uint64_t blockData = 0;
    // Layout (bits): [63..48]=x (16) | [47..32]=y (16) | [31..24]=x_length (8) | [23..16]=y_length (8)
    //                 [15..8]=textureID (8) | [7..4]=extraData (4) | [3..0]=layer (4)
    blockData |= (static_cast<uint64_t>(static_cast<uint16_t>(block.x)) & 0xFFFFULL) << 48;
    blockData |= (static_cast<uint64_t>(static_cast<uint16_t>(block.y)) & 0xFFFFULL) << 32;
    blockData |= (static_cast<uint64_t>(block.x_length) & 0xFFULL) << 24;
    blockData |= (static_cast<uint64_t>(block.y_length) & 0xFFULL) << 16;
    blockData |= (static_cast<uint64_t>(block.textureID) & 0xFFULL) << 8;
    blockData |= (static_cast<uint64_t>(block.extraData) & 0xFULL) << 4;
    blockData |= (static_cast<uint64_t>(block.layer) & 0xFULL) << 0;
    return blockData;
}

Block decodeBlock(uint64_t blockData) {
    Block block;
    block.x = static_cast<int16_t>((blockData >> 48) & 0xFFFF);
    block.y = static_cast<int16_t>((blockData >> 32) & 0xFFFF);
    block.x_length = static_cast<int8_t>((blockData >> 24) & 0xFF);
    block.y_length = static_cast<int8_t>((blockData >> 16) & 0xFF);
    block.textureID = static_cast<int8_t>((blockData >> 8) & 0xFF);
    block.extraData = static_cast<ExtraData>((blockData >> 4) & 0x0F);
    block.layer = static_cast<Layers>(blockData & 0x0F);
    return block;
}

bool sortByX(uint64_t a, uint64_t b) {
    int16_t ax = static_cast<int16_t>((a >> 48) & 0xFFFF);
    int16_t bx = static_cast<int16_t>((b >> 48) & 0xFFFF);
    return ax < bx;
}

// MapSaver method implementations
MapSaver::MapSaver(std::fstream& f) : fileStream(&f) {}

void MapSaver::addBlock(Block block) {
    uint64_t blockData = createBlock(block);
    fileStream->write(reinterpret_cast<const char*>(&blockData), sizeof(blockData));
    if (!(*fileStream)) throw std::runtime_error("Failed to write block data to file.");
}

// Sort blocks by their x coordinate
void MapSaver::sortBlocks() {
    fileStream->seekg(0, std::ios::end);
    std::streamsize fileSize = fileStream->tellg();
    if (fileSize <= 0) return;
    size_t blockCount = static_cast<size_t>(fileSize) / sizeof(uint64_t);
    fileStream->seekg(0, std::ios::beg);
    std::vector<uint64_t> blocks(blockCount);
    fileStream->read(reinterpret_cast<char*>(blocks.data()), fileSize);
    if (!(*fileStream)) throw std::runtime_error("Failed to read block data from file.");

    std::sort(blocks.begin(), blocks.end(), sortByX);

    // (optional) write sorted data back to file
    fileStream->clear();
    fileStream->seekp(0, std::ios::beg);
    fileStream->write(reinterpret_cast<const char*>(blocks.data()), static_cast<std::streamsize>(blocks.size() * sizeof(uint64_t)));
    if (!(*fileStream)) throw std::runtime_error("Failed to write sorted block data to file.");
}

MapSaver::~MapSaver() {
    try { this->sortBlocks(); } catch (...) {}
}

    // dziala ale w sumie komu to potrzebne 
    // struct Iterator {
    //     using iterator_category = std::forward_iterator_tag;
    //     using difference_type = std::ptrdiff_t;
    //     using value_type = uint64_t;
    //     using pointer = const uint64_t*;
    //     using reference = const uint64_t&;

    //     std::istream* file = nullptr;
    //     uint64_t currentBlock = 0;
    //     bool at_end = true;

    //     Iterator(std::istream* f, bool readFirst) : file(f), currentBlock(0), at_end(false) {
    //         if (readFirst) {
    //             file->read(reinterpret_cast<char*>(&currentBlock), sizeof(currentBlock));
    //             if (static_cast<std::streamsize>(sizeof(currentBlock)) != file->gcount()) at_end = true;
    //         } else {
    //             at_end = true;
    //         }
    //     }

    //     reference operator*() const { return currentBlock; }
    //     pointer operator->() const { return &currentBlock; }
    //     Iterator& operator++() {
    //         if (file) {
    //             file->read(reinterpret_cast<char*>(&currentBlock), sizeof(currentBlock));
    //             if (static_cast<std::streamsize>(sizeof(currentBlock)) != file->gcount()) at_end = true;
    //             return *this;
    //         }
    //         at_end = true;
    //         return *this;
    //     }

    //     Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

    //     bool operator==(const Iterator& other) const { return at_end == other.at_end; }
    //     bool operator!=(const Iterator& other) const { return !(*this == other); }
    // };

    // Iterator begin() {
    //     fileStream->clear();
    //     fileStream->seekg(0, std::ios::beg);
    //     return Iterator(fileStream, true);
    // }

    // Iterator end() {
    //     return Iterator(fileStream, false);
    // }

// MapLoader method implementations
MapLoader::MapLoader(std::fstream& f) : fileStream(&f) {}

MapLoader::~MapLoader() {}

std::vector<Wall> MapLoader::getAll() {
    fileStream->seekg(0, std::ios::beg);
    std::vector<Wall> walls;
    uint64_t blockData;
    while (fileStream->read(reinterpret_cast<char*>(&blockData), sizeof(blockData))) {
        Block block = decodeBlock(blockData);
        walls.push_back(Wall(block.x, block.y, block.x_length * 10, block.y_length * 10));
    }
    return walls;
}