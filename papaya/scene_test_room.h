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
		player = new Player(30, 100);

		// TWORZENIE POZIOMU
		platforms.push_back({ {0,160,320,40},GRAY }); // pod³oga
		platforms.push_back({ {100,110,80,10},DARKGRAY }); // lewa platforma
		platforms.push_back({ {220,70,60,10},BLACK }); // prawa platforma
	}

	// Destruktor
	~SceneTestRoom() {
		delete player;
	}

	void Update(float deltaTime) {
		
		// przygotwanie przeszkód
		std::vector<Rectangle> obstacles;
		for (const auto& p : platforms) {
			obstacles.push_back(p.rect);
		}

		player->Update(deltaTime, obstacles);

		// reset gracza do pozycji wyjœciowej
		if (player->position.y > 250) {
			player->position = { 30,100 };
			player->velocity = { 0,0 };
		}
	}

	void Draw() {
		//rysowanie t³a
		ClearBackground(RAYWHITE);

		for (const auto& p : platforms) {
			DrawRectangleRec(p.rect, p.color);
		}

		player->Draw();

		// Debug info
		DrawText("Sterowanie: Strza³ki + Spacja", 5 5, 5, DARKGRAY);
	}
};