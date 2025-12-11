#pragma once
#include "raylib.h"
#include <vector>
#include "Player.h"

struct Platform {
	Rectangle rect;
	Color color;
};

class SceneTestRoom {
public:
	Player* player;
	std::vector<Platform> platforms;

	// Konstruktor
	SceneTestRoom() {
		// inicjalizacja gracza (do pozycji startowej)
		player = new Player(100, 300);

		// TWORZENIE POZIOMU

		// pod³oga
		platforms.push_back({ {0,500,800,50},GRAY });

		// lewa platforma
		platforms.push_back({ {200,350,200,20},DARKGRAY });

		// prawa platforma
		platforms.push_back({ {500,200,200,20},BLACK });
	}

	// Destruktor
	~SceneTestRoom() {
		delete player;
	}

	void Update(float deltaTime) {
		float gravity = 1000.0f;
		
		// Aktualizacja logiki gracza
		player->Update(deltaTime, gravity);

		// SYSTEM KOLIZJI
		// sprawdzamy ka¿d¹ platformê
		player->canJump = false; //domyœlnie

		Rectangle playerRect = player->GetRect();

		for (const auto& platform : platforms) {
			if (CheckCollisionRecs(playerRect, platform.rect)) {

				// wykrywanie l¹dowania gracza na platformie (kolizja górna)
				if (player->velocity.y > 0 && player->position.y + player->size.y - (player->velocity.y * deltaTime) <= platform.rect.y) {

					// przyklejenie gracza do platformy
					player->position.y = platform.rect.y - player->size.y;
					player->velocity.y = 0;
					player->canJump = true;
				}

			}
		}

		// reset gracza do pozycji wyjœciowej
		if (player->position.y > 1000) {
			player->position = { 100,300 };
			player->velocity = { 0,0 };
		}
	}

	void Draw() {
		//rysowanie t³a
		ClearBackground(RAYWHITE);

		for (const auto& platforms : platforms) {
			DrawRectangleRec(platforms.rect, platforms.color);
		}

		player->Draw();

		// Debug info
		DrawText("Sterowanie: Strza³ki + Spacja", 10, 20, 10, DARKGRAY);
	}
};