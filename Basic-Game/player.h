#ifndef PLAYER_H
#define PLAYER_H

#include <cstddef>

#include "raylib.h"

struct GameState;

typedef struct VertexData {
	Vector2 position;
};

typedef struct Entity {
	bool isPlayer = false;
	Vector2 centerPosition = {-100, -100};
	Color color = {255, 255, 255, 255};
	VertexData* vertexData;
	VertexData* vertexDataEnd;
	unsigned int* triangulationIndices = nullptr;
	unsigned int* triangulationIndicesEnd = nullptr;
};

// Initializes the player memory using the custom arena/push allocator
Entity* PushAndInitializePlayer(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd);

// Renders the player based on the triangulated mesh
void DrawPlayer(Entity* player);

// Triangulates a set of 2D points (exposed if you need to use it outside of player initialization)
void Triangulate2DPoints(VertexData* begin, std::size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd);

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3);

Vector2 AddVectors(Vector2& v1, Vector2& v2);

void MovePlayer(Entity* player, Vector2 movement);

int CalculateRelevantEntitiesFor(GameState* gameState, Entity* entity, Entity** relevanEntities);

void CalculateAndApplyCollisionWithEntity(Entity* e1, Entity* e2);

#endif