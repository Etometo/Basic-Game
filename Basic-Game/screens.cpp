#include "GameState.h"
#include <iostream>

void InitializeGameplayScreen(GameState* gameState) {
	VertexData* rectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* rectDataEnd = rectData;
	*(rectDataEnd++) = VertexData{ 50, 50 };
	*(rectDataEnd++) = VertexData{ 50, -50 };
	*(rectDataEnd++) = VertexData{ -50, -50 };
	*(rectDataEnd++) = VertexData{ -50, 50 };
	gameState->rectData = rectData;
	gameState->rectDataEnd = rectDataEnd;

	VertexData* floorRectData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 6);
	VertexData* floorRectDataEnd = floorRectData;
	*(floorRectDataEnd++) = VertexData{ 399, 50 };
	*(floorRectDataEnd++) = VertexData{ 399, -50 };
	*(floorRectDataEnd++) = VertexData{ 0, -50 };
	*(floorRectDataEnd++) = VertexData{ -399, -50 };
	*(floorRectDataEnd++) = VertexData{ -399, 50 };
	*(floorRectDataEnd++) = VertexData{ 0, 50 };

	VertexData* triData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 3);
	VertexData* triDataEnd = triData;
	*(triDataEnd++) = VertexData{ 33.3333f,  33.3333f };
	*(triDataEnd++) = VertexData{ 33.3333f, -66.6667f };
	*(triDataEnd++) = VertexData{ -66.6667f,  33.3333f };

	VertexData* tri2Data = (VertexData*)PushSize(gameState, sizeof(VertexData) * 3);
	VertexData* tri2DataEnd = tri2Data;
	*(tri2DataEnd++) = VertexData{ -33.3333f, -33.3333f };
	*(tri2DataEnd++) = VertexData{ -33.3333f,  66.6667f };
	*(tri2DataEnd++) = VertexData{ 66.6667f, -33.3333f };

	Vector2 floorCenterPos = { 400, 750 };
	Entity* floor = InitializeAndPushEntity(gameState, floorRectData, floorRectDataEnd, 0.2, NON_MOVING_FLAG | GROUND_COLLISION_FLAG | PHYSICS_FLAG, floorCenterPos, GAMEPLAY_SCREEN);
	floor->frictionCons = 0.1;
}

