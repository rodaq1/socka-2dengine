#pragma once

#include "../System.h"
#include <glm/glm.hpp>
#include <vector>

namespace Engine {

    class Entity;
    class BoxColliderComponent;
    class CircleColliderComponent;
    class PolygonColliderComponent;

    /**
     * @brief High-performance 2D Collision System.
     * Handles SAT-based Polygon/Box collisions and Circle-based collisions.
     */
    class CollisionSystem : public System {
    public:
        CollisionSystem();
        virtual ~CollisionSystem() = default;

        /** @brief Process all entity collisions in the scene. */
        void onUpdate(float dt) override;

    private:
        /** @brief Helper to check and handle collision between two entities. */
        void processCollisionPair(Entity* a, Entity* b);

        /** @brief Centralized resolution for triggers and physical response. */
        void handleCollisionResult(Entity* a, Entity* b, glm::vec2 normal, float penetration);

        /** @brief Filtering based on layer and mask bitflags. */
        bool canCollide(Entity* a, Entity* b);

        /** @brief Checks if any collider in the pair is marked as a trigger. */
        bool isTriggerPair(Entity* a, Entity* b);

        /** @brief Executes onTriggerEnter callbacks for both entities. */
        void fireTriggerEvents(Entity* a, Entity* b);

        // --- Geometric Intersection Logic ---
        
        /** @brief Returns world-space vertices for Box or Polygon colliders. */
        std::vector<glm::vec2> getWorldVertices(Entity* ent);

        /** @brief SAT implementation for two convex polygons. */
        bool checkPolygonPolygon(const std::vector<glm::vec2>& vertsA, const std::vector<glm::vec2>& vertsB, glm::vec2& normal, float& penetration);
        
        /** @brief Intersection test between a circle and a convex polygon. */
        bool checkCirclePolygon(Entity* circleEnt, CircleColliderComponent* circ, const std::vector<glm::vec2>& polyVerts, glm::vec2& normal, float& penetration);
        
        /** @brief Intersection test between two circles. */
        bool checkCircleCircle(Entity* a, CircleColliderComponent* ca, Entity* b, CircleColliderComponent* cb, glm::vec2& normal, float& penetration);
        
        /** @brief Static resolution that adjusts positions and velocities. */
        void resolveCollision(Entity* a, Entity* b, glm::vec2 normal, float penetration);

        static constexpr float penetrationSlop = 0.01f;
        static constexpr float penetrationPercent = 0.8f;

    };
}