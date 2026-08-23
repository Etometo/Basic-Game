#include "player.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include <tuple>
#include <raylib.h>
#include "GameState.h"

constexpr float BAUMGARTE_BETA = 0.5f; 
constexpr float PENETRATION_SLOP = 0.2;           
constexpr float EPSILON = 1e-5f;           

float GetDeltaTime() {
    float frameTime = GetFrameTime();
    if (frameTime > 0.1) {
        return 0.1;
    }
    return frameTime;
}

Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags) {
	if (vertexDataEnd - vertexData < 3) {
		throw std::runtime_error("At least 3 vertices for the player");
	}
	Entity* returnPointer = (Entity*)PushEntity(gameState);

	returnPointer->vertexData = vertexData;
    returnPointer->vertexDataEnd = vertexDataEnd;
	unsigned int* indices;
	unsigned int* indicesEnd;
	Triangulate2DPoints(returnPointer->vertexData, vertexDataEnd - vertexData, gameState, &indices, &indicesEnd);

    int vertexCount = vertexDataEnd - vertexData;
    float inertia = 0;
    Vector2 centerOfTheShape = {0, 0};
    for (int i = 0; i < vertexCount; i++) {
        centerOfTheShape.x += vertexData[i].position.x;
        centerOfTheShape.y += vertexData[i].position.y;
    }
    centerOfTheShape.x /= vertexCount;
    centerOfTheShape.y /= vertexCount;

	returnPointer->centerPosition = centerOfTheShape;
	returnPointer->triangulationIndices = indices;
	returnPointer->triangulationIndicesEnd = indicesEnd;
    returnPointer->color = { 255, 0, 0, 255 };
    returnPointer->mass = mass;
    returnPointer->flags |= flags;

    float massPerVertex = returnPointer->mass / vertexCount;
    for (int i = 0; i < vertexCount; i++) {
        returnPointer->inertia += massPerVertex * (pow(vertexData[i].position.x - centerOfTheShape.x, 2) + pow(vertexData[i].position.y - centerOfTheShape.y, 2));
    }
    inertia /= 10;
    returnPointer->angularDamping = 5;

	return returnPointer;
}

Entity* InitializeAndPushEntity(GameState* gameState, VertexData* vertexData, VertexData* vertexDataEnd, float mass, uint32_t flags, Vector2 centerPos, SCREEN_CODES screenCode) {
    Entity* returnPointer = InitializeAndPushEntity(gameState, vertexData, vertexDataEnd, mass, flags);
    returnPointer->centerPosition.x += centerPos.x;
    returnPointer->centerPosition.y += centerPos.y;
    returnPointer->gridPositionsOfVertices = (uint32_t*)PushSize(gameState, (returnPointer->vertexDataEnd - returnPointer->vertexData) * 2 * sizeof(uint32_t));
    returnPointer->gridPositionOfVerticesSize = returnPointer->vertexDataEnd - returnPointer->vertexData;
    CalibrateEntityWithGrid(gameState, returnPointer);

    returnPointer->screenCode = screenCode;
    return returnPointer;
} 

int CutEntityIntoTwoPiecesByALine(GameState* gameState, Entity* entity, Vector2 cutStart, Vector2 cutEnd, uint32_t entityFlags, Entity* &newE1, Entity* &newE2) {
	uint16_t vertexCount = entity->vertexDataEnd - entity->vertexData;
	bool cutIsVertical = false, edgeIsVertical = false;
	float cutDx = cutEnd.x - cutStart.x;
	float cuttingLineSlope;
	if (abs(cutDx) < FLT_EPSILON) {
		cutIsVertical = true;
	}
	if (!cutIsVertical) {
		cuttingLineSlope = (cutEnd.y - cutStart.y) / (cutDx);
	}
	uint32_t totalAllocatedTemporarySize = vertexCount * sizeof(Vector2) + vertexCount * sizeof(uint32_t);
	Vector2* intersectionPoints = (Vector2*)PushTemporarySize(gameState, (vertexCount) * sizeof(Vector2));
	Vector2* intersectionPointsEnd = intersectionPoints;
	uint32_t* indicesOfPointOnLineIntersections = (uint32_t*)PushTemporarySize(gameState, vertexCount * sizeof(uint32_t));
	uint32_t* indicesOfPointOnLineIntersectionsEnd = indicesOfPointOnLineIntersections;

	VertexData* vertexData1 = (VertexData*)PushSize(gameState, (vertexCount + 2) * sizeof(VertexData));
	VertexData* vertexData1End = vertexData1;
	VertexData* vertexData2 = (VertexData*)PushSize(gameState, (vertexCount + 2) * sizeof(VertexData));
	VertexData* vertexData2End = vertexData2;
    uint32_t totalAllocatedSizeForVertexData = 2 * (vertexCount + 2) * sizeof(VertexData);
	Vector2 vertexData1center = { 0, 0 };
	Vector2 vertexData2center = { 0, 0 };
    uint8_t intersectionCount = 0;
	for (int i = 0; i < vertexCount; i++) {
		Vector2 vertexPos = entity->vertexData[i].position;
		vertexPos.x += entity->centerPosition.x;
		vertexPos.y += entity->centerPosition.y;

		float sideOfThePoint = (cutEnd.x - cutStart.x) * (vertexPos.y - cutStart.y) - (cutEnd.y - cutStart.y) * (vertexPos.x - cutStart.x);
		if (sideOfThePoint < -EPSILON) {
			(*vertexData1End).position = entity->vertexData[i].position;
			vertexData1center.x += (*vertexData1End).position.x;
			vertexData1center.y += (*vertexData1End).position.y;
			vertexData1End++;
		}
		else if (sideOfThePoint > -EPSILON) {
			(*vertexData2End).position = entity->vertexData[i].position;
			vertexData2center.x += (*vertexData2End).position.x;
			vertexData2center.y += (*vertexData2End).position.y;
			vertexData2End++;
		}

        Vector2 nextVertexPos;
        if ((i + 1) >= vertexCount) {
			 nextVertexPos = entity->vertexData[0].position;
        }
        else {
			 nextVertexPos = entity->vertexData[i + 1].position;
        }
        nextVertexPos.x += entity->centerPosition.x;
        nextVertexPos.y += entity->centerPosition.y;

		float edgeDx = nextVertexPos.x - vertexPos.x;
		edgeIsVertical = abs(edgeDx) < FLT_EPSILON ? true : false;
		float edgeSlope;

		float xValueOfIntersection;
		float yValueOfIntersection;
		if (cutIsVertical && edgeIsVertical) {
			continue;
		}
		else if (cutIsVertical) {
			edgeSlope = (nextVertexPos.y - vertexPos.y) / edgeDx;
			xValueOfIntersection = cutStart.x;
			yValueOfIntersection = edgeSlope * (xValueOfIntersection - vertexPos.x) + vertexPos.y;
		}
		else if (edgeIsVertical) {
			xValueOfIntersection = vertexPos.x;
			yValueOfIntersection = cuttingLineSlope * (xValueOfIntersection - cutStart.x) + cutStart.y;
		}
		else {
			edgeSlope = (nextVertexPos.y - vertexPos.y) / edgeDx;
			xValueOfIntersection = (cuttingLineSlope * cutStart.x - edgeSlope * vertexPos.x + vertexPos.y - cutStart.y) / (cuttingLineSlope - edgeSlope);
			yValueOfIntersection = cuttingLineSlope * (xValueOfIntersection - cutStart.x) + cutStart.y;
		}

		Vector2 positionOfIntersectionPointFromVertex1 = { xValueOfIntersection - vertexPos.x, yValueOfIntersection - vertexPos.y };
		Vector2 positionOfIntersectionPointFromVertex2 = { xValueOfIntersection - nextVertexPos.x, yValueOfIntersection - nextVertexPos.y };

        Vector2 positionOfCuttingLineStartFromIntersectionPoint = { cutStart.x - xValueOfIntersection, cutStart.y - yValueOfIntersection };
        Vector2 positionOfCuttingLineEndFromIntersectionPoint = { cutEnd.x - xValueOfIntersection, cutEnd.y - yValueOfIntersection };
		if (DotProduct(positionOfIntersectionPointFromVertex1, positionOfIntersectionPointFromVertex2) <= 0 && 
            DotProduct(positionOfCuttingLineStartFromIntersectionPoint, positionOfCuttingLineEndFromIntersectionPoint) <= 0) {
            Vector2 intersectionPoint = { xValueOfIntersection, yValueOfIntersection };

			(*vertexData1End).position = intersectionPoint;
			(*vertexData1End).position.x -= entity->centerPosition.x;
			(*vertexData1End).position.y -= entity->centerPosition.y;
			vertexData1center.x += (*vertexData1End).position.x;
			vertexData1center.y += (*vertexData1End).position.y;
			vertexData1End++;

			(*vertexData2End).position = intersectionPoint;
			(*vertexData2End).position.x -= entity->centerPosition.x;
			(*vertexData2End).position.y -= entity->centerPosition.y;
			vertexData2center.x += (*vertexData2End).position.x;
			vertexData2center.y += (*vertexData2End).position.y;
			vertexData2End++;
            intersectionCount++;
		}
	}

    if (intersectionCount == 2) {
        vertexData1center.x /= vertexData1End - vertexData1;
        vertexData1center.y /= vertexData1End - vertexData1;

        vertexData2center.x /= vertexData2End - vertexData2;
        vertexData2center.y /= vertexData2End - vertexData2;

        for (int i = 0; i < vertexData1End - vertexData1; i++) {
            vertexData1[i].position.x -= vertexData1center.x;
            vertexData1[i].position.y -= vertexData1center.y;
        }
        for (int i = 0; i < vertexData2End - vertexData2; i++) {
            vertexData2[i].position.x -= vertexData2center.x;
            vertexData2[i].position.y -= vertexData2center.y;
        }

        vertexData1center.x += entity->centerPosition.x;
        vertexData1center.y += entity->centerPosition.y;

        vertexData2center.x += entity->centerPosition.x;
        vertexData2center.y += entity->centerPosition.y;

        newE1 = (Entity*)InitializeAndPushEntity(gameState, vertexData1, vertexData1End, 10, entityFlags, vertexData1center, GAMEPLAY_SCREEN);
        newE2 = (Entity*)InitializeAndPushEntity(gameState, vertexData2, vertexData2End, 10, entityFlags, vertexData2center, GAMEPLAY_SCREEN);

        RetractTemporarySize(gameState, totalAllocatedTemporarySize);
        return ENTITY_WAS_CUT;
    }
    else {
        RetractTemporarySize(gameState, totalAllocatedTemporarySize);
        RetractSize(gameState, totalAllocatedSizeForVertexData);
		return ENTITY_WASNT_CUT;
    }
}