void UpdateGameplayScreen(GameState* gameState, InputInfo inputInfo) {
	uint32_t relevantEntitiesSize = sizeof(Entity*) * 500;
	Entity** relevantEntities = (Entity**)PushTemporarySize(gameState, relevantEntitiesSize);

	if (gameState->readyForNewEntityInitialization) {
		gameState->entityBeingCut = (Entity*)InitializeAndPushEntity(gameState, gameState->rectData, gameState->rectDataEnd, 10, BEING_CUT_FLAG, { 400, 100 }, GAMEPLAY_SCREEN);
		gameState->readyForNewEntityInitialization = false;
		gameState->entityInitialized = true;
	}

	if (inputInfo.mouseInputInfo.mouseReleasedThisFrame) {
		//hold and release
		if (inputInfo.mouseInputInfo.inputDurationFrames > gameState->ClickThresholdFrames) {
			Entity* newEntity1 = 0;
			Entity* newEntity2 = 0;
			uint32_t newEntityFlags = BEING_CHOSEN_FLAG;
			int operationStatus = CutEntityIntoTwoPiecesByALine(gameState, gameState->entityBeingCut, inputInfo.mouseInputInfo.inputPositions[0], inputInfo.mouseInputInfo.inputPositions[1], newEntityFlags, newEntity1, newEntity2);
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
		if (inputInfo.mouseInputInfo.inputDurationFrames < gameState->ClickThresholdFrames) {
			std::cout << "click" << std::endl;
			Vector2 mousePos = inputInfo.mouseInputInfo.inputPositions[1];
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
						gameState->cutPiece1->flags |= ADJUSTING_POSITION_FLAG;
						gameState->cutPiece1->centerPosition.x -= gameState->cutPiece1->temporaryPositionChange.x;
						gameState->cutPiece1->centerPosition.y -= gameState->cutPiece1->temporaryPositionChange.y;
						gameState->chosenPiece = gameState->cutPiece1;
						gameState->pieceWasChosen = true;
						selectedAnEntity = true;
					}
					else if (entity->id == gameState->cutPiece2->id) {
						DeleteEntity(gameState, gameState->cutPiece1);
						gameState->cutPiece2->flags &= !BEING_CHOSEN_FLAG;
						gameState->cutPiece2->flags |= ADJUSTING_POSITION_FLAG;
						gameState->cutPiece2->centerPosition.x -= gameState->cutPiece2->temporaryPositionChange.x;
						gameState->cutPiece2->centerPosition.y -= gameState->cutPiece2->temporaryPositionChange.y;
						gameState->chosenPiece = gameState->cutPiece2;
						gameState->pieceWasChosen = true;
						selectedAnEntity = true;
					}
					gameState->cutPiece1 = nullptr;
					gameState->cutPiece2 = nullptr;
				}
			}
		}
	}
	else {
		if (inputInfo.mouseInputInfo.inputType == LEFT_CLICK) {
			DrawLine(inputInfo.mouseInputInfo.inputPositions[0].x, inputInfo.mouseInputInfo.inputPositions[0].y, inputInfo.mouseInputInfo.inputPositions[1].x, inputInfo.mouseInputInfo.inputPositions[1].y, RED);
		}
	}

	if (gameState->pieceWasChosen) {
		if (inputInfo.keyCodes & A_KEY_CODE && gameState->chosenPiece->centerPosition.x >= 100) {
			gameState->chosenPiece->centerPosition.x -= 2;
		}
		if (inputInfo.keyCodes & D_KEY_CODE && gameState->chosenPiece->centerPosition.x <= 700) {
			gameState->chosenPiece->centerPosition.x += 2;
		}
		if (inputInfo.keyCodes & ENTER_KEY_CODE) {
			gameState->chosenPiece->flags &= !ADJUSTING_POSITION_FLAG;
			gameState->chosenPiece->flags |= (PHYSICS_FLAG | GRAVITY_FLAG | GROUND_COLLISION_FLAG);
			gameState->pieceWasChosen = false;
			gameState->readyForNewEntityInitialization = true;
			gameState->chosenPiece = nullptr;
		}
	}

	float deltaTime = GetDeltaTime();
	for (int i = 0; i <= (gameState->lastEntityOnEntities - gameState->entities); i++) {
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
						forceApplicationPoint.y += collInfo.minOverlap / 2;
					}
					
					for(int k = 0; k < gameState->SOLVER_ITERATIONS; k++){
						Vector2 impulse = { 0, 0 }, relativeVelocityOfForceApplicationPoint = {0, 0};
						CalculateAndApplyImpulse(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
						HandleFriction(gameState, entity, relevantEntity, collInfo, impulse, relativeVelocityOfForceApplicationPoint, forceApplicationPoint, solverIterationTimeStep);
					}
				}
			}
			if (!(entity->flags & NON_MOVING_FLAG)){
				MoveAndRotateEntity(entity, deltaTime);
				CalibrateEntityWithGrid(gameState, entity);

				if(magnitude(entity->physicsVelocity) < INSIGNIFICANT_NORMAL_VELOCITY_THRESHOLD && 
					entity->rotationalVelocity < INSIGNIFICANT_ANGULAR_VELOCITY_THRESHOLD) 
				{
					std::cout << "entity " << entity->id << "started stopping" << std::endl;
					if (entity->framesWithConsequentialInsignificantMovement == 0) {
						entity->positionWhenMovementBecameInsignificant.x = entity->centerPosition.x;
						entity->positionWhenMovementBecameInsignificant.y = entity->centerPosition.y;
						entity->angleWhenMovementBecameInsignificant = entity->angle;
					}
					else if (entity->framesWithConsequentialInsignificantMovement >= 10 
						&& distance(entity->centerPosition, entity->positionWhenMovementBecameInsignificant) < INSIGNIFICANT_DISPLACEMENT_THRESHOLD 
						&& abs(entity->angle - entity->angleWhenMovementBecameInsignificant) < INSIGNIFICANT_ANGULAR_DISPLACEMENT_THRESHOLD
						) 
					{
						entity->flags |= NON_MOVING_FLAG;
					}
					entity->framesWithConsequentialInsignificantMovement++;
				}
				else {
					if (entity->framesWithConsequentialInsignificantMovement > 0) {
						if (magnitude(entity->physicsVelocity) > INSIGNIFICANT_NORMAL_VELOCITY_THRESHOLD) {
							std::cout << "stopped because of normal speed" << std::endl;
						}
						if (entity->rotationalVelocity > INSIGNIFICANT_ANGULAR_VELOCITY_THRESHOLD) {
							std::cout << "stopped because of angular speed" << std::endl;
						}
					}
					entity->framesWithConsequentialInsignificantMovement = 0;
				}
			}
			entity->netForce = { 0, 0 };
			entity->forceAppliedToAccelerationAndVelocity = { 0, 0 };
			entity->forcesMultipliedByAppliedTime = { 0, 0 };
			entity->torque = 0;
		}
	}

	for (int i = 0; i < gameState->addedEntities; i++) {
		(gameState->entities + i)->gravityApplied = false;
	}

	RetractTemporarySize(gameState, relevantEntitiesSize);

}

