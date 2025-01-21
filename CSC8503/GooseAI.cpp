#include "GooseAI.h"
#include "GameLevel.h"
#include "PhysicsObject.h"
#include "GameServer.h"

using namespace NCL;
using namespace CSC8503;

GooseAI::GooseAI(NavigationGrid* grid, GameObject* player, GameLevel* level, GameServer* serverRef)
    : EnemyAI(grid, player, level), server(serverRef), isChasing(false) {
    detectionRange = 20.0f;
    moveSpeed = 15.0f;
    minX = 5.0f;
    maxX = 30.0f;
    minZ = -40.0f;
    maxZ = -5.0f;
    startPosition = Vector3(30, 2, 0.5);
    InitBehaviourTree();
}

GooseAI::~GooseAI()
{
    delete sequence;
}

void GooseAI::Update(float dt)
{
    if (server) {
        EnemyAI::Update(dt);

        Vector3 pos = GetTransform().GetPosition();
        GoosePacket packet(pos.x, pos.y, pos.z, isChasing);
        server->SendGlobalPacket(packet);
    }
}

void GooseAI::InitBehaviourTree() {
    // Simpler behavior tree - just chase when sees player
    BehaviourAction* chase = new BehaviourAction("Chase",
        [&](float dt, BehaviourState state)->BehaviourState {
            if (server) {
                if (CanSeePlayer()) {
                    Vector3 playerPos = player->GetTransform().GetPosition();
                    targetPosition = playerPos;
                    isChasing = true;
                    return BehaviourState::Success;
                }
                targetPosition = startPosition;
                isChasing = false;
            }
            
            return BehaviourState::Success;
        });

    BehaviourAction* move = new BehaviourAction("Move",
        [&](float dt, BehaviourState state)->BehaviourState {
            Vector3 currentPos = GetTransform().GetPosition();
            float distToTarget = Vector::Length(currentPos - targetPosition);

            if (distToTarget < 1.0f) {
                GetPhysicsObject()->SetLinearVelocity(Vector3(0, 0, 0));
                return BehaviourState::Success;
            }

            Vector3 direction = targetPosition - currentPos;
            direction.y = 0;
            Vector::Normalise(direction);

            GetPhysicsObject()->AddForce(direction * moveSpeed);
            return BehaviourState::Ongoing;
        });

    // Simple sequence: if can chase then move
    sequence = new BehaviourSequence("Root");
    sequence->AddChild(chase);
    sequence->AddChild(move);
    rootSequence = sequence;
}

bool GooseAI::CanSeePlayer()
{
    Vector3 playerPos = player->GetTransform().GetPosition();
    Vector3 currentPos = GetTransform().GetPosition();

    // First check if player is in goose's territory
    if (playerPos.x < minX || playerPos.x > maxX ||
        playerPos.z < minZ || playerPos.z > maxZ) {
        return false;
    }

    // Then do normal distance check
    float distToPlayer = Vector::Length(playerPos - currentPos);
    return distToPlayer < detectionRange;
}
