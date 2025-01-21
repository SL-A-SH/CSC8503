#pragma once

#include "PushdownState.h"
#include "GameWorld.h"
#include "Debug.h"
#include "GameBase.h"
#include "GameLevel.h"
#include "NetworkBase.h"
#include "GameServer.h"
#include "GameClient.h"

namespace NCL {
    namespace CSC8503 {
        class GameState : public PushdownState {
        public:
            GameState() {}
            virtual ~GameState() {}
        };

        class MenuState;
        class GamePlayState;
        class PauseState;
        class GameOverState;

        class MenuState : public GameState {
        public:
            MenuState() {}
            PushdownResult OnUpdate(float dt, PushdownState** newState) override;
        };

        class GamePlayState : public GameState, public PacketReceiver {
        public:
            GamePlayState(bool multiplayer, bool asServer, const std::string& ip);
            ~GamePlayState();
            PushdownResult OnUpdate(float dt, PushdownState** newState) override;

            void UpdateGame(float dt);
            void ReceivePacket(int type, GamePacket* packet, int source) override;
            void DisplayScores();

        protected:
            bool isMultiplayer;
            bool isServer;
            GameServer* server;
            GameClient* client;
            int playerID;
            std::map<int, int> playerScores;

            GameLevel* gameLevel;
            float remainingTime;
            const float maxGameTime = 300.0f; // 5 minutes

        private:
            GameWorld* world;
            PhysicsSystem* physics;
        };

        class PauseState : public GameState {
        public:
            PauseState() {}
            PushdownResult OnUpdate(float dt, PushdownState** newState) override;
        };

        class GameOverState : public GameState {
        public:
            GameOverState(int finalScore, bool multiplayer = false, bool server = false, const std::string& ip = "")
                : score(finalScore), wasMultiplayer(multiplayer), isServer(server), serverIP(ip) {}
            PushdownResult OnUpdate(float dt, PushdownState** newState) override;

        protected:
            int score;
            bool wasMultiplayer;
            bool isServer;
            std::string serverIP;
        };
    }
}