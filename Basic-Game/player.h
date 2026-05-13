#pragma once
#include <raylib.h>
#include "Triangulation.h"

typedef struct VertexData {
	Vector2 position;
};

typedef struct Player {
	Vector2 centerPosition;
	Color color;
	VertexData* vertexData;
	VertexData* vertexDataEnd;
	unsigned int* triangulationIndices;
	unsigned int* triangulationIndicesEnd;
};

Player* PushAndInitializePlayer(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd);
