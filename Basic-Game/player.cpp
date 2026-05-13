#include "player.h"
#include "delaunator.hpp"
#include "Triangulation.h"
#include <iostream>

Player* PushAndInitializePlayer(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd) {
	if (vertexDataEnd - vertexData < 3) {
		throw std::runtime_error("At least 3 vertices for the player");
	}
	Player* returnPointer = (Player*)PushSize(gameState, sizeof(Player));

	returnPointer->vertexData = vertexData;
	unsigned int* indices;
	unsigned int* indicesEnd;
	Triangulate2DPoints(returnPointer->vertexData, vertexDataEnd - vertexData, gameState, &indices, &indicesEnd);
	returnPointer->triangulationIndices = indices;
	returnPointer->triangulationIndicesEnd = indicesEnd;

	return returnPointer;
}

void DrawPlayer(Player* player) {
	/*for (int i = 0; i < player->triangulationIndicesEnd - player->triangulationIndices; i += 3) {
		DrawTriangle(player->vertexData[i].position, player->vertexData[i + 1].position, player->vertexData[i + 2].position)
	}*/
}
