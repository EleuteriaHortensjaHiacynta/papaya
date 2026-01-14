#include "entities.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

#include "../Entities/Enemy.h"
#include "../Entities/Entity.h"
#include "../Entities/Player.h"
#include "../external_headers/json.hpp"


// Sort comparator: by x then y
static bool sortByXY(uint64_t a, uint64_t b) {
    EntityS ea = decodeEntityS(a);
    EntityS eb = decodeEntityS(b);
    if (ea.x != eb.x) return ea.x < eb.x;
    return ea.y < eb.y;
}

uint64_t createEntityS(EntityS entity) {
    uint64_t entityData = 0;
    // Layout (bits): [63..48]=x (16) | [47..32]=y (16) | [31..24]=x_length (8) | [23..16]=y_length (8)
    //                 [15..8]=textureID (8) | [7..4]=extraData (4) | [3..0]=layer (4)
    entityData |= (static_cast<uint64_t>(static_cast<int16_t>(entity.x)) & 0xFFFFULL) << 48;
    entityData |= (static_cast<uint64_t>(static_cast<int16_t>(entity.y)) & 0xFFFFULL) << 32;
    entityData |= (static_cast<uint64_t>(entity.entityType) & 0xFFULL) << 24;
    entityData |= (static_cast<uint64_t>(entity.health) & 0xFFULL) << 16;
    return entityData;
}

EntityS decodeEntityS(uint64_t entityData) {
    EntityS entity;
    entity.x = static_cast<int16_t>((entityData >> 48) & 0xFFFF);
    entity.y = static_cast<int16_t>((entityData >> 32) & 0xFFFF);
    entity.entityType = static_cast<EntityType>((entityData >> 24) & 0xFF);
    entity.health = static_cast<int8_t>((entityData >> 16) & 0xFF);
    return entity;
}

std::unique_ptr<Entity> EntitySToEntity(const EntityS& entityS) {
    float x = static_cast<float>(entityS.x);
    float y = static_cast<float>(entityS.y);

    switch (entityS.entityType) {
    case PLAYER:
        return std::make_unique<Player>(x, y);
    case ENEMY:
        return std::make_unique<Enemy>(x, y);
        // Jeœli napotkasz coœ innego (np. WALL w z³ym miejscu, albo b³¹d danych):
    default:
        std::cout << "[WARNING] Unknown EntityType ID: " << (int)entityS.entityType << " at " << x << "," << y << std::endl;
        return nullptr; // Zwracamy pusty wskaŸnik zamiast crashowaæ grê
    }
}
EntitySaver::EntitySaver(std::fstream& f) : fileStream(&f) {}

void EntitySaver::addEntityS(EntityS entity) {
    uint64_t block = createEntityS(entity);
    fileStream->write(reinterpret_cast<const char*>(&block), sizeof(block));
    if (!(*fileStream)) throw std::runtime_error("Failed to write entity to stream");
}

void EntitySaver::addEntity(const std::unique_ptr<Entity>& entity) {
    EntityS entityS;
    entityS.x = static_cast<uint16_t>(entity->mPosition.x);
    entityS.y = static_cast<uint16_t>(entity->mPosition.y);
    entityS.entityType = entity->mType;
    entityS.health = static_cast<uint8_t>(entity->mHealth);

    addEntityS(entityS);
}

void EntitySaver::fromEditor(std::string json) {
    using jsonn = nlohmann::json;
    auto doc = jsonn::parse(json);
    auto arr = doc["chunkData"]["entities"];
    int chunkX = doc["chunkData"]["x"].get<int>();
    int chunkY = doc["chunkData"]["y"].get<int>();
    if (arr.is_null() || !arr.is_array() || arr.size() == 0) {
        throw std::invalid_argument("Invalid JSON: 'chunkData.entities' is missing or not an array.");
    }

    for (long unsigned int i = 0; i < arr.size(); ++i) {
        for (long unsigned int j = 0; j < arr[i].size(); ++j) {
        uint8_t item = arr[i][j];

        if (item == 0) continue; // Pomijamy puste miejsca
        std::cout << "Entity item: " << static_cast<int>(item) << " at chunk pos (" << i << ", " << j << ")\n";
        
        uint16_t x = i + static_cast<uint16_t>(chunkX * 64);
        uint16_t y = j + static_cast<uint16_t>(chunkY * 64);

        EntityS entity = {
            x,
            y,
            static_cast<EntityType>(item),
            0
        };
        this->addEntityS(entity);
        }
    }
}

// void EntitySaver::sortEntities() {
//     fileStream->seekg(0, std::ios::end);
//     std::streamsize size = fileStream->tellg();
//     if (size <= 0) return;
//     size_t count = static_cast<size_t>(size) / sizeof(uint64_t);
//     fileStream->seekg(0, std::ios::beg);
//     std::vector<uint64_t> items(count);
//     fileStream->read(reinterpret_cast<char*>(items.data()), size);
//     if (!(*fileStream)) throw std::runtime_error("Failed to read entities for sorting");
//     std::sort(items.begin(), items.end(), sortByXY);
//     fileStream->clear();
//     fileStream->seekp(0, std::ios::beg);
//     fileStream->write(reinterpret_cast<const char*>(items.data()), static_cast<std::streamsize>(items.size() * sizeof(uint64_t)));
//     if (!(*fileStream)) throw std::runtime_error("Failed to write sorted entities to stream");
// }

EntitySaver::~EntitySaver() {
    // sortEntities();
}

EntityLoader::EntityLoader(std::fstream& f) : fileStream(&f) {}

// std::vector<std::unique_ptr<Entity>> EntityLoader::getAll() {
//     std::vector<std::unique_ptr<Entity>> entities;
//     fileStream->seekg(0);

//     uint64_t entityData;
//     while(fileStream->read(reinterpret_cast<char*>(&entityData), sizeof(entityData))) {
//         EntityS entity = decodeEntityS(entityData);
//         entities.push_back(EntitySToEntity(entity));
//     }
//     return entities;
// }

std::vector<EntityS> EntityLoader::getAll() {
    std::vector<EntityS> entities;
    fileStream->seekg(0);

    uint64_t entityData;
    while(fileStream->read(reinterpret_cast<char*>(&entityData), sizeof(entityData))) {
        EntityS entity = decodeEntityS(entityData);
        entities.push_back(entity);
    }
    return entities;
}

EntityLoader::~EntityLoader() {}
