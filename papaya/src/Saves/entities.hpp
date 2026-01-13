#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include "../Entities/Entity.h"

#define BLOCK_SIZE sizeof(uint64_t)

// Structure to represent an entity's saved data
struct EntityS {
    uint16_t x;
    uint16_t y;
    EntityType entityType;
    uint8_t health;
};

uint64_t createEntityS(EntityS entity);
EntityS decodeEntityS(uint64_t entityData);  

// Class to save blocks to a file
class EntitySaver {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit EntitySaver(std::fstream& f);
    void addEntityS(EntityS entity);
    void addEntity(std::unique_ptr<Entity> entity);
    // void sortEntities();
    void fromEditor(std::string json);
    ~EntitySaver();
};

// Class to load blocks from 
class EntityLoader {
private:
    std::fstream* fileStream = nullptr;
public:
    explicit EntityLoader(std::fstream& f);
    // std::vector<std::unique_ptr<Entity>> getAll();
    std::vector<EntityS> getAll();

    ~EntityLoader();
};