#pragma once
#include "raylib.h"
#include <algorithm>
#include <vector>

// Pomocnicza funkcja do p³ynnej zmiany wartoœci (+- Lerp)
inline float Approach(float current, float target, float increase) {
	if (current < target)
		return std::min(current + increase, target);

	else
		return std::max(current - increase, target);
}

class Player {
public:
	Vector2 position;
	Vector2 velocity;
	Vector2 size;

	// Do zmian
	float maxSpeed = 350.0f;
	float acceleration = 1900.0f;
	float friction = 1700.0f;
	float gravity = 1400.0f;
	float jumpForce = 700.0f;

	// Timery
	float jumpBufferTime = 0.1f; //100ms
	float coyoteTime = 0.15f; //150ms

	// Stan
	float jumpBufferCounter = 0.0f;
	float coyoteTimeCounter = 0.0f;
	bool isGrounded = false;

	//Konstruktor
	Player(float startX, float startY) {
		position = { startX, startY };
		velocity = { 0, 0 };
		size = { 50.0f, 50.0f }; //tymczasowe
	}

	// Update przyjmuje listê przeszkód
	void Update(float deltaTime, const std::vector<Rectangle>& obstacles) {

		// Inputy i bufforowanie skoku
		bool jumpPressed = IsKeyPressed(KEY_SPACE);
		bool jumpHeld = IsKeyDown(KEY_SPACE);

		// Zarz¹dzanie buforem skoku
		(jumpPressed) ? jumpBufferCounter = jumpBufferTime : jumpBufferCounter -= deltaTime;

		// Zarz¹dzanie Coyote Time
		(isGrounded) ? coyoteTimeCounter = coyoteTime : coyoteTimeCounter -= deltaTime;

		// Kierunek ruchu
		// -1 lewo
		//  0 brak
		// +1 prawo
		float moveInput = 0.0f;
		if (IsKeyDown(KEY_RIGHT)) moveInput = 1.0f;
		if (IsKeyDown(KEY_LEFT)) moveInput = -1.0f;

		// ruch poziomy
		float targetSpeed = moveInput * maxSpeed;
		float accelRate = (moveInput != 0) ? acceleration : friction;

		velocity.x = Approach(velocity.x, targetSpeed, accelRate * deltaTime);

		// skok
		if (jumpBufferCounter > 0 && coyoteTimeCounter > 0)
		{
			velocity.y = -jumpForce;
			coyoteTimeCounter = 0; //¿eby nie by³o double jump'u
			jumpBufferCounter = 0; //¿eby nie by³o double jump'u
			isGrounded = false;
		}

		if (!jumpHeld && velocity.y < 0) {
			velocity.y = Approach(velocity.y, 0, friction * deltaTime);
		}

		// grawitacja
		velocity.y += gravity * deltaTime;

		// RUCH I KOLIZJE

		// Resetujemy do sprawdzania kolizji
		isGrounded = false;

		// Oœ X
		position.x += velocity.x * deltaTime;
		Rectangle playerRect = GetRect();

		for (const auto& obs : obstacles) {
			if (CheckCollisionRecs(playerRect, obs)) {
				if (velocity.x > 0) position.x = obs.x - size.x;
				else if (velocity.x < 0) position.x = obs.x + obs.width;
				velocity.x = 0;
				playerRect.x = position.x;
			}
		}

		// Oœ Y
		position.y += velocity.y * deltaTime;
		playerRect.y = position.y;

		for (const auto& obs : obstacles) {
			if (CheckCollisionRecs(playerRect, obs)) {
				if (velocity.y > 0) {
					position.y = obs.y - size.y;
					velocity.y = 0;
					isGrounded = true;
				}
				else if(velocity.y < 0){
					position.y = obs.y + obs.height;
					velocity.y = 0;
				}
			}
		}
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
