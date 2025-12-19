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


	Camera2D camera = { 0 };

	// Konstruktor
	SceneTestRoom() {
		// inicjalizacja gracza (do pozycji startowej)
		player = new Player(30, 100);

		// TWORZENIE POZIOMU
		platforms.push_back({ {0,160,320,40},GRAY }); // pod³oga
		platforms.push_back({ {100,110,80,10},DARKPURPLE }); // lewa platforma
		platforms.push_back({ {220,70,60,10},BLACK }); // prawa platforma

		camera.target = { player->position.x, player->position.y }; // na co patrzy kamera
		camera.offset = { 320.0f / 2.0f, 180.0f / 2.0f }; // punkt œrodka na ekranie
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;
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

		Vector2 targetPos = { player->position.x + player->size.x / 2, player->position.y + player->size.y / 2 };

		camera.target.x += (targetPos.x - camera.target.x) * 5.0f * deltaTime;
		camera.target.y += (targetPos.y - camera.target.y) * 5.0f * deltaTime;


	}

	void Draw() {
		//rysowanie t³a
		ClearBackground(DARKGRAY);

		Camera2D renderCam = camera;

		renderCam.target.x = floor(camera.target.x);
		renderCam.target.y = floor(camera.target.y);
		// ¿eby nie by³o sub-pixel rendering zaokr¹glamy float w lerpie do inta

		BeginMode2D(renderCam);

		for (const auto& p : platforms) {
			DrawRectangleRec(p.rect, p.color);
		}

		player->Draw();

		EndMode2D();

		// Debug info
		DrawText("Sterowanie: Strza³ki + Spacja", 5, 5, 5, DARKGRAY);
	}
};