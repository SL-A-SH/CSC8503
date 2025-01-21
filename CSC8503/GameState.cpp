#include "GameState.h"

using namespace NCL;
using namespace CSC8503;

GamePlayState::GamePlayState(bool multiplayer, bool asServer, const std::string& ip) {
    isMultiplayer = multiplayer;
    isServer = asServer;
    server = nullptr;
    client = nullptr;
    playerID = 0;

    static bool networkInitialized = false;
    if (!networkInitialized) {
        std::cout << "Initializing network system...\n";
        NetworkBase::Initialise();
        networkInitialized = true;
        std::cout << "Network system initialized\n";
    }

    if (isMultiplayer) {
        if (isServer) {
            try {
                std::cout << "Creating server...\n";
                server = new GameServer(NetworkBase::GetDefaultPort(), 8);
                server->RegisterPacketHandler(Score_Update, this);
                server->RegisterPacketHandler(Request_Scores, this);
                playerID = 1;  // Server is always player 1
                std::cout << "Server started successfully on port " << NetworkBase::GetDefaultPort() << "\n";
            }
            catch (const std::exception& e) {
                std::cout << "Server creation failed: " << e.what() << "\n";
                delete server;
                server = nullptr;
                isMultiplayer = false;
                isServer = false;
                std::cout << "Falling back to single player mode\n";
            }
        }
        else {
            try {
                std::cout << "Creating client...\n";
                client = new GameClient();
                if (!client->netHandle) {
                    throw std::runtime_error("Failed to initialize client network handle");
                }

                client->RegisterPacketHandler(All_Scores, this);

                // Add delay
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                std::cout << "Attempting to connect to server at " << ip << ":" << NetworkBase::GetDefaultPort() << "\n";

                bool connected = false;

                // Try to connect multiple times
                for (int attempts = 0; attempts < 3; attempts++) {
                    if (client->Connect(127, 0, 0, 1, NetworkBase::GetDefaultPort())) {
                        connected = true;
                        std::cout << "Connected to server successfully!\n";
                        break;
                    }
                    std::cout << "Connection attempt " << (attempts + 1) << " failed, retrying...\n";
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                if (!connected) {
                    throw std::runtime_error("Failed to connect to server after multiple attempts");
                }

                playerID = 0;
                client->RegisterPacketHandler(Player_ID_Assignment, this);
            }
            catch (const std::exception& e) {
                std::cout << "Client connection failed: " << e.what() << "\n";
                delete client;
                client = nullptr;
                isMultiplayer = false;
                std::cout << "Falling back to single player mode\n";
            }
        }
    }

    try {
        world = GameBase::GetWorld();
        physics = new PhysicsSystem(*world);
        gameLevel = new GameLevel(world, GameBase::GetRenderer(), physics, isMultiplayer, isServer);
        if (isMultiplayer) {
            if (isServer) {
                gameLevel->SetAsServer(server);
            }
            if (client) {
                client->RegisterPacketHandler(Goose_Position, gameLevel);
            }
            gameLevel->InitializeNetworkObjects();
        }
        remainingTime = maxGameTime;
        std::cout << "Game level initialized successfully\n";
    }
    catch (const std::exception& e) {
        std::cout << "Game initialization failed: " << e.what() << "\n";
        throw;
    }
}

GamePlayState::~GamePlayState() {
    if (server) {
        std::cout << "Shutting down server...\n";
        server->Shutdown();
        delete server;
    }
    if (client) {
        std::cout << "Disconnecting client...\n";
        delete client;
    }
    delete gameLevel;
    delete physics;
}

PushdownState::PushdownResult MenuState::OnUpdate(float dt, PushdownState** newState) {
    Debug::Print("THE MEOW RUNNER", Vector2(25, 30), Debug::WHITE);
    Debug::Print("1. Start Single Player", Vector2(25, 40), Debug::WHITE);
    Debug::Print("2. Create Server", Vector2(25, 50), Debug::WHITE);
    Debug::Print("3. Join Server", Vector2(25, 60), Debug::WHITE);
    Debug::Print("ESC to Quit", Vector2(25, 80), Debug::WHITE);

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1)) {
        std::cout << "Starting single player game...\n";
        *newState = new GamePlayState(false, false, "");
        return PushdownResult::Push;
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2)) {
        std::cout << "Starting server...\n";
        *newState = new GamePlayState(true, true, "");
        return PushdownResult::Push;
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM3)) {
        std::cout << "Attempting to connect to server...\n";
        *newState = new GamePlayState(true, false, "127.0.0.1");
        return PushdownResult::Push;
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
        return PushdownResult::Pop;
    }
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
        return PushdownResult::Pop;
    }
    return PushdownResult::NoChange;
}

PushdownState::PushdownResult GamePlayState::OnUpdate(float dt, PushdownState** newState) {
    if (Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
        *newState = new PauseState();
        return PushdownResult::Push;
    }

    remainingTime -= dt;

    Debug::Print("Time: " + std::to_string(static_cast<int>(round(remainingTime))), Vector2(70, 5));

    UpdateGame(dt);

    if (remainingTime <= 0 || gameLevel->IsGameComplete()) {
        *newState = new GameOverState(gameLevel->GetScore(), isMultiplayer, isServer, isServer ? "" : "serverIP");
        return PushdownResult::Push;
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::P)) {
        *newState = new PauseState();
        return PushdownResult::Push;
    }

    return PushdownResult::NoChange;
}

