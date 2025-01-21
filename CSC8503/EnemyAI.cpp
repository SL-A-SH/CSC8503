#include "EnemyAI.h"
#include "PhysicsObject.h"
#include "GameLevel.h"

using namespace NCL;
using namespace CSC8503;

EnemyAI::EnemyAI(NavigationGrid* grid, GameObject* player, GameLevel* level) : GameObject("Enemy") {
    if (!grid) {
        throw std::runtime_error("Navigation grid cannot be null!");
    }

    if (!player) {
        throw std::runtime_error("Player object cannot be null!");
    }

    if (!level) {
        throw std::runtime_error("Game level cannot be null!");
    }

    this->navGrid = grid;
    this->player = player;
    this->gameLevel = level;
    this->pathTimer = 0.0f;
    this->isChasing = false;
    this->reachedTarget = false;
    this->patrollingToSpawn = false;
    this->reachedPatrolPoint = false;

    InitBehaviourTree();
}

EnemyAI::~EnemyAI() {
    delete rootSequence;
}

void EnemyAI::OnPlayerCollision()
{
    if (!gameLevel->IsPowerupActive()) {
        gameLevel->ResetLevel();
    }
}

void EnemyAI::InitBehaviourTree() {
    BehaviourAction* move = new BehaviourAction("Move",
        [&](float dt, BehaviourState state) -> BehaviourState {
            Vector3 currentPos = GetTransform().GetPosition();
            float distToTarget = Vector::Length(currentPos - targetPosition);

            if (distToTarget < 1.0f) {
                GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
                return BehaviourState::Success;
            }

            Vector3 moveDirection = targetPosition - currentPos;
            moveDirection.y = 0;
            Vector::Normalise(moveDirection);

            Vector3 currentVel = GetPhysicsObject()->GetLinearVelocity();

            float targetSpeed = moveSpeed;
            if (distToTarget < 5.0f) {
                targetSpeed *= (distToTarget / 5.0f) * (distToTarget / 5.0f);
            }

            Vector3 desiredVelocity = moveDirection * targetSpeed;
            desiredVelocity.y = currentVel.y;
            Vector3 steeringForce = (desiredVelocity - currentVel);
            steeringForce.y = 0;

            if (currentPos.y > 2.0f) {
                GetPhysicsObject()->AddForce(Vector3(0, -20.0f, 0));
            }

            float dampingFactor = 0.8f;
            if (distToTarget < 5.0f) {
                dampingFactor = 1.2f;
            }

            GetPhysicsObject()->AddForce(steeringForce * 5.0f);
            GetPhysicsObject()->AddForce(-currentVel * dampingFactor);

            return BehaviourState::Ongoing;
        });

    BehaviourAction* patrol = new BehaviourAction("Patrol",
        [&](float dt, BehaviourState state) -> BehaviourState {
            if (isChasing) return BehaviourState::Failure;

            Vector3 currentPos = GetTransform().GetPosition();

            float distToTarget = Vector::Length(currentPos - targetPosition);
            if (distToTarget < arrivalRadius) {
                patrollingToSpawn = !patrollingToSpawn;
            }

            targetPosition = patrollingToSpawn ? startPosition : patrolPoint;

            return BehaviourState::Success;
        });

    BehaviourAction* chase = new BehaviourAction("Chase",
        [&](float dt, BehaviourState state) -> BehaviourState {
            if (gameLevel->IsPowerupActive()) {
                return BehaviourState::Failure;
            }

            if (!isChasing) return BehaviourState::Failure;

            Vector3 playerPos = player->GetTransform().GetPosition();

            Vector3 dirToPlayer = playerPos - GetTransform().GetPosition();
            float distToPlayer = Vector::Length(dirToPlayer);
            targetPosition = playerPos;

            return BehaviourState::Success;
        });

    // Build tree
    BehaviourSelector* selector = new BehaviourSelector("Main Selector");

    BehaviourSequence* chaseSeq = new BehaviourSequence("Chase Sequence");
    chaseSeq->AddChild(chase);
    chaseSeq->AddChild(move);

    BehaviourSequence* patrolSeq = new BehaviourSequence("Patrol Sequence");
    patrolSeq->AddChild(patrol);
    patrolSeq->AddChild(move);

    selector->AddChild(chaseSeq);
    selector->AddChild(patrolSeq);

    rootSequence = new BehaviourSequence("Root Sequence");
    rootSequence->AddChild(selector);
}

void EnemyAI::Update(float dt) {
    Vector3 currentPos = GetTransform().GetPosition();

    if (currentPos.y <= 2.5f) {
        if (CanSeePlayer()) {
            isChasing = true;
        }
        else {
            isChasing = false;
        }

        BehaviourState state = rootSequence->Execute(dt);
    }
}

void EnemyAI::UpdatePath() {
    Vector3 startPos = GetTransform().GetPosition();
    Vector3 endPos = targetPosition;

    float gridSize = navGrid->GetNodeSize();
    float gridWidth = navGrid->GetGridWidth() * gridSize;
    float gridHeight = navGrid->GetGridHeight() * gridSize;

    startPos.x = std::clamp(startPos.x, -gridWidth / 2, gridWidth / 2);
    startPos.z = std::clamp(startPos.z, -gridHeight / 2, gridHeight / 2);
    endPos.x = std::clamp(endPos.x, -gridWidth / 2, gridWidth / 2);
    endPos.z = std::clamp(endPos.z, -gridHeight / 2, gridHeight / 2);

    startPos.y = 0;
    endPos.y = 0;
}

bool EnemyAI::CanSeePlayer() {
    Vector3 dirToPlayer = player->GetTransform().GetPosition() - GetTransform().GetPosition();
    float distToPlayer = Vector::Length(dirToPlayer);
    return distToPlayer < detectionRange;
}

void EnemyAI::Reset() {
    GetTransform().SetPosition(startPosition);
    isChasing = false;
    reachedTarget = false;
    pathTimer = 0.0f;
}