void CalibrateEntityWithGrid(GameState* gameState, Entity* e) 
{
    for (int i = 0; i < e->gridPositionOfVerticesSize; i++) {
        uint32_t rowIdx = *(e->gridPositionsOfVertices + (i * 2)), columnIdx = *(e->gridPositionsOfVertices + (i * 2) + 1);
        if (rowIdx > 40000 || columnIdx > 40000) { continue; }
		uint32_t* cellArray = gameState->spatialGrid + ((rowIdx * gameState->gridDimentions[0] + columnIdx) * gameState->gridDimentions[2]);
		for (int j = 0; j < gameState->gridDimentions[2]; j++) {
			if (cellArray[j] == e->id) {
                cellArray[j] = 0;
                break;
			}
		}
    }

	for (int i = 0; i < e->vertexDataEnd - e->vertexData; i++) {
		VertexData vertex = e->vertexData[i];
		vertex.position.x += e->centerPosition.x;
		vertex.position.y += e->centerPosition.y;
		if ((vertex.position.x < 0 || vertex.position.x >= gameState->WINDOW_WIDTH) || (vertex.position.y < 0 || vertex.position.y >= gameState->WINDOW_HEIGHT)) {
			//even though unsigned int can't be -1 this still works because it gets a crazy big value
            *(e->gridPositionsOfVertices + (i * 2)) = -1;
            *(e->gridPositionsOfVertices + (i * 2) + 1) = -1;
			continue;
		}
		int gridRowIdx = (int)vertex.position.y / gameState->gridSquareEdgeLength;
		int gridColumnIdx = (int)vertex.position.x / gameState->gridSquareEdgeLength;
		uint32_t* cellArray = gameState->spatialGrid + ((gridRowIdx * gameState->gridDimentions[0] + gridColumnIdx) * gameState->gridDimentions[2]);
        int numOfEntitiesIdInCell = 0;
		for (int j = 0; j < gameState->gridDimentions[2]; j++) {
			if (cellArray[j] == 0 && numOfEntitiesIdInCell == 0) {
				cellArray[j] = e->id;
				*(e->gridPositionsOfVertices + (i * 2)) = gridRowIdx;
				*(e->gridPositionsOfVertices + (i * 2) + 1) = gridColumnIdx;
                break;
			}
		}
	}
}
void PrintVector(Vector2 vec) {
    std::cout << "(X: " << vec.x << " ,Y: " << vec.y << " )" ;
}

void PrintVector(Vector3 vec) {
    std::cout << "(X: " << vec.x << " ,Y: " << vec.y << " ,Z: " << vec.z << " )" ;
}
void DrawEntity(Entity* player) {
	for (int i = 0; i < player->triangulationIndicesEnd - player->triangulationIndices; i += 3) {
        Vector2 v1 = AddVectors((*(player->vertexData + player->triangulationIndices[i])).position, player->centerPosition);
        Vector2 v2 = AddVectors((*(player->vertexData + player->triangulationIndices[i+1])).position, player->centerPosition);
        Vector2 v3 = AddVectors((*(player->vertexData + player->triangulationIndices[i+2])).position, player->centerPosition);
        if (IsCounterClockwise(v1, v2, v3)) {
            DrawTriangle(v1, v2, v3, player->color);
        }
        else{
            DrawTriangle(v1, v3, v2, player->color);
        }
	}
    if (player->flags & HAS_TEXT) {
        int fontSize = 30;
        float textWidth = MeasureText(player->text, fontSize);
		DrawText(player->text, player->centerPosition.x - textWidth/2, player->centerPosition.y - fontSize/2, fontSize, BLACK);
    }
    else {
		char idStr[10];
		snprintf(idStr, sizeof(idStr), "%d", player->id);
		DrawText(idStr, player->centerPosition.x, player->centerPosition.y, 20, BLACK);
    }
}

void DrawEntityOutline(Entity* entity) {
    int vertexCount = entity->vertexDataEnd - entity->vertexData;
    for (int i = 0; i < vertexCount - 1; i++) {
        Vector2 vertex1Pos = entity->vertexData[i].position;
        Vector2 vertex2Pos = entity->vertexData[i + 1].position;
        vertex1Pos.x += entity->centerPosition.x;
        vertex1Pos.y += entity->centerPosition.y;
        vertex2Pos.x += entity->centerPosition.x;
        vertex2Pos.y += entity->centerPosition.y;
        DrawLineEx(vertex1Pos, vertex2Pos, 10, YELLOW);
    }
	Vector2 vertex1Pos = entity->vertexData[vertexCount - 1].position;
	Vector2 vertex2Pos = entity->vertexData[0].position;
	vertex1Pos.x += entity->centerPosition.x;
	vertex1Pos.y += entity->centerPosition.y;
	vertex2Pos.x += entity->centerPosition.x;
	vertex2Pos.y += entity->centerPosition.y;
    DrawLineEx(vertex1Pos, vertex2Pos, 10, YELLOW);
}

void DrawEntityForceLine(Entity* entity) {
    Color colorOfTheLine = {0, 00, 255, 255};
    int drawingMultiplier = 1;
    DrawLine(entity->centerPosition.x, entity->centerPosition.y, entity->centerPosition.x + entity->forcesMultipliedByAppliedTime.x * drawingMultiplier, entity->centerPosition.y + entity->forcesMultipliedByAppliedTime.y * drawingMultiplier, colorOfTheLine);
}

bool IsCounterClockwise(Vector2 v1, Vector2 v2, Vector2 v3) {
    float crossProduct = (v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x);

    return crossProduct < 0.0f;
}

