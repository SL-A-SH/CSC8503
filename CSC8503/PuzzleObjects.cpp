#include "PuzzleObjects.h"
#include "PhysicsObject.h"
#include "RenderObject.h"

using namespace NCL;
using namespace CSC8503;

// RoomTrigger Implementation
RoomTrigger::RoomTrigger(GameObject* room, DoorObject* door, const Vector3& doorPos, const Vector3& triggerSize) : GameObject("RoomTrigger") {
    this->room = room;
    this->door = door;
    this->doorPosition = doorPos;
    playerInRange = false;
}

void RoomTrigger::Update(GameObject* player)
{
    if (!door->IsOpen()) {
        // If door is closed, ensure room is solid
        if (!room->GetPhysicsObject()->IsEnabled()) {
            room->GetPhysicsObject()->SetEnabled(true);
            room->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
        }
        return;
    }

    // Calculate distance to door
    Vector3 playerPos = player->GetTransform().GetPosition();
    float distToDoor = Vector::Length(playerPos - doorPosition);

    bool shouldBeInRange = distToDoor < doorProximityThreshold;

    // State change: entering range
    if (shouldBeInRange && !playerInRange) {
        room->GetPhysicsObject()->SetEnabled(false);
        door->GetPhysicsObject()->SetEnabled(false);
        room->GetRenderObject()->SetColour(Vector4(1, 1, 1, 0.3f));
        playerInRange = true;
    }
    // State change: leaving range
    else if (!shouldBeInRange && playerInRange) {
        room->GetPhysicsObject()->SetEnabled(true);
        door->GetPhysicsObject()->SetEnabled(true);
        room->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
        playerInRange = false;
    }
}

// DoorObject Implementation
DoorObject::DoorObject() : GameObject("Door") {
    isOpen = false;
    openTexture = nullptr;
    closeTexture = nullptr;
}

void DoorObject::Update(float dt) {
    if (isOpen && GetRenderObject()->GetDefaultTexture() != openTexture) {
        GetRenderObject()->SetDefaultTexture(openTexture);
    }

    else if (!isOpen && GetRenderObject()->GetDefaultTexture() != closeTexture) {
        GetRenderObject()->SetDefaultTexture(closeTexture);
    }
}

void DoorObject::OpenDoor()
{
    isOpen = true;
}

void DoorObject::CloseDoor()
{
    isOpen = false;
}

void DoorObject::SetTextures(Texture* closedTex, Texture* openTex)
{
    closeTexture = closedTex;
    openTexture = openTex;
}

// ButtonTrigger Implementation
ButtonTrigger::ButtonTrigger(const std::string& name) : GameObject("ButtonTrigger") {
    isPressed = false;
    linkedDoor = nullptr;
}

void ButtonTrigger::OnCollisionBegin(GameObject* otherObject) {
    if (!isPressed && otherObject->GetName() == "PuzzleCube") {
        isPressed = true;
        GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
        if (onButtonPressed) {
            onButtonPressed(linkedDoor);
            linkedDoor = nullptr;
        }
    }
}

void ButtonTrigger::SetLinkedDoor(DoorObject* door) {
    linkedDoor = door;
}

void ButtonTrigger::ForceRelease() {
    if (isPressed) {
        isPressed = false;
        GetRenderObject()->SetColour(Vector4(1, 0, 0, 1));
    }
}