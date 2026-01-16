#pragma once
#include <string>
#include <filesystem>
#include <fstream>
#include <memory>
#include "map.hpp"
#include "entities.hpp"

class Saves {
private:
    std::filesystem::path path;

    std::fstream mapFile;
    std::fstream entitiesFile;
    std::fstream entitiesSaveFile;

    std::unique_ptr<MapLoader> mapLoader;
    std::unique_ptr<MapSaver> mapSaver;
    std::unique_ptr<EntityLoader> entityLoader;
    std::unique_ptr<EntitySaver> entitySaver;
    std::unique_ptr<EntityLoader> entitySavesLoader;
    std::unique_ptr<EntitySaver> entitySavesSaver;

    void openFile(std::fstream& file, const std::filesystem::path& filePath);

public:
    explicit Saves(const std::string& pathToFolder);

    void refresh();

    MapLoader& getMap();
    MapSaver& saveMap();
    EntityLoader& getEntities();
    EntitySaver& saveEntities();
    EntitySaver& saveEntityToMap();

    void loadFromEditorDir(const std::string& pathToFolder);
};