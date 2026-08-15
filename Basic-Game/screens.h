#pragma once
struct MouseInputInfo;

enum SCREEN_CODES : uint8_t{
	//dont put 0 because of the main drawing loop
	GAMEPLAY_SCREEN = 1,
	MAIN_SCREEN = 2,
};

struct GameState;

void InitializeGameplayScreen(GameState* gameState);
void UpdateGameplayScreen(GameState* gameState, MouseInputInfo mouseInput);

void InitializeMainMenu(GameState* gameState);
void UpdateMainMenu(GameState* gameState, MouseInputInfo mouseInput);
