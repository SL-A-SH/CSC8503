#include "PlayerController.h"

using namespace NCL;
using namespace CSC8503;

PlayerController::PlayerController(GameObject* player, GameWorld* world) {
    this->player = player;
    this->gameWorld = world;
    lastPosition = player->GetTransform().GetPosition();
}

void PlayerController::Update(float dt) {
    if (!player) return;

    HandleMovement(dt);
    ApplyDrag(dt);
    CheckGrounded();

    lastPosition = player->GetTransform().GetPosition();

    /*Debug::Print("Pos: " + std::to_string(lastPosition.x) + std::to_string(lastPosition.z), Vector2(5, 55));*/
}

void PlayerController::SetMovementMapping(float yawAngle)
{
    currentYawAngle = yawAngle;
}

void PlayerController::HandleMovement(float dt) {
    if (!player->GetPhysicsObject()) return;

    timeSinceLastJump += dt;

    Vector3 moveDir = GetMovementDirection();
    PhysicsObject* phys = player->GetPhysicsObject();

    if (Vector::Length(moveDir) > 0) {
        currentVelocity = phys->GetLinearVelocity();
        float currentSpeed = Vector::Length(currentVelocity);

        float speedFactor = 1.0f - (currentSpeed / maxSpeed);
        Vector3 force = moveDir * moveSpeed * speedFactor;

        if (currentSpeed < maxSpeed) {
            phys->AddForce(force);
        }

        if (Vector::Length(moveDir) > 0.1f) {
            float targetRotation = atan2(moveDir.x, moveDir.z);

            float speedRatio = currentSpeed / maxSpeed;
            float adjustedTurnSpeed = turnSpeed * (1.0f - (speedRatio * 0.5f));

            Quaternion targetQuat = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), RadiansToDegrees(targetRotation));
            Quaternion currentRotation = player->GetTransform().GetOrientation();

            Quaternion newRotation = Quaternion::Lerp(currentRotation, targetQuat, dt * adjustedTurnSpeed);
            player->GetTransform().SetOrientation(newRotation);
        }
    }

    if (Window::GetKeyboard()->KeyPressed(KeyCodes::SPACE)) {
        if (isGrounded && timeSinceLastJump >= jumpCooldown) {
            phys->ApplyLinearImpulse(Vector3(0, jumpForce, 0));
            isGrounded = false;
            isJumping = true;
            timeSinceLastJump = 0.0f;
        }
    }
}

Vector3 PlayerController::GetMovementDirection() {
    Vector3 movement(0, 0, 0);

    if (currentYawAngle >= 45 && currentYawAngle < 135) {
        // Camera facing right
        if (Window::GetKeyboard()->KeyDown(KeyCodes::W)) movement.x = -1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::S)) movement.x = 1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::A)) movement.z = 1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::D)) movement.z = -1;
    }
    else if (currentYawAngle >= 135 && currentYawAngle < 225) {
        // Camera facing back
        if (Window::GetKeyboard()->KeyDown(KeyCodes::W)) movement.z = 1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::S)) movement.z = -1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::A)) movement.x = 1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::D)) movement.x = -1;
    }
    else if (currentYawAngle >= 225 && currentYawAngle < 315) {
        // Camera facing left
        if (Window::GetKeyboard()->KeyDown(KeyCodes::W)) movement.x = 1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::S)) movement.x = -1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::A)) movement.z = -1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::D)) movement.z = 1;
    }
    else {
        // Camera facing forward (default 315-45)
        if (Window::GetKeyboard()->KeyDown(KeyCodes::W)) movement.z = -1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::S)) movement.z = 1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::A)) movement.x = -1;
        if (Window::GetKeyboard()->KeyDown(KeyCodes::D)) movement.x = 1;
    }

    return Vector::Length(movement) > 0 ? Vector::Normalise(movement) : movement;
}

void PlayerController::ApplyDrag(float dt) {
    if (!player->GetPhysicsObject()) return;

    Vector3 velocity = player->GetPhysicsObject()->GetLinearVelocity();
    Vector3 dragForce = velocity * -dragFactor;

    player->GetPhysicsObject()->AddForce(dragForce);
}

void PlayerController::CheckGrounded() {
    wasGrounded = isGrounded;

    // Cast a short ray downward to check for ground
    Vector3 rayStart = player->GetTransform().GetPosition();
    Vector3 rayDir = Vector3(0, -1, 0);
    Ray ray(rayStart, rayDir);
    RayCollision collision;

    if (gameWorld->Raycast(ray, collision, true, player)) {
        float groundThreshold = 1.5f;
        isGrounded = collision.rayDistance < groundThreshold;
    }
    else {
        isGrounded = false;
    }

    if (isGrounded && !wasGrounded) {
        isJumping = false;
    }
}