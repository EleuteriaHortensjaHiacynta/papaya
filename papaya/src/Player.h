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
	float maxSpeed = 100.0f;
	float acceleration = 10000.0f;
	float friction = 9998.0f;
	float gravity = 500.0f;
	float jumpForce = 250.0f;

	// Timery
	float jumpBufferTime = 0.1f; //100ms
	float coyoteTime = 0.15f; //150ms

	// Stan
	float jumpBufferCounter = 0.0f;
	float coyoteTimeCounter = 0.0f;
	bool isGrounded = false;

	// GRAFIKA I ANIMACJA
	Texture2D texture; // sprite sheet
	Rectangle frameRec;

	// Zmienne animacji
	int currentFrame = 0;
	float frameTimer = 0.0f;
	float frameSpeed = 0.1f;
	int totalFrames = 6;

	// Kierunek patrzenia
	bool isFacingRight = true;

	//Konstruktor
	Player(float startX, float startY) {
		position = { startX, startY };
		velocity = { 0, 0 };

		texture = LoadTexture("Assets/player_walk.png");

		frameRec = { 0.0f, 0.0f, 16.0f, 16.0f };

		size = { 10.0f, 14.0f };
	}

	~Player() {
		UnloadTexture(texture);
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

		float currentAccel;
		float currentFriction;

		if (isGrounded) {
			// na ziemi
			currentAccel = 4000.0;
			currentFriction = 3500.0;
		}
		else {
			// w powietrzu
			currentAccel = 1500.0f;
			currentFriction = 800.0f;
		}

		float accelRate = (moveInput != 0) ? currentAccel : currentFriction;

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
				else if (velocity.y < 0) {
					position.y = obs.y + obs.height;
					velocity.y = 0;
				}
			}
		}

		// sprawdzamy w któr¹ stronê patrzy postaæ
		if (velocity.x > 0) isFacingRight = true;
		else if (velocity.x < 0) isFacingRight = false;

		// czy siê rusza?
		bool isMoving = (abs(velocity.x) > 10.0f);

		if (isMoving && isGrounded) {
			// animacja biegania
			frameTimer += deltaTime;
				
			if (frameTimer >= frameSpeed) {
				frameTimer = 0.0f;
				currentFrame++;
					
				// zapêtlanie animacji
				if (currentFrame >= totalFrames) currentFrame = 0;
			}

		}
		else {
			// gracz stoi lub skacze
			currentFrame = 0;
		}

		// przesuwanie "okienko wycinania" na odpowiedni¹ klatkê
		frameRec.x = (float)currentFrame * 16.0f;

	}

	//potrzebne do kolizji
	Rectangle GetRect() {
		return { position.x, position.y, size.x, size.y };
	}

	//funkcja rysuj¹ca
	void Draw() {

		float sourceWidth = isFacingRight ? frameRec.width : -frameRec.width;

		Rectangle source = { frameRec.x, frameRec.y, sourceWidth, frameRec.height };

		// hitbox 10px
		// texture 16px
		// centrujemy obrazek
		Vector2 drawPos;
		drawPos.x = position.x - 3;
		drawPos.y = position.y - 2;

		// Rysujemy na ekranie (dest)
		Rectangle dest = {
			(float)floor(drawPos.x),
			(float)floor(drawPos.y),
			16.0f, // rozmiar wyœwietlania
			16.0f,
		};

		DrawTexturePro(texture, source, dest, { 0,0 }, 0.0f, WHITE);
	}
};
