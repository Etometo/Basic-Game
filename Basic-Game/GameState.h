#pragma once
#include <stdint.h>

typedef struct MemoryArena {
	size_t used;
	size_t capacity;
	uint8_t* base;
};
typedef struct GameState {
	MemoryArena arena;
	unsigned int goalFps = 60;
};

void* PushSize(GameState* state, size_t sizeInBytes);

