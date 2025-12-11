#pragma once
#include "raylib.h"

class Player {
public:
	Vector2 position;
	Vector2 velocity;
	float speed;
	bool canJump;

	//Tymczasowe wymiary gracza
	Vector2 size;

	//Konstruktor
	Player(float startX, float startY) {
		position = { startX, startY };
		velocity = { 0, 0 };
		speed = 300.0f;
		size = { 50.0f, 50.0f }; //tymczasowe
		canJump = false;
	}

	void Update(float deltaTime, float gravity) {
		//Ruch lewo/prawo
		if (IsKeyDown(KEY_RIGHT)) position.x += speed * deltaTime;
		if (IsKeyDown(KEY_LEFT)) position.x -= speed * deltaTime;

		if (IsKeyPressed(KEY_SPACE) && canJump) {
			velocity.y = -600.0f; // si³a skoku
			canJump = false;
		}

		// Grawitacja
		position.y += velocity.y * deltaTime;
		velocity.y += gravity * deltaTime;

		//// Kolizja z ziemi¹
		//if (position.y >= 400.0f) {
		//	position.y = 400.0f;
		//	velocity.y = 0.0f;
		//	canJump = true;
		//}
	}

	//potrzebne do kolizji
	Rectangle GetRect() {
		return { position.x, position.y, size.x, size.y };
	}

	//funkcja rysuj¹ca
	void Draw() {
		//tymczasowy sprite
		DrawRectangleV(position, size, MAROON);
	}
};
