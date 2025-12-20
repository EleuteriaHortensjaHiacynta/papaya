#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Player.h"
#include "Entities/Wall.h"

class SceneTestRoom {
public:
	std::vector<Entity*> entities;
	Player* playerRef; // wskaŸnik na gracza (przyda siê do kamery)

	Camera2D camera = { 0 };

	// Konstruktor
	SceneTestRoom() {
		// inicjalizacja gracza (do pozycji startowej)
		playerRef = new Player(30, 100);
		entities.push_back(playerRef); // dodajemy gracza do gry

		// TWORZENIE POZIOMU
		entities.push_back(new Wall(0, 160, 320, 40)); // pod³oga
		entities.push_back(new Wall(100, 110, 80, 10)); // lewa platforma
		entities.push_back(new Wall(220, 70, 60, 10)); // prawa platforma

		// konfiguracja kamery 
		camera.target = { playerRef->position.x, playerRef->position.y };
		camera.offset = { 160.0f, 90.0f };
		camera.rotation = 0.0f;
		camera.zoom = 1.0f;
		
	}

	// Destruktor
	~SceneTestRoom() {
		for (Entity* e : entities) {
			delete e;
		}
		entities.clear();
	}

	void Update(float deltaTime) {

		// Update wszystkich entity
		for (Entity* e : entities) {
			if (e->active) {
				e->Update(deltaTime);
			}
		}


		// Detekcja kolizji dla wszystkich entity
		for (auto it1 = entities.begin(); it1 != entities.end(); ++it1) {
			for (auto it2 = entities.begin(); it2 != entities.end(); ++it2) {
				Entity* a = *it1;
				Entity* b = *it2;

				if (a == b) continue; // nie sprawdzamy samych ze sob¹
				if (!a->active || !b->active) continue; // niesprawdzamy nieaktywnych

				if (CheckCollisionRecs(a->GetRect(), b->GetRect())) {
					a->OnCollision(b); // na odwrót { b -> OC(a); } i tak siê wykona przez pêtle 
				}
			}
		}

		// reset gracza do pozycji wyjœciowej
		if (playerRef->position.y > 250) {
			playerRef->position = { 30,100 };
			playerRef->velocity = { 0,0 };
		}

		if (playerRef) {
			playerRef->UpdateAnimation(deltaTime);
			
			// kamera
			float targetX = playerRef->position.x;
			float targetY = playerRef->position.y;

			camera.target.x = floor(targetX);
			camera.target.y = floor(targetY);
		}
	}

	void Draw() {
		// rysowanie t³a
		ClearBackground(RAYWHITE);

		// tryb kamery 2D
		BeginMode2D(camera);

			for (Entity* e : entities) {
				if (e->active) e->Draw();
			}

		EndMode2D();

		// Debug info
		DrawText("Sterowanie: Strza³ki + Spacja", 5, 5, 5, DARKGRAY);
		DrawText(TextFormat("Pos: %.0f, %.0f", playerRef->position.x, playerRef->position.y), 10, 40, 20, LIGHTGRAY);
	}
};