void ChangeScreenTo(GameState* gameState, SCREEN_CODES screenCode) {
	gameState->currentScreenCode = screenCode;
}
void InitializeMainMenu(GameState* gameState) {
	VertexData* buttonData = (VertexData*)PushSize(gameState, sizeof(VertexData) * 4);
	VertexData* buttonDataEnd = buttonData;
	*(buttonDataEnd++) = VertexData{ 60, 50 };
	*(buttonDataEnd++) = VertexData{ 60, -50 };
	*(buttonDataEnd++) = VertexData{ -60, -50 };
	*(buttonDataEnd++) = VertexData{ -60, 50 };
	Entity* playButton = InitializeAndPushEntity(gameState, buttonData, buttonDataEnd, 0, BUTTON_FLAG, { (float)gameState->WINDOW_HEIGHT / 2, (float)gameState->WINDOW_WIDTH / 2 }, MAIN_SCREEN);
	playButton->buttonFunction = ChangeScreenTo;
}

void UpdateMainMenu(GameState* gameState, InputInfo inputInfo) {
	uint32_t relevantEntitiesSize = sizeof(Entity*) * 500;
	Entity** relevantEntities = (Entity**)PushTemporarySize(gameState, relevantEntitiesSize);
	if (inputInfo.mouseInputInfo.mouseReleasedThisFrame) {
		//click
		if (inputInfo.mouseInputInfo.inputDurationFrames < gameState->ClickThresholdFrames) {
			Vector2 mousePos = inputInfo.mouseInputInfo.inputPositions[1];
			int numOfCloseEntities = CalculateRelevantEntitiesForPosition(gameState, mousePos, relevantEntities);
			bool selectedAnEntity = false;
			for (int i = 0; i < numOfCloseEntities; i++) {
				Entity* entity = relevantEntities[i];
				bool pointIsInsideEntity = CheckIfAPointIsInsideAnEntity(mousePos, entity);
				if (pointIsInsideEntity) {
					std::cout << "Entity " << entity->id << " clicked" << std::endl;
				}
				if (pointIsInsideEntity && (entity->flags & BUTTON_FLAG)) {
					entity->buttonFunction(gameState, GAMEPLAY_SCREEN);
				}
			}
		}
	}
	else {
		if (inputInfo.mouseInputInfo.inputType == LEFT_CLICK) {
			DrawLine(inputInfo.mouseInputInfo.inputPositions[0].x, inputInfo.mouseInputInfo.inputPositions[0].y, inputInfo.mouseInputInfo.inputPositions[1].x, inputInfo.mouseInputInfo.inputPositions[1].y, RED);
		}
	}
	RetractTemporarySize(gameState, relevantEntitiesSize);
}
