#pragma once
#include "GameTechRenderer.h"
#include "Window.h"
#include "PushdownMachine.h"
#include "GameState.h"
#include "GameWorld.h"

namespace NCL {
    namespace CSC8503 {
        class GameBase {
        public:
            GameBase();
            ~GameBase();

            virtual void UpdateGame(float dt);
            void InitialiseGame();

            static GameTechRenderer* GetRenderer() { return renderer; }
            static GameWorld* GetWorld() { return world; }

        protected:
            static GameTechRenderer* renderer;
            static GameWorld* world;
            PushdownMachine* stateMachine;
        };
    }
}