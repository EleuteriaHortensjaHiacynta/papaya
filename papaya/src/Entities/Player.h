#pragma once
#include "raylib.h"
#include <vector>
#include "Entities/Entity.h"

enum AnimState {
	IDLE,
	WALK,
	JUMP_UP,
	JUMP_DOWN,
	TURN
};

class Player : public Entity {
public:
	Vector2 position;
	Vector2 prevPosition;
	Vector2 velocity;
	Vector2 size;

	// Do zmian
	float maxSpeed = 100.0f;
	float acceleration = 1110.0f;
	float friction = 990.0f;
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
	AnimState currentState = IDLE;

	// Konfiguracja klatek
	int startFrame = 0;
	int lengthFrames = 0; // ile klatek trwa animacja
	bool loop = true; // czy animacja siê powtarza?

	// Liczniki
	int currentFrameRelative = 0;
	float frameTimer = 0.0f;
	float frameSpeed = 0.1f;

	// Kierunek patrzenia
	bool isFacingRight = true;

	//Konstruktor
	Player(float startX, float startY) : Entity({ startX, startY }, PLAYER) {
		position = { startX, startY };
		prevPosition = position;
		velocity = { 0, 0 };
		texture = LoadTexture("Assets/player.png");
		frameRec = { 0.0f, 0.0f, 16.0f, 16.0f };
		size = { 10.0f, 14.0f };
	}

	~Player() {
		UnloadTexture(texture);
	}

	// Update przyjmuje listê przeszkód
	void Update(float deltaTime) override {

		// Zapisujemy prevPosition
		prevPosition = position;

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

		// ruch poziomy z pominiêciem akceleracji
		if (moveInput != 0) {
			velocity.x = moveInput * maxSpeed;
		}
		else {
			velocity.x = 0;
		}

		// skok
		if (jumpBufferCounter > 0 && coyoteTimeCounter > 0)
		{
			velocity.y = -jumpForce;
			coyoteTimeCounter = 0; //¿eby nie by³o double jump'u
			jumpBufferCounter = 0; //¿eby nie by³o double jump'u
			isGrounded = false;
		}

		if (!jumpHeld && velocity.y < -50.0f) {
			velocity.y = -50.0f;
			//velocity.y = Approach(velocity.y, 0, friction * deltaTime);
		}

		// grawitacja
		velocity.y += gravity * deltaTime;

		// RUCH I KOLIZJE

		// Resetujemy do sprawdzania kolizji
		isGrounded = false;

		// Oœ X
		position.x += velocity.x * deltaTime;

		// Oœ Y
		position.y += velocity.y * deltaTime;
	}

	void UpdateAnimation(float deltaTime) {
		// Kierunek ruchu
		// -1 lewo
		//  0 brak
		// +1 prawo
		float moveInput = 0.0f;
		if (IsKeyDown(KEY_RIGHT)) moveInput = 1.0f;
		if (IsKeyDown(KEY_LEFT)) moveInput = -1.0f;

		// STATE MACHINE
		if (moveInput != 0.0f) {

			bool wantsGoRight = (moveInput > 0);

			if (wantsGoRight != isFacingRight) {
				isFacingRight = wantsGoRight;

				if (currentState != TURN && isGrounded) {
					currentState = TURN;
					currentFrameRelative = 0;
				}
			}
		}

		bool isBusyTurning = (currentState == TURN && currentFrameRelative < 4 && isGrounded); //animacja obrotu ma 5 klatek (0-4)

		if (!isBusyTurning) {
			if (!isGrounded) {
				if (velocity.y < 0) {
					currentState = JUMP_UP;
				}
				else {
					currentState = JUMP_DOWN;
				}
			}
			else {
				if (moveInput != 0.0f) {
					currentState = WALK;
				}
				else {
					currentState = IDLE;
				}
			}
		}

		// mapa

		switch (currentState) {
		case WALK:
			startFrame = 0; lengthFrames = 6; loop = true; frameSpeed = 0.1f;
			break;

		case JUMP_UP:
			startFrame = 6; lengthFrames = 3; loop = false; frameSpeed = 0.1f;
			break;

		case JUMP_DOWN:
			startFrame = 9; lengthFrames = 3; loop = false; frameSpeed = 0.1f;
			break;

		case IDLE:
			startFrame = 12; lengthFrames = 6; loop = true; frameSpeed = 0.15f;
			break;

		case TURN:
			startFrame = 18; lengthFrames = 3; loop = false; frameSpeed = 0.04f;
			break;
		}

		// Silnik animacji

		frameTimer += deltaTime;

		if (frameTimer >= frameSpeed) {
			frameTimer = 0.0f;
			currentFrameRelative++;

			if (currentFrameRelative >= lengthFrames) {
				// obs³ugiwanie koñca animacji
				if (loop) {
					currentFrameRelative = 0;
				}
				else {
					currentFrameRelative = lengthFrames - 1;

					if (currentState == TURN) {
						currentState = (moveInput != 0) ? WALK : IDLE;
						currentFrameRelative = 0;
					}
				}
			}
		}
		// finalne przesuniêcie okienka animacji
		frameRec.x = (float)(startFrame + currentFrameRelative) * 16.0f;
	}

	//potrzebne do kolizji
	Rectangle GetRect() override {
		return { position.x, position.y, size.x, size.y };
	}

	void OnCollision(Entity* other) override {
		if (other->type == WALL) {
			
			Rectangle otherRect = other->GetRect(); // prostok¹t œciany

			// dolna krawêdŸ gracza w poprzedniej klatce
			float prevBottom = prevPosition.y + size.y;

			// lewa i prawa krawêdŸ w poprzedniej klatce
			float prevRight = prevPosition.x + size.x;
			float prevLeft = prevPosition.x;

			if (prevBottom <= otherRect.y + 2.0f) { // 2.0f toleracji dla floatów 
				// jesteœmy na ziemi
				position.y = otherRect.y - size.y;
				velocity.y = 0;
				isGrounded = true;
			}

			// entity pod przeszkod¹ (sufit)
			else if(prevPosition.y >= otherRect.y + otherRect.height - 2.0f) { // 2.0f tolerancji dla floatów
				position.y = otherRect.y + otherRect.height;
				velocity.y = 0;
			}

			// œciana boczna
			else {
				// czy entity by³o z lewej strony
				if (prevRight <= otherRect.x + 5.0f) { // margines b³êdu
					position.x = otherRect.x - size.x;
				}
				// czy entity by³o z prawej strony
				else if (prevLeft >= otherRect.x + otherRect.width - 5.0f) { // margines b³êdu
					position.x = otherRect.x + otherRect.width;
				}

				velocity.x = 0;
			}
			
		}
	}

	//funkcja rysuj¹ca
	void Draw() override {

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