float triangleArea(Vector2 a, Vector2 b, Vector2 c) {
    return 0.5f * std::abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x));
}

Vector2 AddVectors(Vector2& v1, Vector2& v2) {
    Vector2 newVector = { v1.x + v2.x, v1.y + v2.y };
    return newVector;
}

double DotProduct(Vector3& vec1, Vector3& vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z;
}

double DotProduct(Vector3& vec1, Vector2& vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y;
}

double DotProduct(Vector2& vec1, Vector2& vec2) {
    return vec1.x * vec2.x + vec1.y * vec2.y;
}

float square(float f1) {
    return f1 * f1;
}

float distance(Vector2 v1, Vector2 v2) {
    return sqrtf(square(v1.x - v2.x) + square(v1.y - v2.y));
}

float magnitude(Vector2 v) {
    return sqrtf(square(v.x) + square(v.y));
}
void RotateEntity(Entity* entity, float deltaTime) {
    entity->rotationalVelocity *= 1 / (1 + entity->angularDamping * deltaTime);
    float rotationalChange = entity->rotationalVelocity * deltaTime;
    float sin = sinf(rotationalChange);
    float cos = cosf(rotationalChange);
    for (int i = 0; i < entity->vertexDataEnd - entity->vertexData; i++) {
        Vector2 vertexPos = entity->vertexData[i].position;
        entity->vertexData[i].position.x = vertexPos.x * cos - vertexPos.y * sin;
        entity->vertexData[i].position.y = vertexPos.x * sin + vertexPos.y * cos;
    }
    entity->angle += rotationalChange;
}

void MoveAndRotateEntity(Entity* player, float deltaTime) {
	player->centerPosition.x += player->physicsVelocity.x * deltaTime;
	player->centerPosition.y += player->physicsVelocity.y * deltaTime;
	player->lastSpeed = { player->physicsVelocity.x, player->physicsVelocity.y };
	player->penetrationVelocity = { 0, 0 };

	RotateEntity(player, deltaTime);
}

void ApplyForceToEntity(Entity* player, Vector2 mov) {
    player->netForce.x += mov.x;
    player->netForce.y += mov.y;
}

void ApplyForceToEntitiesVelocityImmediately(Entity* entity, Vector2 force, float deltaTime, Vector2 forceApplicationPoint) {
    if (entity->flags & NON_MOVING_FLAG) {
        return;
    }

    Vector2 forceApplicationPointFromTheCenter;
    float torque;
    forceApplicationPointFromTheCenter.x = forceApplicationPoint.x - entity->centerPosition.x;
    forceApplicationPointFromTheCenter.y = forceApplicationPoint.y - entity->centerPosition.y;
    if (abs(forceApplicationPointFromTheCenter.x) < EPSILON && abs(forceApplicationPointFromTheCenter.y) < EPSILON) {
        torque = 0;
    }
    else {
		Vector2 torqueAxis = { -forceApplicationPointFromTheCenter.y, forceApplicationPointFromTheCenter.x };
		torque = DotProduct(force, torqueAxis);
    }

    entity->netForce.x += force.x;
    entity->netForce.y += force.y;
    entity->acceleration.x = force.x / entity->mass;
    entity->acceleration.y = force.y / entity->mass;
    entity->physicsVelocity.x += entity->acceleration.x * deltaTime;
    entity->physicsVelocity.y += entity->acceleration.y * deltaTime;

    entity->torque += torque;
    entity->rotationalAcceleration = torque / entity->inertia;
    entity->rotationalVelocity += entity->rotationalAcceleration * deltaTime;

    entity->forcesMultipliedByAppliedTime.x += force.x * deltaTime;
    entity->forcesMultipliedByAppliedTime.y += force.y * deltaTime;
}


bool CheckIfAPointIsInsideAnEntity(Vector2 positionOfPoint, Entity* entity) {
    float minX = FLT_MAX, minY = FLT_MAX;
    float maxX = -FLT_MAX, maxY = -FLT_MAX;
    int vertexCount = entity->vertexDataEnd - entity->vertexData;
    for (int i = 0; i < vertexCount; i++) {
        Vector2 vertexPos = entity->vertexData[i].position;
        vertexPos.x += entity->centerPosition.x;
        vertexPos.y += entity->centerPosition.y;
		minX = vertexPos.x < minX ? vertexPos.x : minX;
		minY = vertexPos.y < minY ? vertexPos.y : minY;
		maxX = vertexPos.x > maxX ? vertexPos.x : maxX;
		maxY = vertexPos.y > maxY ? vertexPos.y : maxY;
	}

	if (positionOfPoint.x < minX || positionOfPoint.x > maxX || positionOfPoint.y < minY || positionOfPoint.y > maxY){
        return false;
	} 

    bool isInside = false;
    Vector2 vertex1Pos, vertex2Pos;

    for (int j = 0; j < vertexCount; j++) {
        vertex1Pos = { entity->vertexData[j].position.x + entity->centerPosition.x, entity->vertexData[j].position.y + entity->centerPosition.y };

        int nextIndex = (j + 1) < vertexCount ? (j + 1) : 0;
        vertex2Pos = { entity->vertexData[nextIndex].position.x + entity->centerPosition.x, entity->vertexData[nextIndex].position.y + entity->centerPosition.y };

        if ((vertex1Pos.y > positionOfPoint.y) != (vertex2Pos.y > positionOfPoint.y)) {
            float xValueOfRaycastsIntersection = (vertex2Pos.x - vertex1Pos.x) * (positionOfPoint.y - vertex1Pos.y) / (vertex2Pos.y - vertex1Pos.y) + vertex1Pos.x;

            if (positionOfPoint.x < xValueOfRaycastsIntersection) {
                isInside = !isInside;
            }
        }
    }	
    return isInside;
}

int MakeAnArrayFullOfUniqueItems(GameState* gameState, char* arrayStart, char* arrayEnd, uint32_t numOfElements, uint32_t typeSize) {
    uint32_t totalAllocatedSize = (numOfElements) * typeSize;
    char* uniqueItems = (char*)PushSize(gameState, totalAllocatedSize);
    uint32_t numOfUniqueItems = 0;

    char* item = (char*)PushSize(gameState, typeSize);
    for (int i = 0; i < numOfElements; i++) {
        item = arrayStart + (i * typeSize);
        uint32_t count = 0;
        for (int j = 0; j < numOfUniqueItems; j++) {
            char* itemInUniqueItems = uniqueItems + (j * typeSize);
            bool bytesAreIdentical = true;
            for (int b = 0; b < typeSize; b++) {
                if (*(itemInUniqueItems + b) != *(item + b)) {
                    bytesAreIdentical = false;
                    break;
                }
            }
            if (bytesAreIdentical) {
                count++;
                break;
            }
        }
        if (count == 0) {
            for (int b = 0; b < typeSize; b++) {
                *(uniqueItems + (numOfUniqueItems * typeSize) + b) = *(item + b);
            }
            numOfUniqueItems++;
        }
    }
    RetractSize(gameState, typeSize);

    arrayEnd = arrayStart;
    for (int b = 0; b < numOfUniqueItems * typeSize; b++) {
        *(arrayEnd) = uniqueItems[b];
        arrayEnd++;
    }

    RetractSize(gameState, totalAllocatedSize);

    return numOfUniqueItems;
}

int CalculateRelevantEntitiesForPosition(GameState* gameState, Vector2 position, Entity** relevanEntities) {
    if ((position.x < 0 || position.x >= gameState->WINDOW_WIDTH) || (position.y < 0 || position.y >= gameState->WINDOW_HEIGHT)) {
		return -1;
	}
    int gridRowIdx = (int)position.y / gameState->gridSquareEdgeLength;
    int gridColumnIdx = (int)position.x / gameState->gridSquareEdgeLength;

    Entity* entities = gameState->entities;
    Entity** relevantEntitiesEnd = relevanEntities;

    for (int i = -2; i < 3; i++) {
        for (int j = -2; j < 3; j++) {
            if (gridRowIdx + i < 0 || gridRowIdx + i >= gameState->gridDimentions[0] || gridColumnIdx + j < 0 || gridColumnIdx + j >= gameState->gridDimentions[1]) {
                continue;
            }
            uint32_t* cellArray = gameState->spatialGrid + (((gridRowIdx + i) * gameState->gridDimentions[0] + (gridColumnIdx + j)) * gameState->gridDimentions[2]);
            for (int k = 0; k < gameState->gridDimentions[2]; k++) {
                if (cellArray[k] != 0){
                    *(relevantEntitiesEnd++) = entities + (cellArray[k] - 1);
                }
            }
        }
    }

    MakeAnArrayFullOfUniqueItems(gameState, (char*)relevanEntities, (char*)relevantEntitiesEnd, relevantEntitiesEnd - relevanEntities, sizeof(Entity*));

    return relevantEntitiesEnd - relevanEntities;
}

