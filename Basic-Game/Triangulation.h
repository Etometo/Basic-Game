#pragma once
#include "GameState.h"

struct VertexData;

void Triangulate2DPoints(VertexData* begin, size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd);
