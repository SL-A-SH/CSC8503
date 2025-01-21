#pragma once

#include "GameObject.h"
#include "GameWorld.h"

namespace NCL {
    namespace CSC8503 {
        class DoorObject;

        class RoomTrigger : public GameObject {
        public:
            RoomTrigger(GameObject* room, DoorObject* door, const Vector3& triggerPos, const Vector3& triggerSize);
            void Update(GameObject* player);

        protected:
            GameObject* room;
            DoorObject* door;
            Vector3 doorPosition;
            bool playerInRange;
            const float doorProximityThreshold = 6.0f;
        };

        class DoorObject : public GameObject {
        public:
            DoorObject();
            void Update(float dt);
            void OpenDoor();
            void CloseDoor();
            void SetTextures(Texture* closedTex, Texture* openTex);
            bool IsOpen() const {
                return isOpen;
            }

        protected:
            bool isOpen;
            Texture* openTexture;
            Texture* closeTexture;
        };

        class ButtonTrigger : public GameObject {
        public:
            ButtonTrigger(const std::string& name = "");
            void OnCollisionBegin(GameObject* otherObject) override;
            void SetLinkedDoor(DoorObject* door);
            void ForceRelease();

            using ButtonCallback = std::function<void(DoorObject*)>;
            void SetButtonCallback(ButtonCallback callback) {
                onButtonPressed = callback;
            }

            bool debugMode = true;

        protected:
            bool isPressed;
            DoorObject* linkedDoor;
            ButtonCallback onButtonPressed;
        };
    }
}