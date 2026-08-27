#include <raylib.h>
#include <iostream>
#include "player.h"
#include "GameState.h"
#include "screens.h"
#include "screens.h"
#include <thread>
#include <chrono>


int main() {
	void* rawMemory = new uint8_t[150 * 1024 * 1024];
	memset(rawMemory, 0, 150 * 1024 * 1024);
	GameState* gameState = (GameState*)rawMemory;
	InitializeGameState(gameState);

	InitializeMainMenu(gameState);
	InitializeGameplayScreen(gameState);
	gameState->gameplayScreenInitialized = true;
	ChangeScreenTo(gameState, MAIN_SCREEN);

	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
	InitWindow(gameState->WINDOW_WIDTH, gameState->WINDOW_HEIGHT, "GAME");
	SetTargetFPS(gameState->goalFps);

	MouseInputInfo mouseInputInfo = { NONE, false, 0, {{0, 0}, {0, 0}}};
	InputInfo inputInfo = { mouseInputInfo, 0 };
	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(RAYWHITE);

			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				if(inputInfo.mouseInputInfo.inputType == NONE){
					inputInfo.mouseInputInfo.inputType = LEFT_CLICK;
					inputInfo.mouseInputInfo.inputPositions[0] = GetMousePosition();
				}
				inputInfo.mouseInputInfo.inputPositions[1] = GetMousePosition();
				inputInfo.mouseInputInfo.inputDurationFrames++;
			}
			else {
				if (!inputInfo.mouseInputInfo.mouseReleasedThisFrame && inputInfo.mouseInputInfo.inputType == LEFT_CLICK) {
					inputInfo.mouseInputInfo.mouseReleasedThisFrame = true;
					inputInfo.mouseInputInfo.inputPositions[1] = GetMousePosition();
				}
				else {
					inputInfo.mouseInputInfo.inputType = NONE;
					inputInfo.mouseInputInfo.inputDurationFrames = 0;
					inputInfo.mouseInputInfo.mouseReleasedThisFrame = false;
				}
			}
			//I made new key codes because i wanna use them like flags so I can store multiple key inputs in one int
			if (IsKeyDown(KEY_A)) {
				inputInfo.keyCodes |= A_KEY_CODE;
			}
			if (IsKeyDown(KEY_D)) {
				inputInfo.keyCodes |= D_KEY_CODE;
			}
			if (IsKeyDown(KEY_ENTER)) {
				inputInfo.keyCodes |= ENTER_KEY_CODE;
			}

			switch (gameState->currentScreenCode) {
				case MAIN_SCREEN:
					UpdateMainMenu(gameState, inputInfo);
					break;
				case GAMEPLAY_SCREEN:
					UpdateGameplayScreen(gameState, inputInfo);
					break;
				case END_SCREEN:
					UpdateEndScreen(gameState, inputInfo);
			}

			for (int i = 0; i < gameState->lastEntityOnEntities + 1 - gameState->entities; i++) {
				Entity* entity = gameState->entities + i;
				if (entity->id != 0 && !(entity->flags & INVISIBLE_FLAG)) {
					DrawEntity(entity);
				}
				if (entity->id != gameState->floor->id) {
					entity->flags &= ~IN_CONTACT_WITH_GROUND_FLAG;
				}
			}

			gameState->frameCount++;
			double totalTime = GetTime();
			gameState->averageFPS = gameState->frameCount / totalTime;

		EndDrawing();
		inputInfo.keyCodes = 0;
	}
	RetractSize(gameState, sizeof(Entity*) * 20);
	CloseWindow();
	return 0;
}
