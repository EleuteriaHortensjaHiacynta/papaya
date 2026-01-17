#pragma once

#include <string>

namespace GameConstants {
    constexpr int VIRTUAL_WIDTH = 320;
    constexpr int VIRTUAL_HEIGHT = 180;
    constexpr int TILE_SIZE_PX = 8;
    constexpr int ATLAS_COLUMNS = 64;

    inline const std::string TILESET_PATH = "assets/tiles/atlas_512x512.png";
    inline const std::string PLAYER_TEXTURE_PATH = "assets/player.png";
    inline const std::string BOSS_TEXTURE_PATH = "assets/mage_boss.png";
    inline const std::string RABBIT_TEXTURE_PATH = "assets/rabbit.png";
    inline const std::string BACKGROUND_PATH = "assets/background.png";
    inline const std::string SAVE_PATH = "assets/saves/main_save";
    inline const std::string EDITOR_PATH = "assets/editor";
}