int CalculateRelevantEntitiesForEntity(GameState* gameState, Entity* entity, Entity** relevanEntities, int startOffset) {
    Entity* entities = gameState->entities;
    Entity** relevantEntitiesEnd = relevanEntities;

    for (int v = 0; v < entity->vertexDataEnd - entity->vertexData; v++) {
		int centerRowPosition = (int)(entity->centerPosition.y + entity->vertexData[v].position.y) / (int)gameState->gridSquareEdgeLength;
		int centerColumnPosition = (int)(entity->centerPosition.x + entity->vertexData[v].position.x) / (int)gameState->gridSquareEdgeLength;
        for (int i = -2; i < 3; i++) {
            for (int j = -2; j < 3; j++) {
                if (centerRowPosition + i < 0 || centerRowPosition + i >= gameState->gridDimentions[0] || centerColumnPosition + j < 0 || centerColumnPosition + j >= gameState->gridDimentions[1]) {
                    continue;
                }
                uint32_t* cellArray = gameState->spatialGrid + (((centerRowPosition + i) * gameState->gridDimentions[0] + (centerColumnPosition + j)) * gameState->gridDimentions[2]);
                for (int k = 0; k < gameState->gridDimentions[2]; k++) {
                    //the ids are one added to their indexes on entities array
                    if (cellArray[k] != 0 && cellArray[k] != entity->id && (cellArray[k] - 1) >= startOffset) {
                        *(relevantEntitiesEnd++) = entities + (cellArray[k] - 1);
                    }
                }
            }
        }
    }

    MakeAnArrayFullOfUniqueItems(gameState, (char*)relevanEntities, (char*)relevantEntitiesEnd, relevantEntitiesEnd - relevanEntities, sizeof(Entity*));

    return relevantEntitiesEnd - relevanEntities;
}

void CrossProduct(Vector3& vec1, Vector3& vec2, Vector3& crossProduct) {
    crossProduct.x = vec1.y * vec2.z - vec1.z * vec2.y;
    crossProduct.y = vec1.z * vec2.x - vec1.x * vec2.z;
    crossProduct.z = vec1.x * vec2.y - vec1.y * vec2.x;
}

void NormalVector(Vector3& vector, Vector3& normalVector) {
    Vector3 vector2;
    vector2.x = 0;
    vector2.y = 0;
    vector2.z = 1;

    CrossProduct(vector, vector2, normalVector);
}

