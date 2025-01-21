#include "GameLevel.h"
#include "Assets.h"
#include "Debug.h"
#include <fstream>
#include <string>
#include <vector>
#include "GameServer.h"
#include "GooseAI.h"

using namespace NCL;
using namespace CSC8503;

GameLevel::GameLevel(GameWorld* existingWorld, GameTechRenderer* existingRenderer, PhysicsSystem* existingPhysics, bool multiplayer, bool server) {
    world = existingWorld;
    physics = existingPhysics;
    renderer = existingRenderer;
    isMultiplayer = multiplayer;
    isServer = server;
    gooseObject = nullptr;

    InitialiseAssets();
    InitWorld();
}

GameLevel::~GameLevel() {
    delete kittenManager;
    delete playerController;
    delete player;
    delete thirdPersonCamera;
    delete navGrid;

    for (auto enemy : enemies) {
        delete enemy;
    }
}

bool GameLevel::IsGameComplete() const
{
    if (!exitGate || !player || !hasKey || !kittenManager || !AllKittensCollected()) {
        return false;
    }

    Vector3 playerPos = player->GetTransform().GetPosition();
    Vector3 doorPos = exitGate->GetTransform().GetPosition();
    float distToDoor = Vector::Length(playerPos - doorPos);

    const float exitProximityThreshold = 6.0f;
    return distToDoor < exitProximityThreshold;
}

void GameLevel::InitializeNetworkObjects()
{
    if (isMultiplayer) {
       SpawnGoose();
    }
}

void GameLevel::ReceivePacket(int type, GamePacket* payload, int source)
{
    if (type == Goose_Position) {
        GoosePacket* goosePacket = (GoosePacket*)payload;
        if (!isServer && gooseObject) {
            Vector3 newPos(goosePacket->posX, goosePacket->posY, goosePacket->posZ);
            gooseObject->GetTransform().SetPosition(newPos);
            if (auto goose = dynamic_cast<GooseAI*>(gooseObject)) {
                goose->SetChasing(goosePacket->isChasing == 1);
            }
        }
    }
}

void GameLevel::InitialiseAssets() {
    cubeMesh = renderer->LoadMesh("cube.msh");
    sphereMesh = renderer->LoadMesh("sphere.msh");
    catMesh = renderer->LoadMesh("ORIGAMI_Chat.msh");
    enemyMesh = renderer->LoadMesh("Keeper.msh");
    kittenMesh = renderer->LoadMesh("Kitten.msh");
    coinMesh = renderer->LoadMesh("coin.msh");
    fishMesh = renderer->LoadMesh("FishEmojiRecreation.msh");
    gooseMesh = renderer->LoadMesh("goose.msh");

    basicTex = renderer->LoadTexture("checkerboard.png");
    floorTex = renderer->LoadTexture("dirt.png");
    wallTex = renderer->LoadTexture("hedge_wall.png");
    catTex = renderer->LoadTexture("cat.jpg");
    cubeTex = renderer->LoadTexture("companion-cube.png");
    doorOpenTex = renderer->LoadTexture("doorOpen.png");
    doorCloseTex = renderer->LoadTexture("doorClose.png");
    coinTex = renderer->LoadTexture("coin.png");
    keyTex = renderer->LoadTexture("key.png");

    basicShader = renderer->LoadShader("scene.vert", "scene.frag");
}

void GameLevel::InitWorld() {
    world->ClearAndErase();
    physics->Clear();

    physics->SetGlobalDamping(0.95f);
    physics->UseGravity(true);

    InitializeLevel();

    SpawnPlayer();
    if (!player || !player->GetPhysicsObject()) {
        throw std::runtime_error("Failed to create player!");
    }

    playerController = new PlayerController(player, world);
    thirdPersonCamera = new ThirdPersonCamera(player, &world->GetMainCamera(), world, playerController);
    kittenManager = new KittenManager(player);

    SpawnKittens();
    SpawnEnemies();
    SpawnCoins();
    CreatePowerups();

    StartCatDialogue();
    ValidatePhysicsObjects();

    physics->UseBroadPhase(true);
}

