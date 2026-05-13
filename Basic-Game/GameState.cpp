#include "GameState.h"
#include <iostream>
#include <cassert>

void* PushSize(GameState* state, size_t sizeInBytes) {
	assert((char*)state->arena.capacity - (char*)state->arena.used > sizeInBytes);

	state->arena.used += sizeInBytes;
	return state->arena.base + state->arena.used - sizeInBytes;
}

