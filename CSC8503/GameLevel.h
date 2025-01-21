#pragma once

#include "GameWorld.h"
#include "PhysicsSystem.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "GameTechRenderer.h"
#include "NetworkBase.h"
#include "PlayerController.h"
#include "ThirdPersonCamera.h"
#include "KittenManager.h"
#include "PuzzleObjects.h"
#include "EnemyAI.h"

namespace NCL {
    namespace CSC8503 {
        class GameServer;
        class GameClient;

        class GameLevel : public PacketReceiver {
        private:
            bool isMultiplayer;
            bool isServer;
            GameServer* networkServer;
            GameObject* gooseObject;

        public:
            GameLevel(GameWorld* existingWorld, GameTechRenderer* existingRenderer, PhysicsSystem* physics, 
                bool multiplayer = false, bool server = false);
            ~GameLevel();

            virtual void UpdateGame(float dt);

            bool AllKittensCollected() const {
                return kittenManager && kittenManager->GetFoundKittens() >= kittenManager->GetTotalKittens();
            }

            int GetScore() const {
                return score;
            }

            bool IsPowerupActive() const { return powerupTimer > 0.0f; }
            void ResetLevel();
            bool IsGameComplete() const;
            
            void SetAsServer(GameServer* server) {
                networkServer = server;
            }

            void InitializeNetworkObjects();
            void ReceivePacket(int type, GamePacket* packet, int source) override;


        protected:
            // Initialization Methods
            void InitialiseAssets();
            void InitWorld();
            void InitializeLevel();
            void LoadMazeFromGrid();
            void SpawnKittens();
            void SpawnGoose();
            void SpawnTrapper();
            void CreateKeyGateSystem();
            void CreateButtonPuzzle();
            void SpawnCoins();
            void CreatePowerups();
            void StartCatDialogue();
            void SpawnPlayer();
            void SpawnEnemies();
            void ActivatePowerup();
            void ValidatePhysicsObjects();

            // Update Methods
            void UpdateDialogue(float dt);
            void CheckCollectibles();
            void UpdatePowerups(float dt);
            void DisplayGameInfo();
            void UpdateEnemies(float dt);
            void HandleObjectInteraction();

            // Object Creation Methods
            GameObject* AddFloorToWorld(const Vector3& position);
            GameObject* AddWallToWorld(const Vector3& position, const Vector3& dimensions);
            GameObject* AddPlayerToWorld(const Vector3& position);
            GameObject* AddKittenToWorld(const Vector3& position);
            GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, bool moveable, Texture* texture);
            GameObject* AddDoorToWorld(DoorObject* door, const Vector3& position);
            GameObject* AddButtonToWorld(ButtonTrigger* button, const Vector3& position);
            GameObject* AddPuzzleCube(const Vector3& position);
            GameObject* AddCoinToWorld(const Vector3& position);
            GameObject* AddKeyToWorld(const Vector3& position);
            GameObject* AddPowerupToWorld(const Vector3& position);

            // Core Systems
            GameWorld* world;
            PhysicsSystem* physics;
            GameTechRenderer* renderer;

            // Game Objects
            GameObject* player;
            GameObject* key;
            GameObject* exitGate;
            GameObject* puzzleButton;
            GameObject* puzzleCube;
            GameObject* selectedObject;

            // Controllers and Managers
            PlayerController* playerController;
            ThirdPersonCamera* thirdPersonCamera;
            KittenManager* kittenManager;
            NavigationGrid* navGrid;
            std::vector<EnemyAI*> enemies;

            // Game State
            int score = 0;
            bool hasKey = false;
            float powerupTimer = 0.0f;
            const float powerupDuration = 60.0f;
            const float normalSpeed = 20.0f;
            float interactionDistance = 5.0f;
            Vector3 heldObjectOffset = Vector3(0, 1, 2);

            // Dialogue System
            std::vector<std::string> dialogueMessages;
            float dialogueTimer = 0.0f;
            int dialogueIndex = 0;

            // Assets
            Mesh* cubeMesh = nullptr;
            Mesh* sphereMesh = nullptr;
            Mesh* catMesh = nullptr;
            Mesh* kittenMesh = nullptr;
            Mesh* enemyMesh = nullptr;
            Mesh* coinMesh = nullptr;
            Mesh* fishMesh = nullptr;
            Mesh* gooseMesh = nullptr;

            Texture* basicTex = nullptr;
            Texture* floorTex = nullptr;
            Texture* wallTex = nullptr;
            Texture* catTex = nullptr;
            Texture* cubeTex = nullptr;
            Texture* doorCloseTex = nullptr;
            Texture* doorOpenTex = nullptr;
            Texture* coinTex = nullptr;
            Texture* keyTex = nullptr;

            Shader* basicShader = nullptr;

            // Constants
            const float WALL_SIZE = 10.0f;
            const float WALL_HEIGHT = 8.0f;
            const float MAZE_SIZE = 100.0f;
            const float COIN_VALUE = 10;
            const float KEY_VALUE = 200;
            const float KITTEN_VALUE = 50;
            const float BUTTON_VALUE = 100;
        };
    }
}