#include "CollisionSystem.h"
#include "../components/TransformComponent.h"
#include "../components/BoxColliderComponent.h"
#include "../components/CircleColliderComponent.h"
#include "../components/PolygonColliderComponent.h"
#include "../components/RigidBodyComponent.h"
#include "../Entity.h"
#include <algorithm>
#include <limits>

namespace Engine {

    CollisionSystem::CollisionSystem() {
        requireComponent<TransformComponent>();
    }

    void CollisionSystem::onUpdate(float dt) {
        auto entities = getSystemEntities();

        // O(n^2) check - acceptable for moderate entity counts
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                processCollisionPair(entities[i], entities[j]);
            }
        }
    }

    void CollisionSystem::processCollisionPair(Entity* a, Entity* b) {
        auto boxA = a->getComponent<BoxColliderComponent>();
        auto circA = a->getComponent<CircleColliderComponent>();
        auto polyA = a->getComponent<PolygonColliderComponent>();

        auto boxB = b->getComponent<BoxColliderComponent>();
        auto circB = b->getComponent<CircleColliderComponent>();
        auto polyB = b->getComponent<PolygonColliderComponent>();

        if (!((boxA || circA || polyA) && (boxB || circB || polyB))) return;

        glm::vec2 normal;
        float penetration;

        if ((boxA || polyA) && (boxB || polyB)) {
            if (checkPolygonPolygon(getWorldVertices(a), getWorldVertices(b), normal, penetration)) {
                handleCollisionResult(a, b, normal, penetration);
            }
        }

        if (circA && circB) {
            if (checkCircleCircle(a, circA, b, circB, normal, penetration)) {
                handleCollisionResult(a, b, normal, penetration);
            }
        }

        if (circA && (boxB || polyB)) {
            if (checkCirclePolygon(a, circA, getWorldVertices(b), normal, penetration)) {
                handleCollisionResult(a, b, normal, penetration);
            }
        }

        if ((boxA || polyA) && circB) {
            if (checkCirclePolygon(b, circB, getWorldVertices(a), normal, penetration)) {
                handleCollisionResult(a, b, -normal, penetration);
            }
        }
    }

    void CollisionSystem::handleCollisionResult(Entity* a, Entity* b, glm::vec2 normal, float penetration) {
        if (!canCollide(a, b)) return;

        if (isTriggerPair(a, b)) {
            fireTriggerEvents(a, b);
            return;
        }

        resolveCollision(a, b, normal, penetration);
    }

    bool CollisionSystem::canCollide(Entity* a, Entity* b) {
        uint32_t lA = 0, mA = 0, lB = 0, mB = 0;
        
        auto getFlags = [](Entity* e, uint32_t& l, uint32_t& m) {
            if (auto c = e->getComponent<BoxColliderComponent>()) { l = c->layer; m = c->mask; return; }
            if (auto c = e->getComponent<CircleColliderComponent>()) { l = c->layer; m = c->mask; return; }
            if (auto c = e->getComponent<PolygonColliderComponent>()) { l = c->layer; m = c->mask; return; }
        };

        getFlags(a, lA, mA);
        getFlags(b, lB, mB);

        return (lA & mB) && (lB & mA);
    }

    bool CollisionSystem::isTriggerPair(Entity* a, Entity* b) {
        auto isT = [](Entity* e) {
            auto ba = e->getComponent<BoxColliderComponent>();
            auto ca = e->getComponent<CircleColliderComponent>();
            auto pa = e->getComponent<PolygonColliderComponent>();
            return (ba && ba->isTrigger) || (ca && ca->isTrigger) || (pa && pa->isTrigger);
        };
        return isT(a) || isT(b);
    }

    void CollisionSystem::fireTriggerEvents(Entity* a, Entity* b) {
        auto notify = [](Entity* self, Entity* other) {
            if (auto c = self->getComponent<BoxColliderComponent>()) if(c->onTriggerEnter) c->onTriggerEnter(other);
            if (auto c = self->getComponent<CircleColliderComponent>()) if(c->onTriggerEnter) c->onTriggerEnter(other);
            if (auto c = self->getComponent<PolygonColliderComponent>()) if(c->onTriggerEnter) c->onTriggerEnter(other);
        };
        notify(a, b);
        notify(b, a);
    }

    std::vector<glm::vec2> CollisionSystem::getWorldVertices(Entity* ent) {
        auto tr = ent->getComponent<TransformComponent>();
        std::vector<glm::vec2> worldPoints;

        if (auto poly = ent->getComponent<PolygonColliderComponent>()) {
            float entRad = glm::radians(tr->rotation);
            float colRad = glm::radians(poly->rotation);
            for (const auto& p : poly->vertices) {
                float lx = p.x * cos(colRad) - p.y * sin(colRad);
                float ly = p.x * sin(colRad) + p.y * cos(colRad);
                glm::vec2 offsetP = glm::vec2(lx, ly) + poly->offset;
                float rx = offsetP.x * cos(entRad) - offsetP.y * sin(entRad);
                float ry = offsetP.x * sin(entRad) + offsetP.y * cos(entRad);
                worldPoints.push_back(tr->position + glm::vec2(rx, ry));
            }
        } else if (auto box = ent->getComponent<BoxColliderComponent>()) {
            float entRad = glm::radians(tr->rotation);
            float colRad = glm::radians(box->rotation);
            glm::vec2 half = box->size * 0.5f;
            glm::vec2 corners[4] = { -half, {half.x, -half.y}, half, {-half.x, half.y} };
            
            for (int i = 0; i < 4; i++) {
                float lx = corners[i].x * cos(colRad) - corners[i].y * sin(colRad);
                float ly = corners[i].x * sin(colRad) + corners[i].y * cos(colRad);
                glm::vec2 offsetP = glm::vec2(lx, ly) + box->offset + half;
                float rx = offsetP.x * cos(entRad) - offsetP.y * sin(entRad);
                float ry = offsetP.x * sin(entRad) + offsetP.y * cos(entRad);
                worldPoints.push_back(tr->position + glm::vec2(rx, ry));
            }
        }
        return worldPoints;
    }

    bool CollisionSystem::checkPolygonPolygon(const std::vector<glm::vec2>& vertsA, const std::vector<glm::vec2>& vertsB, glm::vec2& normal, float& penetration) {
        if (vertsA.empty() || vertsB.empty()) return false;
        
        float minOverlap = std::numeric_limits<float>::max();
        glm::vec2 smallestAxis;

        auto getAxes = [](const std::vector<glm::vec2>& verts) {
            std::vector<glm::vec2> axes;
            for (size_t i = 0; i < verts.size(); i++) {
                glm::vec2 p1 = verts[i];
                glm::vec2 p2 = verts[(i + 1) % verts.size()];
                glm::vec2 edge = p2 - p1;
                axes.push_back(glm::normalize(glm::vec2(-edge.y, edge.x)));
            }
            return axes;
        };

        std::vector<glm::vec2> allAxes = getAxes(vertsA);
        std::vector<glm::vec2> axesB = getAxes(vertsB);
        allAxes.insert(allAxes.end(), axesB.begin(), axesB.end());

        for (const auto& axis : allAxes) {
            float minA = std::numeric_limits<float>::max(), maxA = -std::numeric_limits<float>::max();
            float minB = std::numeric_limits<float>::max(), maxB = -std::numeric_limits<float>::max();

            for (const auto& v : vertsA) { float p = glm::dot(v, axis); minA = std::min(minA, p); maxA = std::max(maxA, p); }
            for (const auto& v : vertsB) { float p = glm::dot(v, axis); minB = std::min(minB, p); maxB = std::max(maxB, p); }

            if (maxA < minB || maxB < minA) return false;

            float overlap = std::min(maxA, maxB) - std::max(minA, minB);
            if (overlap < minOverlap) {
                minOverlap = overlap;
                smallestAxis = axis;
            }
        }

        penetration = minOverlap;
        normal = smallestAxis;
        
        glm::vec2 centerA(0), centerB(0);
        for(auto& v : vertsA) centerA += v; centerA /= (float)vertsA.size();
        for(auto& v : vertsB) centerB += v; centerB /= (float)vertsB.size();
        if (glm::dot(centerA - centerB, normal) < 0) normal = -normal;

        return true;
    }

    bool CollisionSystem::checkCirclePolygon(Entity* circleEnt, CircleColliderComponent* circ, const std::vector<glm::vec2>& polyVerts, glm::vec2& normal, float& penetration) {
        auto tr = circleEnt->getComponent<TransformComponent>();
        float rad = glm::radians(tr->rotation);
        glm::vec2 worldOff = { circ->offset.x * cos(rad) - circ->offset.y * sin(rad), circ->offset.x * sin(rad) + circ->offset.y * cos(rad) };
        glm::vec2 center = tr->position + worldOff;

        float minOverlap = std::numeric_limits<float>::max();
        glm::vec2 smallestAxis;

        for (size_t i = 0; i < polyVerts.size(); i++) {
            glm::vec2 p1 = polyVerts[i];
            glm::vec2 p2 = polyVerts[(i + 1) % polyVerts.size()];
            glm::vec2 edge = p2 - p1;
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

            float minP = std::numeric_limits<float>::max(), maxP = -std::numeric_limits<float>::max();
            for (const auto& v : polyVerts) { float p = glm::dot(v, axis); minP = std::min(minP, p); maxP = std::max(maxP, p); }

            float circleProj = glm::dot(center, axis);
            float minC = circleProj - circ->radius;
            float maxC = circleProj + circ->radius;

            if (maxP < minC || maxC < minP) return false;
            float overlap = std::min(maxP, maxC) - std::max(minP, minC);
            if (overlap < minOverlap) { minOverlap = overlap; smallestAxis = axis; }
        }

        glm::vec2 closestVert;
        float minDistSq = std::numeric_limits<float>::max();
        for(const auto& v : polyVerts) {
            glm::vec2 diff = center - v;
            float d2 = glm::dot(diff, diff);
            if(d2 < minDistSq) { minDistSq = d2; closestVert = v; }
        }
        
        glm::vec2 vDiff = center - closestVert;
        float vDist = glm::length(vDiff);
        if (vDist > 0.0001f) {
            glm::vec2 vAxis = vDiff / vDist;
            float minP = std::numeric_limits<float>::max(), maxP = -std::numeric_limits<float>::max();
            for (const auto& v : polyVerts) { float p = glm::dot(v, vAxis); minP = std::min(minP, p); maxP = std::max(maxP, p); }
            float cProj = glm::dot(center, vAxis);
            float minC = cProj - circ->radius, maxC = cProj + circ->radius;
            if (maxP < minC || maxC < minP) return false;
            float overlap = std::min(maxP, maxC) - std::max(minP, minC);
            if (overlap < minOverlap) { minOverlap = overlap; smallestAxis = vAxis; }
        }

        penetration = minOverlap;
        normal = smallestAxis;
        
        glm::vec2 polyCenter(0);
        for(auto& v : polyVerts) polyCenter += v; polyCenter /= (float)polyVerts.size();
        if (glm::dot(center - polyCenter, normal) < 0) normal = -normal;

        return true;
    }

    bool CollisionSystem::checkCircleCircle(Entity* a, CircleColliderComponent* ca, Entity* b, CircleColliderComponent* cb, glm::vec2& normal, float& penetration) {
        auto trA = a->getComponent<TransformComponent>();
        auto trB = b->getComponent<TransformComponent>();

        auto getCenter = [](Entity* e, CircleColliderComponent* c, TransformComponent* t) {
            float r = glm::radians(t->rotation);
            glm::vec2 off = { c->offset.x * cos(r) - c->offset.y * sin(r), c->offset.x * sin(r) + c->offset.y * cos(r) };
            return t->position + off;
        };

        glm::vec2 centerA = getCenter(a, ca, trA);
        glm::vec2 centerB = getCenter(b, cb, trB);

        glm::vec2 delta = centerA - centerB;
        float distSq = glm::dot(delta, delta);
        float radiusSum = ca->radius + cb->radius;

        if (distSq >= radiusSum * radiusSum) return false;

        float dist = sqrt(distSq);
        if (dist != 0) {
            penetration = radiusSum - dist;
            normal = delta / dist;
        } else {
            penetration = ca->radius;
            normal = glm::vec2(1, 0);
        }
        return true;
    }

    void CollisionSystem::resolveCollision(Entity* a, Entity* b, glm::vec2 normal, float penetration) {
        auto rbA = a->getComponent<RigidBodyComponent>();
        auto rbB = b->getComponent<RigidBodyComponent>();
        auto transA = a->getComponent<TransformComponent>();
        auto transB = b->getComponent<TransformComponent>();

        bool aDyn = rbA && rbA->bodyType == BodyType::Dynamic;
        bool bDyn = rbB && rbB->bodyType == BodyType::Dynamic;

        if (!aDyn && !bDyn) return;

        if (aDyn && !bDyn) {
            transA->position += normal * penetration; 
        } else if (!aDyn && bDyn) {
            transB->position -= normal * penetration; 
        } else {
            transA->position += normal * (penetration * 0.5f);
            transB->position -= normal * (penetration * 0.5f);
        }

        if (aDyn) {
            float dot = glm::dot(rbA->velocity, normal);
            if (dot < 0) rbA->velocity -= normal * dot;
        }
        if (bDyn) {
            float dot = glm::dot(rbB->velocity, normal);
            if (dot > 0) rbB->velocity -= normal * dot;
        }
    }
}