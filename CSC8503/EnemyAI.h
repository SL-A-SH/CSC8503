#pragma once

#include "GameObject.h"
#include "NavigationGrid.h"
#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"
#include "Debug.h"

namespace NCL {
    namespace CSC8503 {
        class GameLevel;

        class EnemyAI : public GameObject {
        public:
            EnemyAI(NavigationGrid* grid, GameObject* player, GameLevel* level);
            ~EnemyAI();

            virtual void Update(float dt);
            void Reset();

            bool IsChasing() const { return isChasing; }
            void SetPatrolPoint(const Vector3& point) { patrolPoint = point; }
            void SetStartPosition(const Vector3& pos) { startPosition = pos; }
            virtual void OnPlayerCollision();

        protected:
            virtual void InitBehaviourTree();
            void UpdatePath();
            virtual bool CanSeePlayer();

            BehaviourSequence* rootSequence;
            NavigationGrid* navGrid;
            GameObject* player;
            GameLevel* gameLevel;

            Vector3 startPosition;
            Vector3 targetPosition;
            Vector3 patrolPoint;

            float detectionRange = 15.0f;
            float moveSpeed = 3.0f;
            const float maxVelocity = 6.0f;
            const float maxForce = 9.0f;
            const float catchDistance = 2.0f;
            const float pathUpdateTime = 1.0f;
            float pathTimer = 0.0;
            const float arrivalRadius = 1.0f;

            bool isChasing;
            bool reachedTarget;
            bool patrollingToSpawn;
            bool reachedPatrolPoint;
        };
    }
}