void GameLevel::ValidatePhysicsObjects() {
    std::vector<GameObject*>::const_iterator first;
    std::vector<GameObject*>::const_iterator last;
    world->GetObjectIterators(first, last);

    for (auto i = first; i != last; ++i) {
        GameObject* obj = *i;
        if (obj->GetBoundingVolume() && !obj->GetPhysicsObject()) {
            std::cout << "Warning: Object with collision volume missing physics object: " << obj->GetName() << std::endl;

            obj->SetPhysicsObject(new PhysicsObject(&obj->GetTransform(), obj->GetBoundingVolume()));
            obj->GetPhysicsObject()->SetInverseMass(1.0f);
        }
    }
}

void GameLevel::InitializeLevel() {
    AddFloorToWorld(Vector3(0, -1, 0));

    navGrid = new NavigationGrid("LevelGrid.txt");

    if (!navGrid) {
        throw std::runtime_error("Failed to create navigation grid!");
    }

    LoadMazeFromGrid();
    CreateKeyGateSystem();
    CreateButtonPuzzle();
}

void GameLevel::LoadMazeFromGrid() {
    std::ifstream gridFile;
    gridFile.open(Assets::DATADIR + "LevelGrid.txt");

    if (!gridFile) {
        std::cout << "Failed to load maze grid file!" << std::endl;
        return;
    }

    int nodeSize, width, height;
    gridFile >> nodeSize >> width >> height;

    float wallSize = 10.0f;
    float wallHeight = 8.0f;

    std::string line;
    std::getline(gridFile, line);

    std::vector<std::vector<bool>> wallMap(height, std::vector<bool>(width, false));

    // First pass: Read the maze into a 2D array
    for (int z = 0; z < height; ++z) {
        std::getline(gridFile, line);
        for (int x = 0; x < width && x < line.length(); ++x) {
            wallMap[z][x] = (line[x] == 'x');
        }
    }

    // Second pass: Combine adjacent walls where possible
    for (int z = 0; z < height; ++z) {
        int startX = -1;
        for (int x = 0; x < width; ++x) {
            if (wallMap[z][x]) {
                if (startX == -1) startX = x;
            }
            else if (startX != -1) {
                // Create a wall from startX to x-1
                float worldX = ((startX + (x - startX - 1) / 2.0f) - width / 2.0f) * wallSize;
                float worldZ = (z - height / 2.0f) * wallSize;
                Vector3 position(worldX, wallHeight / 2, worldZ);
                Vector3 dimensions(wallSize * (x - startX) / 2.0f, wallHeight / 2, wallSize / 2);
                AddWallToWorld(position, dimensions);
                startX = -1;
            }
        }
        if (startX != -1) {
            float worldX = ((startX + (width - startX - 1) / 2.0f) - width / 2.0f) * wallSize;
            float worldZ = (z - height / 2.0f) * wallSize;
            Vector3 position(worldX, wallHeight / 2, worldZ);
            Vector3 dimensions(wallSize * (width - startX) / 2.0f, wallHeight / 2, wallSize / 2);
            AddWallToWorld(position, dimensions);
        }
    }
}

void GameLevel::SpawnKittens()
{
    Vector3 kittenPositions[] = {
       Vector3(-20, 5, -40),
       Vector3(30, 5, -40),
       Vector3(-40, 5, 0),
       Vector3(-40, 5, 30),
       Vector3(30, 5, 20)
    };

    for (const auto& pos : kittenPositions) {
        GameObject* kitten = AddKittenToWorld(pos);
        if (kittenManager) {
            kittenManager->RegisterKitten(kitten);
        }
    }
}

