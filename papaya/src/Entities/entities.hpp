#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include "../Entities/Entity.h"

#define BLOCK_SIZE sizeof(uint64_t)

struct EntityS {
    int16_t x;
    int16_t y;
    EntityType entityType;
    uint8_t health;
    uint8_t textureID;
};

uint64_t createEntityS(EntityS entity);
EntityS decodeEntityS(uint64_t entityData);

std::unique_ptr<Entity> EntitySToEntity(const EntityS& entityS, Texture2D& mainTileset, Entity* playerPtr);

class EntitySaver {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit EntitySaver(std::fstream& f);
    void addEntityS(EntityS entity);
    int mTextureID = 0;
    void addEntity(const std::unique_ptr<Entity>& entity);

    void fromEditor(std::string json);
    ~EntitySaver();
};

class EntityLoader {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit EntityLoader(std::fstream& f);
    std::vector<EntityS> getAll();

    ~EntityLoader();
};