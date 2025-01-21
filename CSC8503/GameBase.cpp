#include "GameBase.h"

using namespace NCL;
using namespace CSC8503;

GameTechRenderer* GameBase::renderer = nullptr;
GameWorld* GameBase::world = nullptr;

GameBase::GameBase() {
    world = new GameWorld();
    renderer = new GameTechRenderer(*world);
    stateMachine = nullptr;
}

GameBase::~GameBase() {
    delete stateMachine;
    delete renderer;
    delete world;
}

void GameBase::InitialiseGame() {
    stateMachine = new PushdownMachine(new MenuState());
}

void GameBase::UpdateGame(float dt) {
    renderer->Update(dt);
    stateMachine->Update(dt);
    renderer->Render();
    Debug::UpdateRenderables(dt);
}