void GameLevel::SpawnGoose()
{
    Vector3 gooseSpawn(30, 2, 0.5f);

    GooseAI* goose = new GooseAI(navGrid, player, this, networkServer);
    Vector3 gooseSize(1.0f, 1.0f, 1.0f);
    AABBVolume* volume = new AABBVolume(gooseSize);
    goose->SetBoundingVolume((CollisionVolume*)volume);

    goose->SetRenderObject(new RenderObject(&goose->GetTransform(), gooseMesh, nullptr, basicShader));
    goose->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));

    goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));
    goose->GetPhysicsObject()->SetInverseMass(0.5f);
    goose->GetPhysicsObject()->InitSphereInertia();

    goose->GetTransform().SetScale(gooseSize).SetPosition(gooseSpawn);
    goose->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));

    enemies.push_back(goose);
    world->AddGameObject(goose);

    gooseObject = goose;
}

void GameLevel::SpawnTrapper()
{
    /*Vector3 trapperSpawn(60, 2, 60);

    TrapperAI* trapper = new TrapperAI(navGrid, player, this, kittenManager);
    Vector3 trapperSize(1.0f, 2.0f, 1.0f);
    AABBVolume* volume = new AABBVolume(trapperSize);
    trapper->SetBoundingVolume((CollisionVolume*)volume);

    trapper->SetRenderObject(new RenderObject(&trapper->GetTransform(), enemyMesh, nullptr, basicShader));
    trapper->GetRenderObject()->SetColour(Vector4(0, 0.5f, 0, 1));

    trapper->SetPhysicsObject(new PhysicsObject(&trapper->GetTransform(), trapper->GetBoundingVolume()));
    trapper->GetPhysicsObject()->SetInverseMass(0.5f);
    trapper->GetPhysicsObject()->InitSphereInertia();

    trapper->GetTransform().SetScale(trapperSize).SetPosition(trapperSpawn);

    enemies.push_back(trapper);
    world->AddGameObject(trapper);*/
}

void GameLevel::SpawnPlayer() {
    Vector3 startPos(-40, 5, -40);
    player = AddPlayerToWorld(startPos);
}

GameObject* GameLevel::AddPlayerToWorld(const Vector3& position) {
    float meshSize = 1.0f;
    float inverseMass = 0.5f;

    GameObject* character = new GameObject();
    SphereVolume* volume = new SphereVolume(1.0f);

    character->SetName("Player");
    character->SetBoundingVolume((CollisionVolume*)volume);
    character->GetTransform().SetScale(Vector3(meshSize, meshSize, meshSize)).SetPosition(position);

    Quaternion rotation = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), 180.0f);
    character->GetTransform().SetOrientation(rotation);

    character->SetRenderObject(new RenderObject(&character->GetTransform(), catMesh, catTex, basicShader));
    character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

    character->GetPhysicsObject()->SetInverseMass(inverseMass);
    character->GetPhysicsObject()->InitSphereInertia();

    world->AddGameObject(character);
    return character;
}

GameObject* GameLevel::AddKittenToWorld(const Vector3& position)
{
    float meshSize = 0.6f;

    KittenObject* kitten = new KittenObject();
    SphereVolume* volume = new SphereVolume(0.5f);

    kitten->SetBoundingVolume((CollisionVolume*)volume);
    kitten->GetTransform()
        .SetScale(Vector3(meshSize, meshSize, meshSize))
        .SetPosition(position);

    float randomRotation = rand() % 360;
    kitten->GetTransform().SetOrientation(
        Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), randomRotation)
    );

    kitten->SetRenderObject(new RenderObject(
        &kitten->GetTransform(),
        kittenMesh,
        nullptr,
        basicShader
    ));

    kitten->SetPhysicsObject(new PhysicsObject(
        &kitten->GetTransform(),
        kitten->GetBoundingVolume()
    ));

    kitten->GetPhysicsObject()->SetInverseMass(1.0f);
    kitten->GetPhysicsObject()->InitSphereInertia();

    kitten->GetRenderObject()->SetColour(Vector4(0, 0, 0, 1.0f));

    world->AddGameObject(kitten);
    return kitten;
}