void GamePlayState::UpdateGame(float dt)
{
    if (isMultiplayer) {
        std::string idText = "Player ID: " + std::to_string(playerID);
        Debug::Print(idText, Vector2(35, 5), Debug::YELLOW);

        if (isServer && server) {
            Debug::Print("Server Mode", Vector2(5, 25), Debug::GREEN);
            server->UpdateServer();
        }
        else if (client) {
            Debug::Print("Client Mode", Vector2(5, 25), Debug::GREEN);
            client->UpdateClient();

            if (Window::GetKeyboard()->KeyPressed(KeyCodes::TAB)) {
                GamePacket request(Request_Scores);
                client->SendPacket(request);
            }
        }
        else {
            Debug::Print("WARNING: Connection Lost!", Vector2(5, 25), Debug::RED);
        }

        int currentScore = gameLevel->GetScore();
        if (currentScore != playerScores[playerID]) {
            std::cout << "Score changed! Sending update...\n";
            ScorePacket scorePacket(playerID, currentScore);
            if (isServer) {
                std::cout << "Server broadcasting score update\n";
                server->SendGlobalPacket(scorePacket);
            }
            else {
                std::cout << "Client sending score update to server\n";
                client->SendPacket(scorePacket);
            }
            playerScores[playerID] = currentScore;
        }

        if (Window::GetKeyboard()->KeyDown(KeyCodes::TAB)) {
            DisplayScores();
        }
    }

    gameLevel->UpdateGame(dt);
}

void GamePlayState::ReceivePacket(int type, GamePacket* packet, int source)
{
    if (type == Player_ID_Assignment) {
        PlayerIDPacket* idPacket = (PlayerIDPacket*)packet;
        playerID = idPacket->playerID;
        std::cout << "Received player ID from server: " << playerID << "\n";
    }
    else if (type == Score_Update) {
        ScorePacket* scorePacket = (ScorePacket*)packet;
        playerScores[scorePacket->playerID] = scorePacket->score;

        if (isServer) {
            // Broadcast updated scores to all clients
            AllScoresPacket allScores;
            for (const auto& pair : playerScores) {
                allScores.AddScore(pair.first, pair.second);
            }
            server->SendGlobalPacket(allScores);
        }
    }
    else if (type == Request_Scores && isServer) {
        // Client has requested scores, send them
        AllScoresPacket allScores;
        for (const auto& pair : playerScores) {
            allScores.AddScore(pair.first, pair.second);
        }
        server->SendGlobalPacket(allScores);
    }
    else if (type == All_Scores && !isServer) {
        // Client received scores update
        AllScoresPacket* scoresPacket = (AllScoresPacket*)packet;
        playerScores.clear();
        for (int i = 0; i < scoresPacket->numPlayers; i++) {
            playerScores[scoresPacket->scores[i].playerID] = scoresPacket->scores[i].score;
        }
    }
}

void GamePlayState::DisplayScores()
{
    int yPos = 50;
    Debug::Print("MULTIPLAYER SCORES", Vector2(5, yPos), Debug::YELLOW);
    yPos += 10;

    // Show your own score first
    std::string yourScore = "Your Score (Player " + std::to_string(playerID) + "): " + std::to_string(playerScores[playerID]);
    Debug::Print(yourScore, Vector2(5, yPos), Debug::WHITE);
    yPos += 10;

    // Then show other players' scores
    Debug::Print("Other Players:", Vector2(5, yPos), Debug::WHITE);
    yPos += 10;

    bool otherPlayersFound = false;
    for (const auto& pair : playerScores) {
        if (pair.first != playerID) {
            std::string scoreText = "Player " + std::to_string(pair.first) + ": " + std::to_string(pair.second);
            Debug::Print(scoreText, Vector2(5, yPos), Debug::WHITE);
            yPos += 10;
            otherPlayersFound = true;
        }
    }

    if (!otherPlayersFound) {
        Debug::Print("No other players connected", Vector2(5, yPos), Debug::RED);
    }
}

PushdownState::PushdownResult PauseState::OnUpdate(float dt, PushdownState** newState) {
    Debug::Print("GAME PAUSED", Vector2(35, 30), Debug::RED);
    Debug::Print("Press P to Resume", Vector2(35, 40), Debug::WHITE);
    Debug::Print("Press M for Menu", Vector2(35, 50), Debug::WHITE);

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::P)) {
        std::cout << "P pressed - Resuming\n";
        return PushdownResult::Pop;
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::M)) {
        std::cout << "M pressed - To Menu\n";
        return PushdownResult::Pop;
    }

    return PushdownResult::NoChange;
}

PushdownState::PushdownResult GameOverState::OnUpdate(float dt, PushdownState** newState) {
    Debug::Print("GAME OVER!", Vector2(35, 30), Debug::RED);
    Debug::Print("Final Score: " + std::to_string(score), Vector2(35, 40), Debug::WHITE);
    Debug::Print("Press R to Restart", Vector2(35, 50), Debug::WHITE);
    Debug::Print("Press M for Menu", Vector2(35, 60), Debug::WHITE);

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::R)) {
        if (wasMultiplayer) {
            *newState = new GamePlayState(true, isServer, serverIP);
        }
        else {
            *newState = new GamePlayState(false, false, "");
        }
        return PushdownResult::Push;
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::M)) {
        std::cout << "M pressed - To Menu\n";
        *newState = new MenuState();
        return PushdownResult::Push;
    }

    return PushdownResult::NoChange;
}