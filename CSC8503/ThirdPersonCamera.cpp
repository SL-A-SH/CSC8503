#include "ThirdPersonCamera.h"
#include "PlayerController.h"

using namespace NCL;
using namespace CSC8503;

ThirdPersonCamera::ThirdPersonCamera(GameObject* target, PerspectiveCamera* camera, GameWorld* world, PlayerController* controller) {
    this->target = target;
    this->camera = camera;
    this->gameWorld = world;
    this->playerController = controller;
    initialOffset = Vector3(0, 10, 10);
    currentOffset = initialOffset;
    pitch = -20.0f;

    Initialize();
}

void NCL::CSC8503::ThirdPersonCamera::Initialize()
{
    if (!target || !camera) return;

    Vector3 targetPos = target->GetTransform().GetPosition();

    float pitchRad = Maths::DegreesToRadians(pitch);
    float yawRad = Maths::DegreesToRadians(yaw);

    float horizontalDistance = currentOffset.z * cos(pitchRad);
    float verticalDistance = currentOffset.z * sin(pitchRad);

    float offsetX = horizontalDistance * sin(yawRad);
    float offsetZ = horizontalDistance * cos(yawRad);

    Vector3 initialPos = targetPos + Vector3(offsetX, verticalDistance + currentOffset.y, offsetZ);

    camera->SetPosition(initialPos);
    camera->SetPitch(pitch);
    camera->SetYaw(yaw);

    lastTargetPos = targetPos;
    lastCameraPos = initialPos;

    HandleCameraCollision();
}

void ThirdPersonCamera::Update(float dt) {
    if (!target || !camera) return;

    HandleMouseInput(dt);
    UpdateCameraPosition(dt);
    /*HandleCameraCollision();*/
}

void ThirdPersonCamera::HandleMouseInput(float dt) {
    Vector2 delta = Window::GetMouse()->GetRelativePosition();
    delta.x = -delta.x;

    yaw += delta.x * mouseSensitivity * dt;
    pitch -= delta.y * mouseSensitivity * dt;

    pitch = std::max(-maxPitchAngle, std::min(maxPitchAngle, pitch));

    while (yaw < 0) {
        yaw += 360.0f;
    }
    while (yaw > 360.0f) {
        yaw -= 360.0f;
    }

    if (playerController) {
        float yawQuadrant = fmod(yaw, 360.0f);
        playerController->SetMovementMapping(yawQuadrant);
    }
}

void ThirdPersonCamera::UpdateCameraPosition(float dt) {
    Vector3 targetPos = target->GetTransform().GetPosition();

    // Calculate rotation based on yaw and pitch
    float pitchRad = Maths::DegreesToRadians(pitch);
    float yawRad = Maths::DegreesToRadians(yaw);

    // Calculate desired camera position using spherical coordinates
    float horizontalDistance = currentOffset.z * cos(pitchRad);
    float verticalDistance = currentOffset.z * sin(pitchRad);

    float offsetX = horizontalDistance * sin(yawRad);
    float offsetZ = horizontalDistance * cos(yawRad);

    Vector3 desiredPos = targetPos + Vector3(offsetX, verticalDistance + currentOffset.y, offsetZ);

    // Smoothly interpolate current camera position to desired position
    Vector3 currentPos = camera->GetPosition();
    Vector3 newPos = LerpVector3(currentPos, desiredPos, dt * positionSmoothSpeed);

    // Update camera position and orientation
    camera->SetPosition(newPos);
    camera->SetPitch(pitch);
    camera->SetYaw(yaw);

    // Make camera look at a point slightly above the target for better framing
    Vector3 lookAtPoint = targetPos + Vector3(0, 1.0f, 0);
    Vector3 lookDir = Vector::Normalise(lookAtPoint - newPos);

    // Store these for collision checks
    lastTargetPos = targetPos;
    lastCameraPos = newPos;
}

void ThirdPersonCamera::HandleCameraCollision() {
    // Cast a ray from target to camera to detect obstacles
    Vector3 rayDir = Vector::Normalise(lastCameraPos - lastTargetPos);
    float rayLength = Vector::Length(lastCameraPos - lastTargetPos);

    Ray ray(lastTargetPos, rayDir);
    RayCollision collision;

    // Check for collisions between target and camera
    if (gameWorld->Raycast(ray, collision, true, target)) {
        if (collision.rayDistance < rayLength) {
            // If there's an obstacle, move camera to collision point
            Vector3 newPos = lastTargetPos + (rayDir * (collision.rayDistance - 0.5f));
            camera->SetPosition(newPos);

            // Update current offset for smooth transitions when obstacle is gone
            Vector3 newOffset = newPos - lastTargetPos;
            currentOffset = Vector3(0, newOffset.y, Vector::Length(Vector3(newOffset.x, 0, newOffset.z)));
        }
    }
    else {
        // Gradually return to initial offset when no obstacles
        currentOffset = LerpVector3(currentOffset, initialOffset, 0.1f);
    }
}

Vector3 ThirdPersonCamera::LerpVector3(const Vector3& start, const Vector3& end, float t) {
    t = std::min(1.0f, std::max(0.0f, t));
    return start + ((end - start) * t);
}