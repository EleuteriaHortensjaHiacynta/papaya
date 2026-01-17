#include "saves.hpp"
#include <iostream>

void Saves::openFile(std::fstream& file, const std::filesystem::path& filePath) {
    file.open(filePath, std::ios::in | std::ios::out | std::ios::binary);

    if (!file.is_open()) {
        std::ofstream create(filePath, std::ios::binary);
        create.close();
        file.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
    }
}

Saves::Saves(const std::string& pathToFolder) : path(pathToFolder) {
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }

    openFile(mapFile, path / "map.bin");
    openFile(entitiesFile, path / "entities.bin");
    openFile(entitiesSaveFile, path / "saves.bin");

    if (mapFile.is_open()) {
        mapLoader = std::make_unique<MapLoader>(mapFile);
        mapSaver = std::make_unique<MapSaver>(mapFile);
    }

    if (entitiesFile.is_open()) {
        entityLoader = std::make_unique<EntityLoader>(entitiesFile);
        entitySaver = std::make_unique<EntitySaver>(entitiesFile);
    }

    if (entitiesSaveFile.is_open()) {
        entitySavesLoader = std::make_unique<EntityLoader>(entitiesSaveFile);
        entitySavesSaver = std::make_unique<EntitySaver>(entitiesSaveFile);
    }
}

void Saves::refresh() {
    auto resetStream = [](std::fstream& stream) {
        if (stream.is_open()) {
            stream.flush();
            stream.clear();
            stream.seekg(0, std::ios::beg);
            stream.seekp(0, std::ios::end);
        }
        };

    resetStream(mapFile);
    resetStream(entitiesFile);
    resetStream(entitiesSaveFile);
}

MapLoader& Saves::getMap() {
    if (!mapLoader) throw std::runtime_error("MapLoader not initialized");
    return *mapLoader;
}

MapSaver& Saves::saveMap() {
    if (!mapSaver) throw std::runtime_error("MapSaver not initialized");
    return *mapSaver;
}

EntityLoader& Saves::getEntities() {
    if (!entityLoader) throw std::runtime_error("EntityLoader not initialized");
    return *entityLoader;
}

EntitySaver& Saves::saveEntities() {
    if (!entitySaver) throw std::runtime_error("EntitySaver not initialized");
    return *entitySaver;
}

EntitySaver& Saves::saveEntityToMap() {
    if (!entitySavesSaver) throw std::runtime_error("EntitySavesSaver not initialized");
    return *entitySavesSaver;
}

void Saves::loadFromEditorDir(const std::string& pathToFolder) {
    std::filesystem::path editorPath(pathToFolder);

    if (!std::filesystem::exists(editorPath)) {
        std::cerr << "[ERROR] Folder nie istnieje: " << pathToFolder << std::endl;
        return;
    }

    std::cout << "[SYSTEM] Import z '" << pathToFolder << "'..." << std::endl;

    for (const auto& entry : std::filesystem::directory_iterator(editorPath)) {
        if (entry.path().extension() == ".json") {

            // --- NOWY FRAGMENT KODU ---
            // Sprawdzamy, czy nazwa pliku zawiera "autosave". Jeśli tak, pomijamy go.
            std::string filename = entry.path().filename().string();
            if (filename.find("autosave") != std::string::npos) {
                std::cout << "[SYSTEM] Pominieto plik autozapisu: " << filename << std::endl;
                continue;
            }
            // --------------------------

            std::ifstream jsonFile(entry.path());
            if (jsonFile.is_open()) {
                std::string content(
                    (std::istreambuf_iterator<char>(jsonFile)),
                    std::istreambuf_iterator<char>()
                );
                jsonFile.close();

                if (mapSaver) {
                    mapSaver->fromEditor(content);
                }

                // Pamiętaj o tym, co dodaliśmy wcześniej!
                if (entitySaver) {
                    entitySaver->fromEditor(content);
                }
            }
        }
    }

    refresh();
    std::cout << "[SYSTEM] Import zakończony." << std::endl;
}