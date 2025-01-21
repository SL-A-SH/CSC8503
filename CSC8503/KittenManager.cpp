#include "KittenManager.h"
#include "Debug.h"

using namespace NCL;
using namespace CSC8503;

void KittenManager::RegisterKitten(GameObject* kitten) {
    allKittens.push_back(kitten);
}

void KittenManager::UpdateKittens(float dt) {
    // Check for new kittens to collect
    for (auto* kitten : allKittens) {
        KittenObject* kittenObj = dynamic_cast<KittenObject*>(kitten);
        if (kittenObj && !kittenObj->IsFollowing()) {
            if (IsNearPlayer(kittenObj->GetTransform().GetPosition())) {
                kittenObj->SetFollowing(true);
                foundKittens.push_back(kittenObj);
            }
        }
    }

    UpdateFollowBehavior(dt);
}

bool KittenManager::IsNearPlayer(const Vector3& kittenPos) const {
    Vector3 playerPos = player->GetTransform().GetPosition();
    return Vector::Length((kittenPos - playerPos)) < detectionRadius;
}

void KittenManager::UpdateFollowBehavior(float dt) {
    for (size_t i = 0; i < foundKittens.size(); ++i) {
        GameObject* kitten = foundKittens[i];
        Vector3 targetPos = CalculateFollowPosition(kitten, i);

        Vector3 currentPos = kitten->GetTransform().GetPosition();
        Vector3 moveDir = targetPos - currentPos;

        if (Vector::Length(moveDir) > 0.1f) {
            Vector::Normalise(moveDir);
            kitten->GetPhysicsObject()->AddForce(moveDir * followSpeed);

            // Calculate rotation to face movement direction
            float targetRotation = atan2(moveDir.x, moveDir.z);
            Quaternion targetQuat = Quaternion::AxisAngleToQuaterion(Vector3(0, 1, 0), RadiansToDegrees(targetRotation));
            Quaternion currentRotation = kitten->GetTransform().GetOrientation();

            // Smoothly interpolate rotation
            float rotateSpeed = 5.0f;
            Quaternion newRotation = Quaternion::Lerp(currentRotation, targetQuat, dt * rotateSpeed);
            kitten->GetTransform().SetOrientation(newRotation);
        }

        // Add drag to prevent overshooting
        kitten->GetPhysicsObject()->AddForce(
            -kitten->GetPhysicsObject()->GetLinearVelocity() * 2.0f);
    }
}

Vector3 KittenManager::CalculateFollowPosition(GameObject* kitten, int index) const {
    Vector3 playerPos = player->GetTransform().GetPosition();

    // Calculate position in a line behind the player
    float angle = index * (3.14f / 8.0f);  // Spread kittens in an arc
    Vector3 offset = Vector3(sin(angle) * followDistance, 0, cos(angle) * followDistance);

    return playerPos + offset;
}
