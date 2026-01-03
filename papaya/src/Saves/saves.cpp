#pragma once
#include <iostream>
#include <fstream>

class SaveManager {
    private:
        std::fstream saveFile;
    public:
        SaveManager(const std::string& filename) {
            saveFile.open(filename, std::ios::binary | std::ios::in | std::ios::out | std::ios::app);
            if (!saveFile.is_open()) {
                throw std::runtime_error("Failed to open save file.");
            }
        }
        ~SaveManager() {
            if (saveFile.is_open()) {
                saveFile.close();
            }
        }
};