GameObject* GameLevel::AddDoorToWorld(DoorObject* door, const Vector3& position)
{
    Vector3 doorSize(5.0f, 3.0f, 0.5f);
    AABBVolume* volume = new AABBVolume(doorSize);
    door->SetBoundingVolume((CollisionVolume*)volume);

    door->GetTransform().SetScale(doorSize * 2.0f).SetPosition(position);

    door->SetRenderObject(new RenderObject(&door->GetTransform(), cubeMesh, doorCloseTex, basicShader));
    door->SetPhysicsObject(new PhysicsObject(&door->GetTransform(), door->GetBoundingVolume()));

    door->GetPhysicsObject()->SetInverseMass(0);
    door->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(door);
    return door;
}

GameObject* GameLevel::AddButtonToWorld(ButtonTrigger* button, const Vector3& position)
{
    Vector3 buttonSize(2.0f, 0.3f, 2.0f);
    AABBVolume* volume = new AABBVolume(buttonSize);
    button->SetBoundingVolume((CollisionVolume*)volume);

    button->GetTransform().SetScale(buttonSize * 2.0f).SetPosition(position);

    button->SetRenderObject(new RenderObject(&button->GetTransform(), cubeMesh, basicTex, basicShader));
    button->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));

    button->SetPhysicsObject(new PhysicsObject(&button->GetTransform(), button->GetBoundingVolume()));
    button->GetPhysicsObject()->SetInverseMass(0);
    button->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(button);
    return button;
}

GameObject* GameLevel::AddPuzzleCube(const Vector3& position)
{
    GameObject* cube = new GameObject("PuzzleCube");
    Vector3 cubeSize(1.0f, 1.0f, 1.0f);

    AABBVolume* volume = new AABBVolume(cubeSize);
    cube->SetBoundingVolume((CollisionVolume*)volume);

    cube->GetTransform().SetScale(cubeSize * 2.0f).SetPosition(position);

    cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, cubeTex, basicShader));

    cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));
    cube->GetPhysicsObject()->SetInverseMass(0.4f);
    cube->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(cube);
    return cube;
}

GameObject* GameLevel::AddCoinToWorld(const Vector3& position) {
    GameObject* coin = new GameObject("Coin");
    SphereVolume* volume = new SphereVolume(0.2f);
    coin->SetBoundingVolume((CollisionVolume*)volume);

    coin->GetTransform().SetScale(Vector3(0.2f, 0.2f, 0.2f)).SetPosition(position);
    coin->SetRenderObject(new RenderObject(&coin->GetTransform(), coinMesh, coinTex, basicShader));

    PhysicsObject* physics = new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume());
    physics->SetInverseMass(0.0f);
    physics->InitSphereInertia();
    coin->SetPhysicsObject(physics);

    world->AddGameObject(coin);
    return coin;
}

GameObject* GameLevel::AddKeyToWorld(const Vector3& position) {
    GameObject* key = new GameObject("Key");
    SphereVolume* volume = new SphereVolume(0.5f);
    key->SetBoundingVolume((CollisionVolume*)volume);

    key->GetTransform().SetScale(Vector3(0.5f, 0.5f, 0.5f)).SetPosition(position);
    key->SetRenderObject(new RenderObject(&key->GetTransform(), fishMesh, nullptr, basicShader));
    key->GetRenderObject()->SetColour(Vector4(1.0f, 0.8f, 0.2f, 1.0f));

    PhysicsObject* physics = new PhysicsObject(&key->GetTransform(), key->GetBoundingVolume());
    physics->SetInverseMass(0.0f);
    physics->InitSphereInertia();
    key->SetPhysicsObject(physics);

    world->AddGameObject(key);
    return key;
}

