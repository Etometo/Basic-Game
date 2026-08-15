#include "GameState.h"
#include <iostream>

void UpdateGameplayScreen(GameState* gameState, MouseInputInfo mouseInput) {

	uint32_t relevantEntitiesSize = sizeof(Entity*) * 500;
	Entity** relevantEntities = (Entity**)PushTemporarySize(gameState, relevantEntitiesSize);

	if (gameState->readyForNewEntityInitialization) {
		gameState->entityBeingCut = (Entity*)InitializeAndPushEntity(gameState, gameState->rectData, gameState->rectDataEnd, 10, BEING_CUT_FLAG, { 400, 100 });
		gameState->readyForNewEntityInitialization = false;
		gameState->entityInitialized = true;
	}

	if (mouseInput.mouseReleasedThisFrame) {
		//hold and release
		if (mouseInput.inputDurationFrames > gameState->ClickThresholdFrames) {
			Entity* newEntity1 = 0;
			Entity* newEntity2 = 0;
			uint32_t newEntityFlags = BEING_CHOSEN_FLAG;
			int operationStatus = CutEntityIntoTwoPiecesByALine(gameState, gameState->entityBeingCut, mouseInput.inputPositions[0], mouseInput.inputPositions[1], newEntityFlags, newEntity1, newEntity2);
			if (operationStatus == ENTITY_WAS_CUT) {
				gameState->cutPiece1 = newEntity1;
				gameState->cutPiece2 = newEntity2;
				gameState->cutPiece1->centerPosition.x -= 100;
				gameState->cutPiece1->temporaryPositionChange.x = -100;
				gameState->cutPiece2->centerPosition.x += 100;
				gameState->cutPiece2->temporaryPositionChange.x = 100;

				DeleteEntity(gameState, gameState->entityBeingCut);
			}
		}
		//click
		if (mouseInput.inputDurationFrames < gameState->ClickThresholdFrames) {
			std::cout << "click" << std::endl;
			Vector2 mousePos = GetMousePosition();
			int numOfCloseEntities = CalculateRelevantEntitiesForPosition(gameState, mousePos, relevantEntities);
			bool selectedAnEntity = false;
			for (int i = 0; i < numOfCloseEntities; i++) {
				Entity* entity = relevantEntities[i];
				bool pointIsInsideEntity = CheckIfAPointIsInsideAnEntity(mousePos, entity);
				if (pointIsInsideEntity) {
					std::cout << "Entity " << entity->id << " clicked" << std::endl;
				}
				if (pointIsInsideEntity && (entity->flags & BEING_CHOSEN_FLAG)) {
					if (entity->id == gameState->cutPiece1->id) {
						DeleteEntity(gameState, gameState->cutPiece2);
						gameState->cutPiece1->flags &= !BEING_CHOSEN_FLAG;
						gameState->cutPiece1->flags |= (PHYSICS_FLAG | GRAVITY_FLAG | GROUND_COLLISION_FLAG);
						gameState->cutPiece1->centerPosition.x -= gameState->cutPiece1->temporaryPositionChange.x;
						gameState->cutPiece1->centerPosition.y -= gameState->cutPiece1->temporaryPositionChange.y;
						selectedAnEntity = true;
					}
					else if (entity->id == gameState->cutPiece2->id) {
						DeleteEntity(gameState, gameState->cutPiece1);
						gameState->cutPiece2->flags &= !BEING_CHOSEN_FLAG;
						gameState->cutPiece2->flags |= (PHYSICS_FLAG | GRAVITY_FLAG | GROUND_COLLISION_FLAG);
						gameState->cutPiece2->centerPosition.x -= gameState->cutPiece2->temporaryPositionChange.x;
						gameState->cutPiece2->centerPosition.y -= gameState->cutPiece2->temporaryPositionChange.y;
						selectedAnEntity = true;
					}
					gameState->cutPiece1 = nullptr;
					gameState->cutPiece2 = nullptr;
					gameState->readyForNewEntityInitialization = true;
				}
			}
			if (selectedAnEntity == false) {
				std::cout << " ";
			}
		}
	}
	else {
		if (mouseInput.inputType == LEFT_CLICK) {
			DrawLine(mouseInput.inputPositions[0].x, mouseInput.inputPositions[0].y, mouseInput.inputPositions[1].x, mouseInput.inputPositions[1].y, RED);
		}
	}

	float deltaTime = GetDeltaTime();
	for (int i = 0; i < gameState->lastEntityOnEntities + 1 - gameState->entities; i++) {
		Entity* entity = gameState->entities + i;
		if (entity->id == 0) {
			continue;
		}
		int numOfRelevantEntities = CalculateRelevantEntitiesForEntity(gameState, entity, relevantEntities, i);
		float solverIterationTimeStep = deltaTime / gameState->SOLVER_ITERATIONS;
		if ((entity->flags & PHYSICS_FLAG)) {
			if ((entity->flags & GRAVITY_FLAG) > 0) {
				float forcePerVertex = (entity->mass * gameState->gravityConstant) / (entity->vertexDataEnd - entity->vertexData);
				for (int v = 0; v < entity->vertexDataEnd - entity->vertexData; v++) {
					Vector2 vertexPos = entity->centerPosition;
					vertexPos.x += entity->vertexData[v].position.x;
					vertexPos.y += entity->vertexData[v].position.y;
					ApplyForceToEntitiesVelocityImmediately(entity, { 0, forcePerVertex}, deltaTime, vertexPos);
				}
				entity->gravityApplied = true;
			}
			for (int j = 0; j < numOfRelevantEntities; j++) {
				Entity* relevantEntity = relevantEntities[j];
				if ((relevantEntity->flags & PHYSICS_FLAG) == 0) {
					continue;
				}
				CollisionInfo collInfo = DetectCollisionWithEntity(entity, relevantEntity);
				float totalMass = entity->mass + relevantEntity->mass;

				if (collInfo.minOverlap > 0) {
					Vector2 forceApplicationPoint = CalculateForceApplicationPoint(entity, relevantEntity);
					if (forceApplicationPoint.x == 0 && forceApplicationPoint.y == 0) {
						forceApplicationPoint = entity->centerPosition;
					}
					
					for(int k = 0; k < gameState->SOLVER_ITERATIONS; k++){
						Vector2 impulse = { 0, 0 }, relativeVelocityOfForceApplicationPoint = {0, 0};
						CalculateAndApplyImpulse(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
						HandleFriction(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
					}
				}
			}
			MoveEntity(entity, deltaTime);
			CalibrateEntityWithGrid(gameState, entity);
			DrawEntityForceLine(entity);
			entity->netForce = { 0, 0 };
			entity->forceAppliedToAccelerationAndVelocity = { 0, 0 };
			entity->forcesMultipliedByAppliedTime = { 0, 0 };
			entity->torque = 0;
		}
		DrawEntity(entity);
	}

	for (int i = 0; i < gameState->addedEntities; i++) {
		(gameState->entities + i)->gravityApplied = false;
	}

	RetractTemporarySize(gameState, relevantEntitiesSize);

}



