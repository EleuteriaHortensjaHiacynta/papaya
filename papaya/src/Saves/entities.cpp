#include "entities.hpp"
#include <iostream>
#include <algorithm>
#include <cstring>

#include "../Entities/Enemy.h"
#include "../Entities/Entity.h"
#include "../Entities/Player.h"
#include "../external_headers/json.hpp"
#include "../Entities/mage_boss.h"
#include "../Entities/rabbit.h"

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

    // [POPRAWIONE] Rzutowanie na uint16_t (zgodnie ze struktur¹ EntityS)
    entityData |= (static_cast<uint64_t>(static_cast<uint16_t>(entity.x)) & 0xFFFFULL) << 48;
    entityData |= (static_cast<uint64_t>(static_cast<uint16_t>(entity.y)) & 0xFFFFULL) << 32;
    entityData |= (static_cast<uint64_t>(entity.entityType) & 0xFFULL) << 24;
    entityData |= (static_cast<uint64_t>(entity.health) & 0xFFULL) << 16;
    return entityData;
}

EntityS decodeEntityS(uint64_t entityData) {
    EntityS entity;
    // [POPRAWIONE] Odkodowanie jako uint16_t
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

        // WA¯NE: Dodaj tutaj inne typy wrogów, jeœli masz (np. SLIME, BAT)
        // case SLIME: return std::make_unique<Slime>(x, y, tex, playerPtr);

    default:
        // Zabezpieczenie przed zwróceniem nullptr dla nieznanego typu
        // Jeœli masz klasê bazow¹ Enemy, mo¿esz tu zwróciæ generycznego wroga
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
    entityS.x = static_cast<uint16_t>(entity->mPosition.x);
    entityS.y = static_cast<uint16_t>(entity->mPosition.y);
    entityS.entityType = entity->mType;
    entityS.health = static_cast<uint8_t>(entity->mHealth);

    addEntityS(entityS);
}

void EntitySaver::fromEditor(std::string json) {
    using jsonn = nlohmann::json;
    auto doc = jsonn::parse(json);

    if (!doc.contains("chunkData")) return; // Zabezpieczenie

    auto arr = doc["chunkData"]["entities"];
    int chunkX = doc["chunkData"]["x"].get<int>();
    int chunkY = doc["chunkData"]["y"].get<int>();

    if (arr.is_null() || !arr.is_array() || arr.size() == 0) {
        // Mo¿na pomin¹æ rzucanie b³êdu, jeœli chunk jest pusty
        return;
    }

    // i = Wiersz (Y), j = Kolumna (X)
    for (long unsigned int i = 0; i < arr.size(); ++i) {
        for (long unsigned int j = 0; j < arr[i].size(); ++j) {
            uint8_t item = arr[i][j];

            if (item == 0) continue;

            // [POPRAWIONE] Zamieniono i z j przy obliczaniu X i Y
            uint16_t x = j + static_cast<uint16_t>(chunkX * 64); // j to X
            uint16_t y = i + static_cast<uint16_t>(chunkY * 64); // i to Y

            EntityS entity = {
                x,
                y,
                static_cast<EntityType>(item),
                10 // Domyœlne HP, bo w JSON z edytora mo¿e go nie byæ
            };
            this->addEntityS(entity);
        }
    }
}

EntitySaver::~EntitySaver() {
    // sortEntities();
}

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