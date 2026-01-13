#include "Scenes/scene_test_room.h"

int main() {
    InitWindow(1280, 720, "Papaya - DEBUG");
    InitAudioDevice();
    SetTargetFPS(60);

    // Uruchom debug room
    debugRoom(1280, 720);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}