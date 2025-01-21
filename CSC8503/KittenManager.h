#pragma once

#include "GameObject.h"
#include "PhysicsObject.h"
#include <vector>

namespace NCL {
    namespace CSC8503 {
        class KittenObject : public GameObject {
        public:
            KittenObject(const std::string& name = "Kitten") : GameObject(name) {
                isFollowing = false;
            }

            void SetFollowing(bool following) { isFollowing = following; }
            bool IsFollowing() const { return isFollowing; }

        protected:
            bool isFollowing;
        };

        class KittenManager {
        public:
            KittenManager(GameObject* player) : player(player) {}
            ~KittenManager() {}

            void UpdateKittens(float dt);
            void RegisterKitten(GameObject* kitten);
            int GetFoundKittens() const { return foundKittens.size(); }
            int GetTotalKittens() const { return totalKittens; }

        protected:
            void UpdateFollowBehavior(float dt);
            bool IsNearPlayer(const Vector3& kittenPos) const;
            Vector3 CalculateFollowPosition(GameObject* kitten, int index) const;

            GameObject* player;
            std::vector<GameObject*> foundKittens;
            std::vector<GameObject*> allKittens;
            const float followDistance = 1.0f;
            const float detectionRadius = 5.0f;
            const float followSpeed = 5.0f;
            const int totalKittens = 5;
        };
    }
}