#pragma once
#include "EnemyAI.h"

namespace NCL {
    namespace CSC8503 {
        class GameLevel;
        class GameServer;

        class GooseAI : public EnemyAI {
        private:
            GameServer* server;
            bool isChasing;

        public:
            GooseAI(NavigationGrid* grid, GameObject* player, GameLevel* level, GameServer* serverRef);
            ~GooseAI();

            void Update(float dt) override;
            void SetChasing(bool chase) { isChasing = chase; }

        protected:
            void InitBehaviourTree() override;
            void OnPlayerCollision() override {
                Debug::Print("Untitled Goose: Peace was never an option!", Vector2(5, 80), Vector4(1, 0.8f, 0.8f, 1));
            }
            bool CanSeePlayer() override;

            BehaviourSequence* sequence;

            float minX, maxX, minZ, maxZ;
        };
    }
}