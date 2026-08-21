#pragma once
struct InputInfo;

enum SCREEN_CODES : uint8_t {
	//dont put 0 because of the main drawing loop
	GAMEPLAY_SCREEN = 1,
	MAIN_SCREEN = 2,
	END_SCREEN = 3,
};

struct GameState;

void ChangeScreenTo(GameState* gameState, SCREEN_CODES screenCode);

void InitializeGameplayScreen(GameState* gameState);
void UpdateGameplayScreen(GameState* gameState, InputInfo inputInfo);

void InitializeMainMenu(GameState* gameState);
void UpdateMainMenu(GameState* gameState, InputInfo inputInfo);

void InitializeEndScreen(GameState* gameState);
void UpdateEndScreen(GameState* gameState);
