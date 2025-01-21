#pragma once

#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "GameObject.h"
#include "ThirdPersonCamera.h"
#include "Window.h"
#include "Debug.h" 

using namespace NCL::Maths;

namespace NCL {
    namespace CSC8503 {
        class PlayerController {
        public:
            PlayerController(GameObject* player, GameWorld* world);
            ~PlayerController() {}

            void Update(float dt);

            void SetMovementSpeed(float speed) { moveSpeed = speed; }
            void SetJumpForce(float force) { jumpForce = force; }
            void SetTurnSpeed(float speed) { turnSpeed = speed; }
            void SetDrag(float drag) { dragFactor = drag; }
            void SetMovementMapping(float yawAngle);

            bool IsGrounded() const { return isGrounded; }
            GameObject* GetPlayerObject() const { return player; }

        protected:
            void HandleMovement(float dt);
            void ApplyDrag(float dt);
            void CheckGrounded();
            Vector3 GetMovementDirection();

            GameObject* player;
            GameWorld* gameWorld;

            float moveSpeed = 20.0f;
            float jumpForce = 20.0f;
            float turnSpeed = 2.0f;
            float dragFactor = 0.2f;
            float maxSpeed = 15.0f;
            float currentYawAngle = 0.0f;

            bool isGrounded = false;
            bool isJumping = false;
            bool wasGrounded = false;
            float timeSinceLastJump = 0.0f;
            const float jumpCooldown = 0.3f;

            Vector3 currentVelocity;
            Vector3 lastPosition;
        };
    }
}