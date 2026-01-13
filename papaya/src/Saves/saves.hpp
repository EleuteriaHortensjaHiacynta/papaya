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

    EntityLoader entityLoader;
    EntitySaver entitySaver;

    EntityLoader entitySavesLoader;
    EntitySaver entitySavesSaver;

    bool isSaveAvailable();

    public:

    MapLoader getMap();
    MapSaver saveMap();
    EntityLoader getEntities();
    EntitySaver saveEntities();
    EntitySaver saveEntityToMap();
    void loadFromEditorDir(std::string pathToFolder);
    
    // void addIcon();
    // Texture2D getIcon();

    explicit Saves(std::string pathToFolder);
};