#include "scene_game.hpp"
#include "raylib.h"
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include <string>

#include "Saves/saves.hpp"
#include "Entities/Entity.h" 

// ==========================================
// KONFIGURACJA
// ==========================================
const int VIRTUAL_WIDTH = 320;
const int VIRTUAL_HEIGHT = 180;
const int TILE_SIZE_PX = 8;
const std::string TILESET_PATH = "assets/tiles/atlas_512x512.png";
// ==========================================

void sceneGame(int windowWidth, int windowHeight, bool& shouldQuit, int& state) {

    // --- ODKOMENTUJ TÊ LINIJKÊ TYMCZASOWO ---
    // Upewnij siê, ¿e œcie¿ka wskazuje na folder, w którym masz pliki chunk-X-Y.json!
    // Jeœli pliki json le¿¹ w "assets/saves/main_save", wpisz tê œcie¿kê.
    // Jeœli le¿¹ w folderze edytora, wpisz œcie¿kê do edytora.

    Saves saves("assets/saves/main_save");
    saves.loadFromEditorDir("assets/saves/main_save");

    // OPCJONALNE: Jeœli edytor nie zapisuje automatycznie do binary, odkomentuj to raz:
    // saves.loadFromEditorDir("assets/saves/editor_chunks"); 


    // 1. £ADOWANIE TEKSTURY
    Texture2D tileset = LoadTexture(TILESET_PATH.c_str());
    SetTextureFilter(tileset, TEXTURE_FILTER_POINT);

    bool textureLoaded = (tileset.id != 0);
    int columns = (textureLoaded && tileset.width > 0) ? tileset.width / TILE_SIZE_PX : 1;
    if (columns == 0) columns = 1;

    // 2. £ADOWANIE MAPY (Tylko raz przed pêtl¹!)
    std::cout << "[INFO] Rozpoczynam ladowanie mapy..." << std::endl;
    std::vector<RenderTile> mapTiles = saves.getMap().getRenderData();

    // --- DIAGNOSTYKA ---
    std::cout << "[DEBUG] Wczytano klockow: " << mapTiles.size() << std::endl;
    if (!mapTiles.empty()) {
        std::cout << "[DEBUG] Przyklad klocka [0]: X=" << mapTiles[0].x
            << " Y=" << mapTiles[0].y
            << " TextureID=" << mapTiles[0].textureID << std::endl;
    }
    else {
        std::cout << "[WARNING] Lista klockow jest PUSTA! Sprawdz save'y." << std::endl;
    }
    // -------------------

    RenderTexture2D target = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    // 3. £ADOWANIE ENTITIES
    auto rawEntities = saves.getEntities().getAll();
    std::vector<std::unique_ptr<Entity>> activeEntities;
    Entity* playerPtr = nullptr;

    for (const auto& eData : rawEntities) {
        auto newEnt = EntitySToEntity(eData);
        if (newEnt) {
            // Zak³adamy, ¿e pierwszy znaleziony to gracz (lub dodaj logikê sprawdzania typu)
            if (!playerPtr) playerPtr = newEnt.get();
            activeEntities.push_back(std::move(newEnt));
        }
    }

    // Teleportacja gracza na start jeœli jest w 0,0 (opcjonalne)
    if (playerPtr) {
        std::cout << "[DEBUG] Gracz znaleziony na: " << playerPtr->mPosition.x << ", " << playerPtr->mPosition.y << std::endl;
        if (playerPtr->mPosition.x < 10 && playerPtr->mPosition.y < 10) {
            playerPtr->mPosition = Vector2{ (float)VIRTUAL_WIDTH / 2, (float)VIRTUAL_HEIGHT / 2 };
        }
    }

    // 4. INICJALIZACJA KAMERY
    Camera2D camera = { 0 };
    camera.zoom = 1.0f;
    camera.offset = { VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
    camera.target = { 0.0f, 0.0f };

    bool exitScene = false;

    // ==========================================
    // G£ÓWNA PÊTLA GRY
    // ==========================================
    while (!WindowShouldClose() && !shouldQuit && !exitScene) {
        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_ESCAPE)) {
            state = 0;
            exitScene = true;
        }

        // Tryb DEBUG (latanie kamer¹/graczem)
        if (playerPtr && IsKeyDown(KEY_TAB)) {
            float speed = 100.0f * dt;
            if (IsKeyDown(KEY_W)) playerPtr->mPosition.y -= speed;
            if (IsKeyDown(KEY_S)) playerPtr->mPosition.y += speed;
            if (IsKeyDown(KEY_A)) playerPtr->mPosition.x -= speed;
            if (IsKeyDown(KEY_D)) playerPtr->mPosition.x += speed;
        }

        // --- OBS£UGA B£ÊDU BRAKU TEKSTURY ---
        if (!textureLoaded) {
            BeginDrawing();
            ClearBackground(RED); // Czerwony ekran b³êdu
            DrawText("BLAD: Brak pliku texture atlas!", 10, 10, 20, WHITE);
            DrawText(TILESET_PATH.c_str(), 10, 40, 10, WHITE);
            EndDrawing();
            continue; // Pomijamy resztê klatki
        }

        // Update logiki
        for (auto& ent : activeEntities) ent->update(dt);

        if (playerPtr) {
            camera.target = { playerPtr->mPosition.x, playerPtr->mPosition.y };
        }

        // --- RYSOWANIE NA WIRTUALNYM EKRANIE ---
        BeginTextureMode(target);
        // WA¯NE: Ustawiamy t³o na CIEMNOSZARY, ¿eby widzieæ czy mapa siê ³aduje (bia³y zlewa siê z brakiem tekstur)
        ClearBackground(DARKGRAY);

        BeginMode2D(camera);

        // Rysowanie mapy
        for (const auto& tile : mapTiles) {
            if (tile.textureID == 0) continue;

            // TextureID w nowym systemie to uint16_t, odejmujemy 1 bo ID 0 to pustka
            int drawID = static_cast<int>(tile.textureID);

            if (drawID < 0) continue;

            int srcX = (drawID % columns) * TILE_SIZE_PX;
            int srcY = (drawID / columns) * TILE_SIZE_PX;

            // Rysuj tylko jeœli klatka mieœci siê w atlasie
            if (srcX + TILE_SIZE_PX <= tileset.width && srcY + TILE_SIZE_PX <= tileset.height) {
                Rectangle sourceRec = { (float)srcX, (float)srcY, (float)TILE_SIZE_PX, (float)TILE_SIZE_PX };
                Vector2 destPos = { (float)tile.x, (float)tile.y };
                DrawTextureRec(tileset, sourceRec, destPos, WHITE);
            }
        }

        for (const auto& ent : activeEntities) ent->draw();

        EndMode2D();
        EndTextureMode();

        // --- SKALOWANIE I RYSOWANIE NA EKRANIE ---
        BeginDrawing();
        ClearBackground(BLACK); // Pasy wokó³ ekranu bêd¹ czarne

        float scale = std::min((float)windowWidth / VIRTUAL_WIDTH, (float)windowHeight / VIRTUAL_HEIGHT);
        float renderWidth = VIRTUAL_WIDTH * scale;
        float renderHeight = VIRTUAL_HEIGHT * scale;
        float offsetX = (windowWidth - renderWidth) * 0.5f;
        float offsetY = (windowHeight - renderHeight) * 0.5f;

        DrawTexturePro(target.texture,
            { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height },
            { offsetX, offsetY, renderWidth, renderHeight },
            { 0.0f, 0.0f }, 0.0f, WHITE);

        // Opcjonalne info debugowe na ekranie
        // DrawText(TextFormat("Tiles: %i", (int)mapTiles.size()), 10, 10, 20, GREEN);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    UnloadTexture(tileset);
}