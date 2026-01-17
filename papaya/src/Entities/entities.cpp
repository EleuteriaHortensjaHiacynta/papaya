#include "entities.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

#include "../Entities/Enemy.h"
#include "../Entities/Entity.h"
#include "../Entities/Player/Player.h"
#include "../external_headers/json.hpp"
#include "../Entities/Enemies/mage_boss.h"
#include "../Entities/Enemies/rabbit.h"

static bool sortByXY(uint64_t a, uint64_t b) {
    EntityS ea = decodeEntityS(a);
    EntityS eb = decodeEntityS(b);
    if (ea.x != eb.x) return ea.x < eb.x;
    return ea.y < eb.y;
}

uint64_t createEntityS(EntityS entity) {
    uint64_t entityData = 0;
    // Layout (bits): [63..48]=x | [47..32]=y | [31..24]=type | [23..16]=health

    entityData |= (static_cast<uint64_t>(static_cast<uint16_t>(entity.x)) & 0xFFFFULL) << 48;
    entityData |= (static_cast<uint64_t>(static_cast<uint16_t>(entity.y)) & 0xFFFFULL) << 32;

    entityData |= (static_cast<uint64_t>(entity.entityType) & 0xFFULL) << 24;
    entityData |= (static_cast<uint64_t>(entity.health) & 0xFFULL) << 16;
    return entityData;
}

EntityS decodeEntityS(uint64_t entityData) {
    EntityS entity;

    entity.x = static_cast<uint16_t>((entityData >> 48) & 0xFFFF);
    entity.y = static_cast<uint16_t>((entityData >> 32) & 0xFFFF);

    entity.entityType = static_cast<EntityType>((entityData >> 24) & 0xFF);
    entity.health = static_cast<int8_t>((entityData >> 16) & 0xFF);
    return entity;
}

std::unique_ptr<Entity> EntitySToEntity(const EntityS& entityS, Texture2D& tex, Entity* playerPtr) {
    float x = static_cast<float>(entityS.x);
    float y = static_cast<float>(entityS.y);

    switch (entityS.entityType) {
    case MAGE_BOSS:
        return std::make_unique<MageBoss>(x, y, tex, playerPtr);
    case RABBIT:
        return std::make_unique<RabbitEnemy>(x, y, tex, playerPtr);
    default:
        return nullptr;
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
    // Convert Pixel pos to Tile pos
    entityS.x = static_cast<int16_t>(entity->mPosition.x / 8.0f);
    entityS.y = static_cast<int16_t>(entity->mPosition.y / 8.0f);
    entityS.entityType = entity->mType;
    entityS.health = static_cast<uint8_t>(entity->mHealth);

    addEntityS(entityS);
}

void EntitySaver::fromEditor(std::string json) {
    using jsonn = nlohmann::json;
    auto doc = jsonn::parse(json);

    if (!doc.contains("chunkData")) return;

    auto arr = doc["chunkData"]["entities"];
    int chunkX = doc["chunkData"]["x"].get<int>();
    int chunkY = doc["chunkData"]["y"].get<int>();

    if (arr.is_null() || !arr.is_array() || arr.size() == 0) return;

    std::cout << "[SYSTEM] Przetwarzanie chunka (" << chunkX << "," << chunkY << ")..." << std::endl;

    for (long unsigned int i = 0; i < arr.size(); ++i) {
        for (long unsigned int j = 0; j < arr[i].size(); ++j) {

            uint8_t editorID = arr[i][j];
            if (editorID == 0) continue;

            EntityType gameType;

            // Map Editor ID -> Game Enum
            switch (editorID) {
            case 1:
                gameType = RABBIT;
                std::cout << " -> Mapowanie: EditorID 1 -> RABBIT" << std::endl;
                break;
            case 2:
                gameType = MAGE_BOSS;
                std::cout << " -> Mapowanie: EditorID 2 -> MAGE_BOSS" << std::endl;
                break;
            default:
                std::cout << " -> WARNING: Nieznane ID (" << (int)editorID << "), ustawiam RABBIT" << std::endl;
                gameType = RABBIT;
                break;
            }

            int globalTileX = static_cast<int>(j) + (chunkX * 64);
            int globalTileY = static_cast<int>(i) + (chunkY * 64);

            EntityS entity;
            entity.x = static_cast<int16_t>(globalTileX);
            entity.y = static_cast<int16_t>(globalTileY);
            entity.entityType = gameType;
            entity.health = 10;

            this->addEntityS(entity);
        }
    }
}

EntitySaver::~EntitySaver() {}

EntityLoader::EntityLoader(std::fstream& f) : fileStream(&f) {}

std::vector<EntityS> EntityLoader::getAll() {
    std::vector<EntityS> entities;
    fileStream->seekg(0);

    uint64_t entityData;
    while (fileStream->read(reinterpret_cast<char*>(&entityData), sizeof(entityData))) {
        EntityS entity = decodeEntityS(entityData);
        entities.push_back(entity);
    }
    return entities;
}

EntityLoader::~EntityLoader() {}