CollisionInfo DetectCollisionWithEntity(Entity* e1, Entity* e2) {

    //stupid comment change later
    //because we go over every pair twice if we skip this other calculation will be done anyways
    CollisionInfo collInfo;
    collInfo.minOverlap = 0;
    if (((e1->flags & e2->flags) & GROUND_COLLISION_FLAG) == 0) { return collInfo; }

    double minOverlap = DBL_MAX;
    Vector3 overlapLine;

    int e1NumOfVertices = e1->vertexDataEnd - e1->vertexData;
    int e2NumOfVertices = e2->vertexDataEnd - e2->vertexData;
    for (int i = 0; i < e1NumOfVertices + (e2NumOfVertices); i++) {
        double player1Down = DBL_MAX, player1Up = -DBL_MAX;
        double player2Down = DBL_MAX, player2Up = -DBL_MAX;

        Vector3 vectorOfTwoVertices;
        Vector2 vertex1;
        Vector2 vertex2;
        if (i < e1NumOfVertices) {
            vertex1 = AddVectors(e1->vertexData[i].position, e1->centerPosition);
            if (i + 1 >= e1NumOfVertices) {
                vertex2 = AddVectors(e1->vertexData[0].position, e1->centerPosition);
            }
            else {
                vertex2 = AddVectors(e1->vertexData[i + 1].position, e1->centerPosition);
            }
        }
        else {
            int a = i - (e1NumOfVertices);
            vertex1 = AddVectors(e2->vertexData[a].position, e2->centerPosition);
            if (a + 1 >= e2NumOfVertices) {
                vertex2 = AddVectors(e2->vertexData[0].position, e2->centerPosition);
            }
            else {
                vertex2 = AddVectors(e2->vertexData[a + 1].position, e2->centerPosition);
            }
        }

        vectorOfTwoVertices.x = vertex2.x - vertex1.x;
        vectorOfTwoVertices.y = vertex2.y - vertex1.y;
        vectorOfTwoVertices.z = 0;

        Vector3 normal;
        NormalVector(vectorOfTwoVertices, normal);
        double normalLength = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);

        for (int j = 0; j < e1NumOfVertices; j++) {
            Vector3 posVector;
            //because we are taking the position relative to the center of e1 and vertex positions are relative to it
            //we dont subtract
            posVector.x = e1->vertexData[j].position.x;
            posVector.y = e1->vertexData[j].position.y;
            posVector.z = 0;

            double projectedLen = DotProduct(posVector, normal) / normalLength;

            if (projectedLen > player1Up) {
                player1Up = projectedLen;
            }
            if (projectedLen < player1Down) {
                player1Down = projectedLen;
            }
        }

        for (int j = 0; j < e2NumOfVertices; j++) {
            Vector3 posVector;
            posVector.x = e2->vertexData[j].position.x + e2->centerPosition.x - e1->centerPosition.x;
            posVector.y = e2->vertexData[j].position.y + e2->centerPosition.y - e1->centerPosition.y;
            posVector.z = 0;

            double projectedLen = DotProduct(posVector, normal) / normalLength;
            if (projectedLen > player2Up) {
                player2Up = projectedLen;
            }
            if (projectedLen < player2Down) {
                player2Down = projectedLen;
            }
        }

        if (player1Up <= player2Down || player2Up <= player1Down) {
            minOverlap = 0;
        }
        else {
            double overlap = 0;
            if (player1Up <= player2Up) {
                if (player1Down >= player2Down) {
                    overlap = player1Up - player1Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
                else {
                    overlap = player1Up - player2Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
            }
            else {
                if (player1Down >= player2Down) {
                    overlap = player2Up - player1Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
                else {
                    overlap = player2Up - player2Down;
                    if (overlap < minOverlap) {
                        minOverlap = overlap;
                        overlapLine.x = normal.x;
                        overlapLine.y = normal.y;
                        overlapLine.z = normal.z;
                    }
                }
            }
        }
        if (e1->id == 1 && (minOverlap == DBL_MAX || minOverlap == 0) && (i == (e1NumOfVertices + e2NumOfVertices - 1))) {
            //throw std::runtime_error("as");
        }
    }
    if (minOverlap == DBL_MAX || minOverlap == 0) {
        collInfo.minOverlap = 0;
        collInfo.normalizedOverlapLine = { 0, 0, 0 };
        return collInfo;
    }
    else {
        Vector3 diffOfPositions;
        diffOfPositions.x = e2->centerPosition.x - e1->centerPosition.x;
        diffOfPositions.y = e2->centerPosition.y - e1->centerPosition.y;
        diffOfPositions.z = 0;

        float overlapLineLength = sqrt(pow(overlapLine.x, 2) + pow(overlapLine.y, 2));

        Vector3 normalizedOverlapLine = { overlapLine.x / overlapLineLength, overlapLine.y / overlapLineLength , 0 };

        if (DotProduct(diffOfPositions, overlapLine) < 0) {
            normalizedOverlapLine.x *= -1;
            normalizedOverlapLine.y *= -1;
            normalizedOverlapLine.z *= -1;
        }

        collInfo.normalizedOverlapLine = normalizedOverlapLine;
        collInfo.minOverlap = minOverlap;
        return collInfo;
    }
}

Vector2 CalculateForceApplicationPoint(Entity* e1, Entity* e2, float vertexOutsidePush) {
	Vector2 forceApplicationPoint = { 0, 0 };

	Vector2 centerOfVerticesInsideE2 = { 0, 0 };
	Vector2 centerOfVerticesInsideE1 = { 0, 0 };
    int numberOfVerticesOfE1InsideE2 = CheckHowManyVerticesOfE1IsInE2(e1, e2, centerOfVerticesInsideE2);
    int numberOfVerticesOfE2InsideE1 = CheckHowManyVerticesOfE1IsInE2(e2, e1, centerOfVerticesInsideE1);
    //come up with something for the name
    int divisionNumber = 0;

    if (numberOfVerticesOfE1InsideE2 != 0) {
		centerOfVerticesInsideE2.x /= numberOfVerticesOfE1InsideE2;
		centerOfVerticesInsideE2.y /= numberOfVerticesOfE1InsideE2;
        divisionNumber++;
    }
    if (numberOfVerticesOfE2InsideE1 != 0) {
		centerOfVerticesInsideE1.x /= numberOfVerticesOfE2InsideE1;
		centerOfVerticesInsideE1.y /= numberOfVerticesOfE2InsideE1;
        divisionNumber++;
    }
    if (divisionNumber != 0) {
		forceApplicationPoint.x = (centerOfVerticesInsideE2.x + centerOfVerticesInsideE1.x) / divisionNumber;
		forceApplicationPoint.y = (centerOfVerticesInsideE2.y + centerOfVerticesInsideE1.y) / divisionNumber;
    }

    return forceApplicationPoint;
}

unsigned int CheckHowManyVerticesOfE1IsInE2(Entity* e1, Entity* e2, Vector2& sumOfInsiderPointsPositions) {
    //
	Vector2 centerOfVerticesInsideE2 = { 0, 0 };
	int numberOfVerticesInsideE2 = 0;
    float vertexRelativePositionMagnitude;
    for (int i = 0; i < e1->vertexDataEnd - e1->vertexData; i++) {
		Vector2 vertexPos;
        vertexPos = e1->vertexData[i].position;

        vertexPos.x += e1->centerPosition.x;
        vertexPos.y += e1->centerPosition.y;
        if (CheckIfAPointIsInsideAnEntity(vertexPos, e2)) {
            centerOfVerticesInsideE2.x += vertexPos.x;
            centerOfVerticesInsideE2.y += vertexPos.y;
			numberOfVerticesInsideE2++;
		}
	}
    sumOfInsiderPointsPositions.x = centerOfVerticesInsideE2.x;
    sumOfInsiderPointsPositions.y = centerOfVerticesInsideE2.y;

    return numberOfVerticesInsideE2;
}

void CalculateAndApplyImpulse(GameState* gameState, Entity* e1, Entity* e2, CollisionInfo collInfo, Vector2& impulse, Vector2& relativeVelocityOfForceApplicationPoint, Vector2 forceApplicationPoint, float deltaTime) {
    float push = collInfo.minOverlap - PENETRATION_SLOP;
    float penetrationCorrectionAmount = 0;

    Vector3 distanceVectorOfForceApplicationPointFromE1Center = { forceApplicationPoint.x - e1->centerPosition.x, forceApplicationPoint.y - e1->centerPosition.y };
    Vector3 distanceVectorOfForceApplicationPointFromE2Center = { forceApplicationPoint.x - e2->centerPosition.x, forceApplicationPoint.y - e2->centerPosition.y };
    Vector3 velocityOfE1CausedByRotationalVelocity, velocityOfE2CausedByRotationalVelocity;
    Vector3 rotationalVelocityOfE1 = { 0, 0, e1->rotationalVelocity }, rotationalVelocityOfE2 = { 0, 0, e2->rotationalVelocity };
    CrossProduct(rotationalVelocityOfE1, distanceVectorOfForceApplicationPointFromE1Center, velocityOfE1CausedByRotationalVelocity);
    CrossProduct(rotationalVelocityOfE2, distanceVectorOfForceApplicationPointFromE2Center, velocityOfE2CausedByRotationalVelocity);

    if (e2->gravityApplied == false && ((e2->flags & NON_MOVING_FLAG) == 0)) {
        float speedChangeInY = gameState->gravityConstant * deltaTime;
        relativeVelocityOfForceApplicationPoint.x = (e1->physicsVelocity.x) - (e2->physicsVelocity.x);
        relativeVelocityOfForceApplicationPoint.y = (e1->physicsVelocity.y) - (e2->physicsVelocity.y + speedChangeInY);
		relativeVelocityOfForceApplicationPoint.x += (velocityOfE1CausedByRotationalVelocity.x - velocityOfE2CausedByRotationalVelocity.x);
		relativeVelocityOfForceApplicationPoint.y += (velocityOfE1CausedByRotationalVelocity.y - velocityOfE2CausedByRotationalVelocity.y);
    }
    else{
        relativeVelocityOfForceApplicationPoint.x = (e1->physicsVelocity.x) - (e2->physicsVelocity.x);
        relativeVelocityOfForceApplicationPoint.y = (e1->physicsVelocity.y) - (e2->physicsVelocity.y);
		relativeVelocityOfForceApplicationPoint.x += (velocityOfE1CausedByRotationalVelocity.x - velocityOfE2CausedByRotationalVelocity.x);
		relativeVelocityOfForceApplicationPoint.y += (velocityOfE1CausedByRotationalVelocity.y - velocityOfE2CausedByRotationalVelocity.y);
    }

    if (collInfo.minOverlap > PENETRATION_SLOP) {
        float relativeVelOnCollisionLine = DotProduct(collInfo.normalizedOverlapLine, relativeVelocityOfForceApplicationPoint);
        /*if ((e1->flags | e2->flags) & NON_MOVING_FLAG && false) {
			penetrationCorrectionAmount = ((collInfo.minOverlap - PENETRATION_SLOP) / deltaTime);
        }
        else {
        }*/
		penetrationCorrectionAmount = ((BAUMGARTE_BETA * 1) * (collInfo.minOverlap - PENETRATION_SLOP) / GetFrameTime()) / gameState->SOLVER_ITERATIONS;
    }

    float velAlongNormal = DotProduct(collInfo.normalizedOverlapLine, relativeVelocityOfForceApplicationPoint);

    float e;
    if ((e1->flags | e2->flags) & NON_MOVING_FLAG) {
        e = 0;
    }
    else {
        e = e1->elasticity > e2->elasticity ? e1->elasticity : e2->elasticity;
    }
    float r1CrossN = distanceVectorOfForceApplicationPointFromE1Center.x * collInfo.normalizedOverlapLine.y - distanceVectorOfForceApplicationPointFromE1Center.y * collInfo.normalizedOverlapLine.x;
    float r2CrossN = distanceVectorOfForceApplicationPointFromE2Center.x * collInfo.normalizedOverlapLine.y - distanceVectorOfForceApplicationPointFromE2Center.y * collInfo.normalizedOverlapLine.x;

    float inv1Inertia = e1->flags & NON_MOVING_FLAG ? 0 : (1 / e1->inertia);
    float inv2Inertia = e2->flags & NON_MOVING_FLAG ? 0 : (1 / e2->inertia);

    float rotationalMass1 = (r1CrossN * r1CrossN) * inv1Inertia;
    float rotationalMass2 = (r2CrossN * r2CrossN) * inv2Inertia;

    float inv1mass = e1->flags & NON_MOVING_FLAG ? 0 : (1 / e1->mass);
    float inv2mass = e2->flags & NON_MOVING_FLAG ? 0 : (1 / e2->mass);
    float sumOfInverseMasses = inv1mass + inv2mass + rotationalMass1 + rotationalMass2;
    if (sumOfInverseMasses == 0.0f) {
		return;
	}

    float j = (-(1.0 + e) * velAlongNormal) - penetrationCorrectionAmount;
    if (j > EPSILON) {
        j = 0;
    }
    j /= sumOfInverseMasses;

    impulse.x += j * collInfo.normalizedOverlapLine.x;
    impulse.y += j * collInfo.normalizedOverlapLine.y;


    Vector2 impulseForce = { impulse.x / deltaTime, impulse.y / deltaTime };
    ApplyForceToEntitiesVelocityImmediately(e1, impulseForce, deltaTime, forceApplicationPoint);

    if (!(e2->flags & NON_MOVING_FLAG)) {
        ApplyForceToEntitiesVelocityImmediately(e2, { -impulseForce.x, -impulseForce.y }, deltaTime, forceApplicationPoint);
    }
}

void HandleFriction(GameState* gameState, Entity* e1, Entity* e2, CollisionInfo collInfo, Vector2& impulse, Vector2& relativeVelocityOfForceApplicationPoint, Vector2 forceApplicationPoint, float deltaTime) {
    Vector3 distanceOfForceApplicationPointFromE1Center = { forceApplicationPoint.x - e1->centerPosition.x, forceApplicationPoint.y - e1->centerPosition.y };
    Vector3 distanceOfForceApplicationPointFromE2Center = { forceApplicationPoint.x - e2->centerPosition.x, forceApplicationPoint.y - e2->centerPosition.y };
    Vector3 velocityOfE1CausedByRotationalVelocity, velocityOfE2CausedByRotationalVelocity;
    Vector3 rotationalVelocityOfE1 = { 0, 0, e1->rotationalVelocity }, rotationalVelocityOfE2 = { 0, 0, e2->rotationalVelocity };
    CrossProduct(rotationalVelocityOfE1, distanceOfForceApplicationPointFromE1Center, velocityOfE1CausedByRotationalVelocity);
    CrossProduct(rotationalVelocityOfE2, distanceOfForceApplicationPointFromE2Center, velocityOfE2CausedByRotationalVelocity);

    if (e2->gravityApplied == false && ((e2->flags & NON_MOVING_FLAG) == 0)) {
        float speedChangeInY = gameState->gravityConstant * deltaTime;
        relativeVelocityOfForceApplicationPoint.x = (e1->physicsVelocity.x) - (e2->physicsVelocity.x);
        relativeVelocityOfForceApplicationPoint.y = (e1->physicsVelocity.y) - (e2->physicsVelocity.y + speedChangeInY);
        relativeVelocityOfForceApplicationPoint.x += (velocityOfE1CausedByRotationalVelocity.x - velocityOfE2CausedByRotationalVelocity.x);
        relativeVelocityOfForceApplicationPoint.y += (velocityOfE1CausedByRotationalVelocity.y - velocityOfE2CausedByRotationalVelocity.y);
    }
    else{
        relativeVelocityOfForceApplicationPoint.x = (e1->physicsVelocity.x) - (e2->physicsVelocity.x);
        relativeVelocityOfForceApplicationPoint.y = (e1->physicsVelocity.y) - (e2->physicsVelocity.y);
        relativeVelocityOfForceApplicationPoint.x += (velocityOfE1CausedByRotationalVelocity.x - velocityOfE2CausedByRotationalVelocity.x);
        relativeVelocityOfForceApplicationPoint.y += (velocityOfE1CausedByRotationalVelocity.y - velocityOfE2CausedByRotationalVelocity.y);
    }

    Vector3 k = { 0, 0, 1 };
    Vector3 frictionAxis;
    CrossProduct(collInfo.normalizedOverlapLine, k, frictionAxis);

    // Get the relative speed along the friction tangent
    float relativeVelOnFrictionAxis = DotProduct(frictionAxis, relativeVelocityOfForceApplicationPoint);

    // Calculate rotational mass specifically for the friction axis
    float r1CrossT = distanceOfForceApplicationPointFromE1Center.x * frictionAxis.y - distanceOfForceApplicationPointFromE1Center.y * frictionAxis.x;
    float r2CrossT = distanceOfForceApplicationPointFromE2Center.x * frictionAxis.y - distanceOfForceApplicationPointFromE2Center.y * frictionAxis.x;

    float inv1Inertia = e1->flags & NON_MOVING_FLAG ? 0 : (1 / e1->inertia);
    float inv2Inertia = e2->flags & NON_MOVING_FLAG ? 0 : (1 / e2->inertia);
    float inv1mass = e1->flags & NON_MOVING_FLAG ? 0 : (1 / e1->mass);
    float inv2mass = e2->flags & NON_MOVING_FLAG ? 0 : (1 / e2->mass);

    float rotationalMass1T = (r1CrossT * r1CrossT) * inv1Inertia;
    float rotationalMass2T = (r2CrossT * r2CrossT) * inv2Inertia;
    float sumOfInverseMassesT = inv1mass + inv2mass + rotationalMass1T + rotationalMass2T;
    
    //throw std::runtime_error("make friction stop objects when you try making a tower they slide off");
    if (sumOfInverseMassesT > EPSILON && abs(relativeVelOnFrictionAxis) > EPSILON) {
        float frictionConst = e1->frictionCons > e2->frictionCons ? e1->frictionCons : e2->frictionCons;
        float impulseMagnitude = sqrt(pow(impulse.x, 2) + pow(impulse.y, 2));
        float friction = frictionConst * impulseMagnitude;

        int frictionDir = -relativeVelOnFrictionAxis / abs(relativeVelOnFrictionAxis);

        Vector2 frictionImpulseForce = { friction * frictionAxis.x * frictionDir , friction * frictionAxis.y * frictionDir };

        ApplyForceToEntitiesVelocityImmediately(e1, frictionImpulseForce, deltaTime, forceApplicationPoint);
        if (!(e2->flags & NON_MOVING_FLAG)) {
            ApplyForceToEntitiesVelocityImmediately(e2, { -frictionImpulseForce.x, -frictionImpulseForce.y }, deltaTime, forceApplicationPoint);
        }
    }
    //throw std::runtime_error("look at the videos for the jumping bug");
    return;
}

namespace delaunator {
//@see https://stackoverflow.com/questions/33333363/built-in-mod-vs-custom-mod-function-improve-the-performance-of-modulus-op/33333636#33333636
inline size_t fast_mod(const size_t i, const size_t c) {
    return i >= c ? i % c : i;
}

// Kahan and Babuska summation, Neumaier variant; accumulates less FP error
inline double sum(const std::vector<double>& x) {
    double sum = x[0];
    double err = 0.0;

    for (size_t i = 1; i < x.size(); i++) {
        const double k = x[i];
        const double m = sum + k;
        err += std::fabs(sum) >= std::fabs(k) ? sum - m + k : k - m + sum;
        sum = m;
    }
    return sum + err;
}

inline double dist(
    const double ax,
    const double ay,
    const double bx,
    const double by) {
    const double dx = ax - bx;
    const double dy = ay - by;
    return dx * dx + dy * dy;
}

inline double circumradius(
    const double ax,
    const double ay,
    const double bx,
    const double by,
    const double cx,
    const double cy) {
    const double dx = bx - ax;
    const double dy = by - ay;
    const double ex = cx - ax;
    const double ey = cy - ay;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = (ey * bl - dy * cl) * 0.5 / d;
    const double y = (dx * cl - ex * bl) * 0.5 / d;

    if ((bl > 0.0 || bl < 0.0) && (cl > 0.0 || cl < 0.0) && (d > 0.0 || d < 0.0)) {
        return x * x + y * y;
    } else {
        return std::numeric_limits<double>::max();
    }
}

inline bool orient(
    const double px,
    const double py,
    const double qx,
    const double qy,
    const double rx,
    const double ry) {
    return (qy - py) * (rx - qx) - (qx - px) * (ry - qy) < 0.0;
}

inline std::pair<double, double> circumcenter(
    const double ax,
    const double ay,
    const double bx,
    const double by,
    const double cx,
    const double cy) {
    const double dx = bx - ax;
    const double dy = by - ay;
    const double ex = cx - ax;
    const double ey = cy - ay;

    const double bl = dx * dx + dy * dy;
    const double cl = ex * ex + ey * ey;
    const double d = dx * ey - dy * ex;

    const double x = ax + (ey * bl - dy * cl) * 0.5 / d;
    const double y = ay + (dx * cl - ex * bl) * 0.5 / d;

    return std::make_pair(x, y);
}

struct compare {

    const VertexData* vertexData;
    double cx;
    double cy;

    bool operator()(std::size_t i, std::size_t j) {
        const double d1 = dist(vertexData[i].position.x, vertexData[i].position.y, cx, cy);
        const double d2 = dist(vertexData[j].position.x, vertexData[j].position.y, cx, cy);
        const double diff1 = d1 - d2;
        const double diff2 = vertexData[i].position.x - vertexData[j].position.x;
        const double diff3 = vertexData[i].position.y - vertexData[j].position.y;

        if (diff1 > 0.0 || diff1 < 0.0) {
            return diff1 < 0;
        } else if (diff2 > 0.0 || diff2 < 0.0) {
            return diff2 < 0;
        } else {
            return diff3 < 0;
        }
    }
};

inline bool in_circle(
    const double ax,
    const double ay,
    const double bx,
    const double by,
    const double cx,
    const double cy,
    const double px,
    const double py) {
    const double dx = ax - px;
    const double dy = ay - py;
    const double ex = bx - px;
    const double ey = by - py;
    const double fx = cx - px;
    const double fy = cy - py;

    const double ap = dx * dx + dy * dy;
    const double bp = ex * ex + ey * ey;
    const double cp = fx * fx + fy * fy;

    return (dx * (ey * cp - bp * fy) -
            dy * (ex * cp - bp * fx) +
            ap * (ex * fy - ey * fx)) < 0.0;
}

constexpr double EPSILON = std::numeric_limits<double>::epsilon();
constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();

inline bool check_pts_equal(double x1, double y1, double x2, double y2) {
    return std::fabs(x1 - x2) <= EPSILON &&
           std::fabs(y1 - y2) <= EPSILON;
}

// monotonically increases with real angle, but doesn't need expensive trigonometry
inline double pseudo_angle(const double dx, const double dy) {
    const double p = dx / (std::abs(dx) + std::abs(dy));
    return (dy > 0.0 ? 3.0 - p : 1.0 + p) / 4.0; // [0..1)
}

struct DelaunatorPoint {
    std::size_t i;
    double x;
    double y;
    std::size_t t;
    std::size_t prev;
    std::size_t next;
    bool removed;
};

class Delaunator {

public:
    GameState* gameState;
    const VertexData* vertexData;
    size_t numData;
    unsigned int* triangles;
    unsigned int* trianglesEnd;
    std::vector<std::size_t> halfedges;
    std::vector<std::size_t> hull_prev;
    std::vector<std::size_t> hull_next;
    std::vector<std::size_t> hull_tri;
    std::size_t hull_start;

    Delaunator(const VertexData* in_coords, size_t numData, GameState* state);

    double get_hull_area();

private:
    std::vector<std::size_t> m_hash;
    double m_center_x;
    double m_center_y;
    std::size_t m_hash_size;
    std::vector<std::size_t> m_edge_stack;

    std::size_t legalize(std::size_t a);
    std::size_t hash_key(double x, double y) const;
    std::size_t add_triangle(
        std::size_t i0,
        std::size_t i1,
        std::size_t i2,
        std::size_t a,
        std::size_t b,
        std::size_t c);
    void link(std::size_t a, std::size_t b);
};

Delaunator::Delaunator(const VertexData* in_data, size_t numData, GameState* state)
    : vertexData(in_data),
      numData(numData),
      gameState(state),
      triangles(),
      trianglesEnd(),
      halfedges(),
      hull_prev(),
      hull_next(),
      hull_tri(),
      hull_start(),
      m_hash(),
      m_center_x(),
      m_center_y(),
      m_hash_size(),
      m_edge_stack() {
    std::size_t n = numData;

    double max_x = std::numeric_limits<double>::min();
    double max_y = std::numeric_limits<double>::min();
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    std::vector<std::size_t> ids;
    ids.reserve(n);

    for (std::size_t i = 0; i < n; i++) {
        const double x = vertexData[i].position.x;
        const double y = vertexData[i].position.y;

        if (x < min_x) min_x = x;
        if (y < min_y) min_y = y;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;

        ids.push_back(i);
    }
    const double cx = (min_x + max_x) / 2;
    const double cy = (min_y + max_y) / 2;
    double min_dist = std::numeric_limits<double>::max();

    std::size_t i0 = INVALID_INDEX;
    std::size_t i1 = INVALID_INDEX;
    std::size_t i2 = INVALID_INDEX;

    // pick a seed point close to the centroid
    for (std::size_t i = 0; i < n; i++) {
        const double d = dist(cx, cy, vertexData[i].position.x, vertexData[i].position.y);
        if (d < min_dist) {
            i0 = i;
            min_dist = d;
        }
    }

    const double i0x = vertexData[i0].position.x;
    const double i0y = vertexData[i0].position.y;

    min_dist = std::numeric_limits<double>::max();

    // find the point closest to the seed
    for (std::size_t i = 0; i < n; i++) {
        if (i == i0) continue;
        const double d = dist(i0x, i0y, vertexData[i].position.x, vertexData[i].position.y);
        if (d < min_dist && d > 0.0) {
            i1 = i;
            min_dist = d;
        }
    }

    double i1x = vertexData[i1].position.x;
    double i1y = vertexData[i1].position.y;

    double min_radius = std::numeric_limits<double>::max();

    // find the third point which forms the smallest circumcircle with the first two
    for (std::size_t i = 0; i < n; i++) {
        if (i == i0 || i == i1) continue;

        const double r = circumradius(
            i0x, i0y, i1x, i1y, vertexData[i].position.x, vertexData[i].position.y);

        if (r < min_radius) {
            i2 = i;
            min_radius = r;
        }
    }

    if (!(min_radius < std::numeric_limits<double>::max())) {
        throw std::runtime_error("not triangulation");
    }

    double i2x = vertexData[i2].position.x;
    double i2y = vertexData[i2].position.y;

    if (orient(i0x, i0y, i1x, i1y, i2x, i2y)) {
        std::swap(i1, i2);
        std::swap(i1x, i2x);
        std::swap(i1y, i2y);
    }

    std::tie(m_center_x, m_center_y) = circumcenter(i0x, i0y, i1x, i1y, i2x, i2y);

    // sort the points by distance from the seed triangle circumcenter
    std::sort(ids.begin(), ids.end(), compare{ vertexData, m_center_x, m_center_y });

    // initialize a hash table for storing edges of the advancing convex hull
    m_hash_size = static_cast<std::size_t>(std::llround(std::ceil(std::sqrt(n))));
    m_hash.resize(m_hash_size);
    std::fill(m_hash.begin(), m_hash.end(), INVALID_INDEX);

    // initialize arrays for tracking the edges of the advancing convex hull
    hull_prev.resize(n);
    hull_next.resize(n);
    hull_tri.resize(n);

    hull_start = i0;

    size_t hull_size = 3;

    hull_next[i0] = hull_prev[i2] = i1;
    hull_next[i1] = hull_prev[i0] = i2;
    hull_next[i2] = hull_prev[i1] = i0;

    hull_tri[i0] = 0;
    hull_tri[i1] = 1;
    hull_tri[i2] = 2;

    m_hash[hash_key(i0x, i0y)] = i0;
    m_hash[hash_key(i1x, i1y)] = i1;
    m_hash[hash_key(i2x, i2y)] = i2;

    std::size_t max_triangles = n < 3 ? 1 : 2 * n - 5;
    triangles = (unsigned int*)PushSize(gameState, (max_triangles * 3) * sizeof(unsigned int));
    trianglesEnd = triangles;
    halfedges.reserve(max_triangles * 3);
    add_triangle(i0, i1, i2, INVALID_INDEX, INVALID_INDEX, INVALID_INDEX);
    double xp = std::numeric_limits<double>::quiet_NaN();
    double yp = std::numeric_limits<double>::quiet_NaN();
    for (std::size_t k = 0; k < n; k++) {
        const std::size_t i = ids[k];
        const double x = vertexData[i].position.x;
        const double y = vertexData[i].position.y;

        // skip near-duplicate points
        if (k > 0 && check_pts_equal(x, y, xp, yp)) continue;
        xp = x;
        yp = y;

        // skip seed triangle points
        if (
            check_pts_equal(x, y, i0x, i0y) ||
            check_pts_equal(x, y, i1x, i1y) ||
            check_pts_equal(x, y, i2x, i2y)) continue;

        // find a visible edge on the convex hull using edge hash
        std::size_t start = 0;

        size_t key = hash_key(x, y);
        for (size_t j = 0; j < m_hash_size; j++) {
            start = m_hash[fast_mod(key + j, m_hash_size)];
            if (start != INVALID_INDEX && start != hull_next[start]) break;
        }

        start = hull_prev[start];
        size_t e = start;
        size_t q;

        while (q = hull_next[e], !orient(x, y, vertexData[e].position.x, vertexData[e].position.y, vertexData[q].position.x, vertexData[q].position.y)) { //TODO: does it works in a same way as in JS
            e = q;
            if (e == start) {
                e = INVALID_INDEX;
                break;
            }
        }

        if (e == INVALID_INDEX) continue; // likely a near-duplicate point; skip it

        // add the first triangle from the point
        std::size_t t = add_triangle(
            e,
            i,
            hull_next[e],
            INVALID_INDEX,
            INVALID_INDEX,
            hull_tri[e]);

        hull_tri[i] = legalize(t + 2);
        hull_tri[e] = t;
        hull_size++;

        // walk forward through the hull, adding more triangles and flipping recursively
        std::size_t next = hull_next[e];
        while (
            q = hull_next[next],
            orient(x, y, vertexData[next].position.x, vertexData[next].position.y, vertexData[q].position.x, vertexData[q].position.y)) {
            t = add_triangle(next, i, q, hull_tri[i], INVALID_INDEX, hull_tri[next]);
            hull_tri[i] = legalize(t + 2);
            hull_next[next] = next; // mark as removed
            hull_size--;
            next = q;
        }

        // walk backward from the other side, adding more triangles and flipping
        if (e == start) {
            while (
                q = hull_prev[e],
                orient(x, y, vertexData[q].position.x, vertexData[q].position.y, vertexData[e].position.x, vertexData[e].position.y)) {
                t = add_triangle(q, i, e, INVALID_INDEX, hull_tri[e], hull_tri[q]);
                legalize(t + 2);
                hull_tri[q] = t;
                hull_next[e] = e; // mark as removed
                hull_size--;
                e = q;
            }
        }

        // update the hull indices
        hull_prev[i] = e;
        hull_start = e;
        hull_prev[next] = i;
        hull_next[e] = i;
        hull_next[i] = next;

        m_hash[hash_key(x, y)] = i;
        m_hash[hash_key(vertexData[e].position.x, vertexData[e].position.y)] = e;
    }
}

double Delaunator::get_hull_area() {
    std::vector<double> hull_area;
    size_t e = hull_start;
    do {
        hull_area.push_back((vertexData[e].position.x - vertexData[hull_prev[e]].position.x) * (vertexData[e].position.y + vertexData[hull_prev[e]].position.y));
        e = hull_next[e];
    } while (e != hull_start);
    return sum(hull_area);
}

std::size_t Delaunator::legalize(std::size_t a) {
    std::size_t i = 0;
    std::size_t ar = 0;
    m_edge_stack.clear();

    // recursion eliminated with a fixed-size stack
    while (true) {
        const size_t b = halfedges[a];

        /* if the pair of triangles doesn't satisfy the Delaunay condition
        * (p1 is inside the circumcircle of [p0, pl, pr]), flip them,
        * then do the same check/flip recursively for the new pair of triangles
        *
        *           pl                    pl
        *          /||\                  /  \
        *       al/ || \bl            al/    \a
        *        /  ||  \              /      \
        *       /  a||b  \    flip    /___ar___\
        *     p0\   ||   /p1   =>   p0\---bl---/p1
        *        \  ||  /              \      /
        *       ar\ || /br             b\    /br
        *          \||/                  \  /
        *           pr                    pr
        */
        const size_t a0 = 3 * (a / 3);
        ar = a0 + (a + 2) % 3;

        if (b == INVALID_INDEX) {
            if (i > 0) {
                i--;
                a = m_edge_stack[i];
                continue;
            } else {
                //i = INVALID_INDEX;
                break;
            }
        }

        const size_t b0 = 3 * (b / 3);
        const size_t al = a0 + (a + 1) % 3;
        const size_t bl = b0 + (b + 2) % 3;

        const std::size_t p0 = triangles[ar];
        const std::size_t pr = triangles[a];
        const std::size_t pl = triangles[al];
        const std::size_t p1 = triangles[bl];

        const bool illegal = in_circle(
            vertexData[p0].position.x,
            vertexData[p0].position.y,
            vertexData[pr].position.x,
            vertexData[pr].position.y,
            vertexData[pl].position.x,
            vertexData[pl].position.y,
            vertexData[p1].position.x,
            vertexData[p1].position.y);

        if (illegal) {
            triangles[a] = p1;
            triangles[b] = p0;

            auto hbl = halfedges[bl];

            // edge swapped on the other side of the hull (rare); fix the halfedge reference
            if (hbl == INVALID_INDEX) {
                std::size_t e = hull_start;
                do {
                    if (hull_tri[e] == bl) {
                        hull_tri[e] = a;
                        break;
                    }
                    e = hull_next[e];
                } while (e != hull_start);
            }
            link(a, hbl);
            link(b, halfedges[ar]);
            link(ar, bl);
            std::size_t br = b0 + (b + 1) % 3;

            if (i < m_edge_stack.size()) {
                m_edge_stack[i] = br;
            } else {
                m_edge_stack.push_back(br);
            }
            i++;

        } else {
            if (i > 0) {
                i--;
                a = m_edge_stack[i];
                continue;
            } else {
                break;
            }
        }
    }
    return ar;
}

inline std::size_t Delaunator::hash_key(const double x, const double y) const {
    const double dx = x - m_center_x;
    const double dy = y - m_center_y;
    return fast_mod(
        static_cast<std::size_t>(std::llround(std::floor(pseudo_angle(dx, dy) * static_cast<double>(m_hash_size)))),
        m_hash_size);
}

std::size_t Delaunator::add_triangle(
    std::size_t i0,
    std::size_t i1,
    std::size_t i2,
    std::size_t a,
    std::size_t b,
    std::size_t c) {
    std::size_t t = trianglesEnd - triangles;
    *(trianglesEnd) = i0;
    trianglesEnd++;
    *(trianglesEnd) = i1;
    trianglesEnd++;
    *(trianglesEnd) = i2;
    trianglesEnd++;
    link(t, a);
    link(t + 1, b);
    link(t + 2, c);
    return t;
}

void Delaunator::link(const std::size_t a, const std::size_t b) {
    std::size_t s = halfedges.size();
    if (a == s) {
        halfedges.push_back(b);
    } else if (a < s) {
        halfedges[a] = b;
    } else {
        throw std::runtime_error("Cannot link edge");
    }
    if (b != INVALID_INDEX) {
        std::size_t s2 = halfedges.size();
        if (b == s2) {
            halfedges.push_back(a);
        } else if (b < s2) {
            halfedges[b] = a;
        } else {
            throw std::runtime_error("Cannot link edge");
        }
    }
}

} //namespace delaunator



void Triangulate2DPoints(VertexData* begin, size_t numOfPoints, GameState* gameState, unsigned int** indices, unsigned int** indicesEnd) {

	if (numOfPoints <= 3) {
        *indices = (unsigned int*)PushSize(gameState, sizeof(unsigned int) * numOfPoints);
        *indicesEnd = *indices;
		for (int i = 0; i < numOfPoints; i++) {
			**indicesEnd = i;
			(*indicesEnd)++;
		}
		return;
	}

	delaunator::Delaunator d(begin, numOfPoints, gameState);

	size_t numOfIndices = d.trianglesEnd - d.triangles;
	*indices = d.triangles;
	*indicesEnd = d.trianglesEnd;

};

void ApplyGravityCalculatePhysicsAndMoveEntity(GameState* gameState, Entity* entity) {
    if((entity->flags & GRAVITY_FLAG) > 0){
        entity->netForce.y += gameState->gravityConstant * entity->mass;
    }
    MoveAndRotateEntity(entity, 0);
    entity->physicsVelocity = { 0, 0 };
}