GameObject* GameLevel::AddPowerupToWorld(const Vector3& position) {
    GameObject* powerup = new GameObject("Powerup");
    SphereVolume* volume = new SphereVolume(0.5f);
    powerup->SetBoundingVolume((CollisionVolume*)volume);

    powerup->GetTransform().SetScale(Vector3(0.5f, 0.5f, 0.5f)).SetPosition(position);
    powerup->SetRenderObject(new RenderObject(&powerup->GetTransform(), sphereMesh, nullptr, basicShader));
    powerup->GetRenderObject()->SetColour(Vector4(0, 1, 1, 1));

    PhysicsObject* physics = new PhysicsObject(&powerup->GetTransform(), powerup->GetBoundingVolume());
    physics->SetInverseMass(0.0f);
    physics->InitSphereInertia();
    powerup->SetPhysicsObject(physics);

    world->AddGameObject(powerup);
    return powerup;
}

void GameLevel::UpdateGame(float dt) {
    if (playerController) {
        playerController->Update(dt);
    }
    if (thirdPersonCamera) {
        thirdPersonCamera->Update(dt);
    }
    if (kittenManager) {
        kittenManager->UpdateKittens(dt);
    }

    UpdateDialogue(dt);
    UpdatePowerups(dt);
    CheckCollectibles();
    UpdateEnemies(dt);
    HandleObjectInteraction();

    world->UpdateWorld(dt);
    physics->Update(dt);

    std::vector<GameObject*>::const_iterator first;
    std::vector<GameObject*>::const_iterator last;
    world->GetObjectIterators(first, last);

    for (auto i = first; i != last; ++i) {
        if (DoorObject* door = dynamic_cast<DoorObject*>(*i)) {
            door->Update(dt);
        }
    }

    DisplayGameInfo();
}

void GameLevel::UpdateDialogue(float dt) {
    if (dialogueIndex < dialogueMessages.size()) {
        dialogueTimer += dt;
        if (dialogueTimer >= 2.0f) {
            dialogueTimer = 0.0f;
            dialogueIndex++;
        }
        if (dialogueIndex < dialogueMessages.size()) {
            Debug::Print("Shadow: " + dialogueMessages[dialogueIndex], Vector2(5, 80), Vector4(1, 0.8f, 0.8f, 1));
        }
    }
}

void GameLevel::CheckCollectibles() {
    if (!player) return;

    Vector3 playerPos = player->GetTransform().GetPosition();
    float collectRadius = 2.0f;

    std::vector<GameObject*>::const_iterator first;
    std::vector<GameObject*>::const_iterator last;
    world->GetObjectIterators(first, last);

    // Store objects to remove in a separate list
    std::vector<GameObject*> objectsToRemove;

    // First check all collisions
    for (auto it = first; it != last; ++it) {
        GameObject* obj = *it;
        if (!obj->IsActive()) continue;

        Vector3 objPos = obj->GetTransform().GetPosition();
        float dist = Vector::Length(playerPos - objPos);

        if (dist < collectRadius) {
            if (obj->GetName() == "Coin") {
                score += COIN_VALUE;
                objectsToRemove.push_back(obj);
            }
            else if (obj->GetName() == "Key") {
                hasKey = true;
                score += KEY_VALUE;
                objectsToRemove.push_back(obj);

                if (exitGate && dynamic_cast<DoorObject*>(exitGate)) {
                    DoorObject* door = dynamic_cast<DoorObject*>(exitGate);
                    door->OpenDoor();
                }
            }
            else if (obj->GetName() == "Powerup") {
                ActivatePowerup();
                objectsToRemove.push_back(obj);
            }
        }
    }

    for (auto obj : objectsToRemove) {
        world->RemoveGameObject(obj, true);
    }
}

void GameLevel::UpdateEnemies(float dt) {
    for (auto enemy : enemies) {
        enemy->Update(dt);

        // Check if enemy caught player
        Vector3 dirToPlayer = player->GetTransform().GetPosition() - enemy->GetTransform().GetPosition();
        if (Vector::Length(dirToPlayer) < 3.0f && enemy->IsChasing()) {
            enemy->OnPlayerCollision();
            break;
        }
    }
}

