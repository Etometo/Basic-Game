#pragma once
#include "delaunator.hpp"
#include <vector>
#include "player.h"
#include "Triangulation.h"

void Triangulate2DPoints(VertexData* begin, size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd) {

	if (numOfPoints <= 3) {
		for (int i = 0; i < numOfPoints; i++) {
			**indicesEnd = i;
			*indicesEnd++;
		}
		return;
	}

	delaunator::Delaunator d(begin, numOfPoints, gameState);

	size_t numOfIndices = d.trianglesEnd - d.triangles;
	*indices = d.triangles;
	*indicesEnd = d.trianglesEnd;

	/*for (int i = 0; i < numOfIndices - 2; i += 3) {
		std::cout << "(X: " << (*(begin + indices[i])).x << ", Y: " << (*(begin + indices[i])).y << std::endl;
		std::cout << "(X: " << (*(begin + indices[i+1])).x << ", Y: " << (*(begin + indices[i+1])).y << std::endl;
		std::cout << "(X: " << (*(begin + indices[i+2])).x << ", Y: " << (*(begin + indices[i+2])).y << std::endl;
		std::cout << std::endl;
	}*/
};
