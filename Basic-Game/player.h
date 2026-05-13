#ifndef PLAYER_H
#define PLAYER_H

#include <cstddef>

#include "GameState.h"
#include "raylib.h"

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

// Initializes the player memory using the custom arena/push allocator
Player* PushAndInitializePlayer(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd);

// Renders the player based on the triangulated mesh
void DrawPlayer(Player* player);

// Triangulates a set of 2D points (exposed if you need to use it outside of player initialization)
void Triangulate2DPoints(VertexData* begin, std::size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd);

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3);

Vector2 AddVectors(Vector2 v1, Vector2 v2);

#endif