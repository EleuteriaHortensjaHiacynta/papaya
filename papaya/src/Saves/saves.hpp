#pragma once
#include <string>
#include <filesystem>
#include <fstream>

#include "map.hpp"
#include "entities.hpp"
#include "raylib.h"

class Saves {
    private:
    std::filesystem::path path;

    std::fstream mapFile;
    std::fstream entitiesFile;
    std::fstream entitiesSaveFile;
    
    MapLoader mapLoader;
    MapSaver mapSaver;

    EntitySaver entitySaver;
    EntityLoader entityLoader;

    EntitySaver entitySavesSaver;
    EntityLoader entitySavesLoader;

    bool isSaveAvailable();

    public:
    explicit Saves(std::string pathToFolder);

    MapLoader getMap();
    MapSaver saveMap();
    EntityLoader getEntities();
    EntitySaver saveEntities();
    EntitySaver saveEntityToMap();
    void loadFromEditorDir(std::string pathToFolder);
    
    // void addIcon();
    // Texture2D getIcon();
};