#pragma once
#include <stdint.h>
#include "player.h"

typedef struct MemoryArena {
	size_t used;
	size_t usedTemporary;
	size_t usableCapacity;
	size_t temporaryCapacity;
	size_t capacity;
	uint8_t* base;
};

enum SCREEN_CODES : uint8_t{
	GAMEPLAY_SCREEN = 0,
};

typedef struct GameState {
	bool isInitialized = false;
	MemoryArena arena;
	unsigned int goalFps;
	Entity entities[2000];
	Entity* nextEmptyPlaceForEntity;
	Entity* lastEntityOnEntities;
	unsigned int addedEntities;
	unsigned int entitiesCapacity;
	float gravityConstant;
	float EPSILON;
	int SOLVER_ITERATIONS;
	uint32_t ClickThresholdFrames;
	uint32_t nextAvailableId;

	uint64_t frameCount = 0;

	uint32_t WINDOW_HEIGHT, WINDOW_WIDTH;
	uint32_t gridSquareEdgeLength;
	//grid is a four dimentional array, outest dimention is the row, second outest is the column and inside a cell there is an array
	//capable of holding 10 uint32_t values(ids)
	uint32_t* spatialGrid;
	uint32_t gridDimentions[3];

	SCREEN_CODES currentScreenCode;

	bool gameplayScreenInitialized;
	Entity* entityBeingCut;
	Entity* cutPiece1;
	Entity* cutPiece2;
	bool readyForNewEntityInitialization;
	bool entityInitialized;
	VertexData* rectData;
	VertexData* rectDataEnd;
};


enum MOUSE_INPUT_TYPE{
	NONE = 0,
	LEFT_CLICK = 1,
};
struct MouseInputInfo {
	MOUSE_INPUT_TYPE inputType;
	bool mouseReleasedThisFrame;
	uint32_t inputDurationFrames;
	Vector2 inputPositions[2];
};

void* PushSize(GameState* state, size_t sizeInBytes);

void RetractSize(GameState* state, size_t sizeInBytes);

Entity* PushEntity(GameState* state);

void DeleteEntity(GameState* state, Entity* entity);

//gives memory from the end of the memory arena
void* PushTemporarySize(GameState* state, uint32_t sizeInBytes);

void RetractTemporarySize(GameState* state, uint32_t sizeInBytes);
