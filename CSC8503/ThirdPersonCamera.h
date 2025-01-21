#pragma once

#include "GameObject.h" 
#include "Camera.h"
#include "Window.h"
#include "GameWorld.h"
#include "Ray.h"
#include "Maths.h"


namespace NCL {
    namespace CSC8503 {
        class PlayerController;

        class ThirdPersonCamera {
        public:
            ThirdPersonCamera(GameObject* target, PerspectiveCamera* camera, GameWorld* world, PlayerController* controller);
            ~ThirdPersonCamera() {}

            void Initialize();
            void Update(float dt);

            void SetMouseSensitivity(float sens) { mouseSensitivity = sens; }
            void SetSmoothSpeed(float speed) { positionSmoothSpeed = speed; }
            void SetOffset(const Vector3& offset) {
                initialOffset = offset;
                currentOffset = offset;
                Initialize();
            }

            float GetYaw() const { return yaw; }
            float GetPitch() const { return pitch; }

        protected:
            void HandleMouseInput(float dt);
            void UpdateCameraPosition(float dt);
            void HandleCameraCollision();
            Vector3 LerpVector3(const Vector3& start, const Vector3& end, float t);

            GameObject* target;
            PerspectiveCamera* camera;
            GameWorld* gameWorld;
            PlayerController* playerController;

            Vector3 initialOffset;
            Vector3 currentOffset;
            Vector3 lastTargetPos;
            Vector3 lastCameraPos;

            float yaw = 0.0f;
            float pitch = 0.0f;

            float mouseSensitivity = 15.5f;
            float positionSmoothSpeed = 10.0f;
            float maxPitchAngle = 85.0f;
        };
    }
}