void GameLevel::ResetLevel() {
    if (IsPowerupActive())
    {
        return;
    }

    player->GetTransform().SetPosition(Vector3(-40, 5, -40));

    for (auto enemy : enemies) {
        enemy->Reset();
    }

    score = std::max(0, score - 5);
}

void GameLevel::HandleObjectInteraction() {
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::E)) {
        if (!selectedObject) {
            Ray ray(player->GetTransform().GetPosition(), player->GetTransform().GetOrientation() * Vector3(0, 0, 1));
            RayCollision collision;

            if (world->Raycast(ray, collision, true, player)) {
                GameObject* hitObject = (GameObject*)collision.node;
                if (hitObject->GetName() == "PuzzleCube" && collision.rayDistance < interactionDistance) {
                    selectedObject = hitObject;

                    if (puzzleButton) {
                        ((ButtonTrigger*)puzzleButton)->ForceRelease();
                    }

                    selectedObject->GetPhysicsObject()->SetEnabled(false);
                }
            }
        }
        else {
            selectedObject->GetPhysicsObject()->SetEnabled(true);
            selectedObject->GetPhysicsObject()->SetLinearVelocity(Vector3(0, -0.1f, 0));
            selectedObject = nullptr;
        }
    }

    if (selectedObject) {
        Vector3 newPos = player->GetTransform().GetPosition();
        Quaternion playerOrientation = player->GetTransform().GetOrientation();
        Vector3 offsetPos = newPos + (playerOrientation * heldObjectOffset);
        selectedObject->GetTransform().SetPosition(offsetPos);
        selectedObject->GetTransform().SetOrientation(playerOrientation);
    }
}

void GameLevel::SpawnEnemies() {
    Vector3 spawnPoints[] = {
        Vector3(-20, 2, 0)
        //Vector3(20, 5, 20)
    };

    Vector3 patrolPoints[] = {
        Vector3(-40, 2, -20)
        //Vector3(-20, 5, 20)
    };

    for (int i = 0; i < 1; i++) {
        EnemyAI* enemy = new EnemyAI(navGrid, player, this);

        Vector3 enemySize(1.0f, 1.0f, 1.0f);
        AABBVolume* volume = new AABBVolume(enemySize);
        enemy->SetBoundingVolume((CollisionVolume*)volume);

        enemy->SetRenderObject(new RenderObject(&enemy->GetTransform(), enemyMesh, nullptr, basicShader));
        enemy->GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));

        enemy->SetPhysicsObject(new PhysicsObject(&enemy->GetTransform(), enemy->GetBoundingVolume()));
        enemy->GetPhysicsObject()->SetInverseMass(0.5f);
        enemy->GetPhysicsObject()->InitSphereInertia();

        enemy->SetStartPosition(spawnPoints[i]);
        enemy->SetPatrolPoint(patrolPoints[i]);
        enemy->GetTransform().SetScale(enemySize).SetPosition(spawnPoints[i]);

        enemies.push_back(enemy);
        world->AddGameObject(enemy);
    }
}

void GameLevel::SpawnCoins() {
    Vector3 coinPositions[] = {
        Vector3(0, 5, -30),
        Vector3(30, 5, 0),
        Vector3(-30, 5, 0),
        Vector3(0, 5, 30),
        Vector3(15, 5, 10),
        Vector3(-10, 5, -20)
    };

    for (const auto& pos : coinPositions) {
        AddCoinToWorld(pos);
    }
}

void GameLevel::CreateKeyGateSystem() {
    Vector3 gatePos(30, 4, 35);
    DoorObject* door = new DoorObject();
    door->SetTextures(doorCloseTex, doorOpenTex);
    exitGate = AddDoorToWorld(door, gatePos);

    Vector3 keyPos(-40, 5, 20);
    key = AddKeyToWorld(keyPos);
}

