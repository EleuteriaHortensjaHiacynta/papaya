#include "saves.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

MapLoader getMap();
MapSaver saveMap();
EntityLoader getEntities();
EntitySaver saveEntities();
EntitySaver saveEntityToMap();

Saves::Saves(const std::string pathToFolder) 
    : path(pathToFolder), 
      mapFile(), 
      entitiesFile(),
      entitiesSaveFile(),
      
      mapLoader(mapFile),
      mapSaver(mapFile),
      entitySaver(entitiesFile),
      entityLoader(entitiesFile),
      entitySavesSaver(entitiesSaveFile),
      entitySavesLoader(entitiesSaveFile)
{
    if (!fs::exists(path)) {
        fs::create_directory(path);
    }

    auto ensureFileExists = [](fs::path p) {
        if (!fs::exists(p)) {
            std::ofstream(p).close(); // Create and immediately close
        }
    };

    ensureFileExists(path / "map.dat");
    ensureFileExists(path / "entities.dat");
    ensureFileExists(path / "entities.dat.save");

    auto flags = std::ios::in | std::ios::out | std::ios::binary | std::ios::ate;
    
    mapFile.open(path / "map.dat", flags);
    entitiesFile.open(path / "entities.dat", flags);
    entitiesSaveFile.open(path / "entities.dat.save", flags);
}

bool Saves::isSaveAvailable() {
    if (entitySavesLoader.getAll().empty()) {
        return false;
    }
    return true;
}

EntityLoader Saves::getEntities() {
    if (isSaveAvailable()) {
        return entitySavesLoader;
    }
    return entityLoader;
}

EntitySaver Saves::saveEntities() {
    return entitySavesSaver;
}

EntitySaver Saves::saveEntityToMap() {
    return entitySaver;
}

MapLoader Saves::getMap() {
    return mapLoader;
}

MapSaver Saves::saveMap() {
    return mapSaver;
}

void Saves::loadFromEditorDir(std::string pathToFolder) {
    fs::path editorPath(pathToFolder);

    for (auto& file : fs::directory_iterator(editorPath)) {
        if (file.path().extension() == ".json") {
            std::fstream fileStream(file.path(), std::ios::in | std::ios::binary);
            if (!fileStream.is_open()) {
                throw std::runtime_error("Failed to open file: " + file.path().string());
            }
            std::string jsonContent((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
            fileStream.close();

            mapSaver.fromEditor(jsonContent);
            entitySaver.fromEditor(jsonContent);
        }
    }
}