void GameLevel::CreateButtonPuzzle() {
    Vector3 buttonPos(-20, 1, 10);
    ButtonTrigger* button = new ButtonTrigger();
    puzzleButton = AddButtonToWorld(button, buttonPos);

    Vector3 cubePos = buttonPos + Vector3(20, 5, 20);
    puzzleCube = AddPuzzleCube(cubePos);

    Vector3 doorPos(-40, 5, 25);
    DoorObject* door = new DoorObject();
    door->SetTextures(doorCloseTex, doorOpenTex);
    DoorObject* blockingDoor = (DoorObject*)AddDoorToWorld(door, doorPos);

    button->SetLinkedDoor(blockingDoor);
    button->SetButtonCallback([this](DoorObject* door) {
        if (door) {
            world->RemoveGameObject(door, true);
        }
    });
}

void GameLevel::CreatePowerups() {
    Vector3 powerupPositions[] = {
        Vector3(0, 5, -10)
    };

    for (const auto& pos : powerupPositions) {
        AddPowerupToWorld(pos);
    }
}

void GameLevel::StartCatDialogue() {
    dialogueMessages = {
        "Meow... Where am I?",
        "My kittens! They're missing!",
        "I need to find them all...",
        "But this maze looks dangerous..."
    };
    dialogueTimer = 0.0f;
    dialogueIndex = 0;
}

void GameLevel::ActivatePowerup() {
    powerupTimer = powerupDuration;
    playerController->SetMovementSpeed(normalSpeed * 3.0f);
    player->GetRenderObject()->SetColour(Vector4(1.0f, 0.8f, 0.2f, 1.0f));
}

void GameLevel::UpdatePowerups(float dt) {
    if (powerupTimer > 0) {
        powerupTimer -= dt;
        if (powerupTimer <= 0) {
            playerController->SetMovementSpeed(normalSpeed);
            player->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
        }
    }
}

GameObject* GameLevel::AddFloorToWorld(const Vector3& position) {
    GameObject* floor = new GameObject("Floor");

    Vector3 floorSize = Vector3(60, 2, 60);
    AABBVolume* volume = new AABBVolume(floorSize);
    floor->SetBoundingVolume((CollisionVolume*)volume);
    floor->GetTransform().SetScale(floorSize * 2.0f).SetPosition(position);

    floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, floorTex, basicShader));
    floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

    floor->GetPhysicsObject()->SetInverseMass(0);
    floor->GetPhysicsObject()->InitCubeInertia();

    world->AddGameObject(floor);

    return floor;
}

GameObject* GameLevel::AddWallToWorld(const Vector3& position, const Vector3& dimensions) {
    GameObject* wall = new GameObject("Wall");

    AABBVolume* volume = new AABBVolume(dimensions);
    wall->SetBoundingVolume((CollisionVolume*)volume);
    wall->GetTransform().SetScale(dimensions * 2.0f).SetPosition(position);

    wall->SetRenderObject(new RenderObject(&wall->GetTransform(), cubeMesh, wallTex, basicShader));

    PhysicsObject* physics = new PhysicsObject(&wall->GetTransform(), wall->GetBoundingVolume());
    physics->SetInverseMass(0);
    physics->InitCubeInertia();

    physics->SetLinearVelocity(Vector3(0, 0, 0));
    physics->SetAngularVelocity(Vector3(0, 0, 0));
    wall->SetPhysicsObject(physics);

    world->AddGameObject(wall);
    return wall;
}

void GameLevel::DisplayGameInfo() {
    Debug::Print("Score: " + std::to_string(score), Vector2(5, 5));
    Debug::Print("Kittens Found: " + std::to_string(kittenManager->GetFoundKittens()) +
        "/" + std::to_string(kittenManager->GetTotalKittens()), Vector2(5, 15));

    if (hasKey) {
        Debug::Print("Key Collected!", Vector2(5, 25), Vector4(1, 1, 0, 1));
    }

    if (powerupTimer > 0) {
        Debug::Print("Powerup Active!", Vector2(5, 35), Vector4(1, 0.8f, 0.2f, 1));
        Debug::Print("Time Left: " + std::to_string((int)powerupTimer), Vector2(5, 45), Vector4(1, 0.8f, 0.2f, 1));
    }
}