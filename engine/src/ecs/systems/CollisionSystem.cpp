#include "CollisionSystem.h"
#include "../components/TransformComponent.h"
#include "../components/BoxColliderComponent.h"
#include "../components/CircleColliderComponent.h"
#include "../components/PolygonColliderComponent.h"
#include "../components/RigidBodyComponent.h"
#include "../Entity.h"
#include <algorithm>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Engine
{

    // Same hierarchy world-matrix logic as renderer
    static glm::mat4 GetWorldMatrix(Engine::Entity *e)
    {
        if (!e)
            return glm::mat4(1.0f);

        auto tr = e->getComponent<Engine::TransformComponent>();
        if (!tr)
            return glm::mat4(1.0f);

        glm::mat4 local(1.0f);
        local = glm::translate(local, glm::vec3(tr->position, 0.0f));
        local = glm::rotate(local, glm::radians(tr->rotation), glm::vec3(0, 0, 1));
        local = glm::scale(local, glm::vec3(tr->scale, 1.0f));

        if (e->getParent())
            return GetWorldMatrix(e->getParent()) * local;
        return local;
    }

    static glm::vec2 TransformPoint(const glm::mat4 &m, const glm::vec2 &p)
    {
        glm::vec4 r = m * glm::vec4(p.x, p.y, 0.0f, 1.0f);
        return {r.x, r.y};
    }

    // Extract world XY scale from matrix columns (robust for parent scaling)
    static glm::vec2 ExtractScaleXY(const glm::mat4 &m)
    {
        // column 0 = X axis, column 1 = Y axis (in glm)
        float sx = std::sqrt(m[0][0] * m[0][0] + m[1][0] * m[1][0]);
        float sy = std::sqrt(m[0][1] * m[0][1] + m[1][1] * m[1][1]);
        return {sx, sy};
    }

    static float Clamp01(float x) { return (x < 0.0f) ? 0.0f : (x > 1.0f) ? 1.0f
                                                                          : x; }

    static glm::vec2 ClosestPointOnSegment(const glm::vec2 &a, const glm::vec2 &b, const glm::vec2 &p)
    {
        glm::vec2 ab = b - a;
        float ab2 = glm::dot(ab, ab);
        if (ab2 < 1e-10f)
            return a;
        float t = glm::dot(p - a, ab) / ab2;
        t = Clamp01(t);
        return a + ab * t;
    }

    static void ProjectPolygon(const std::vector<glm::vec2> &verts, const glm::vec2 &axis, float &outMin, float &outMax)
    {
        outMin = std::numeric_limits<float>::max();
        outMax = -std::numeric_limits<float>::max();
        for (const auto &v : verts)
        {
            float p = glm::dot(v, axis);
            outMin = std::min(outMin, p);
            outMax = std::max(outMax, p);
        }
    }

    static void ProjectCircle(const glm::vec2 &center, float radius, const glm::vec2 &axis, float &outMin, float &outMax)
    {
        float c = glm::dot(center, axis);
        outMin = c - radius;
        outMax = c + radius;
    }

    static void ApplyWorldTranslation(Engine::Entity *e, const glm::vec2 &worldDelta)
    {
        if (!e)
            return;
        auto tr = e->getComponent<Engine::TransformComponent>();
        if (!tr)
            return;

        if (auto parent = e->getParent())
        {
            glm::mat4 parentWorld = GetWorldMatrix(parent);
            glm::mat4 invParent = glm::inverse(parentWorld);

            // Convert world delta to parent's local space direction.
            glm::vec4 localDelta4 = invParent * glm::vec4(worldDelta.x, worldDelta.y, 0.0f, 0.0f);
            tr->position += glm::vec2(localDelta4.x, localDelta4.y);
        }
        else
        {
            tr->position += worldDelta;
        }
    }

    static glm::vec2 GetCircleWorldCenter(Engine::Entity *e, Engine::CircleColliderComponent *c)
    {
        glm::mat4 world = GetWorldMatrix(e);
        glm::mat4 col(1.0f);
        col = glm::translate(col, glm::vec3(c->offset, 0.0f));
        glm::vec4 p = (world * col) * glm::vec4(0, 0, 0, 1);
        return {p.x, p.y};
    }

    static float GetCircleWorldRadius(Engine::Entity *e, Engine::CircleColliderComponent *c)
    {
        glm::mat4 world = GetWorldMatrix(e);
        glm::vec2 xAxis = glm::vec2(world[0][0], world[1][0]); // transformed (1,0)
        glm::vec2 yAxis = glm::vec2(world[0][1], world[1][1]); // transformed (0,1)
        float sx = glm::length(xAxis);
        float sy = glm::length(yAxis);
        float s = (sx + sy) * 0.5f;
        return c->radius * s;
    }

    CollisionSystem::CollisionSystem()
    {
        requireComponent<TransformComponent>();
    }

    void CollisionSystem::onUpdate(float dt)
    {
        auto entities = getSystemEntities();

        // O(n^2) check - acceptable for moderate entity counts
        for (size_t i = 0; i < entities.size(); ++i)
        {
            for (size_t j = i + 1; j < entities.size(); ++j)
            {
                processCollisionPair(entities[i], entities[j]);
            }
        }
    }

    void CollisionSystem::processCollisionPair(Entity *a, Entity *b)
    {
        auto boxA = a->getComponent<BoxColliderComponent>();
        auto circA = a->getComponent<CircleColliderComponent>();
        auto polyA = a->getComponent<PolygonColliderComponent>();

        auto boxB = b->getComponent<BoxColliderComponent>();
        auto circB = b->getComponent<CircleColliderComponent>();
        auto polyB = b->getComponent<PolygonColliderComponent>();

        if (!((boxA || circA || polyA) && (boxB || circB || polyB)))
            return;

        glm::vec2 normal;
        float penetration;

        if ((boxA || polyA) && (boxB || polyB))
        {
            if (checkPolygonPolygon(getWorldVertices(a), getWorldVertices(b), normal, penetration))
            {
                handleCollisionResult(a, b, normal, penetration);
            }
        }

        if (circA && circB)
        {
            if (checkCircleCircle(a, circA, b, circB, normal, penetration))
            {
                handleCollisionResult(a, b, normal, penetration);
            }
        }

        if (circA && (boxB || polyB))
        {
            if (checkCirclePolygon(a, circA, getWorldVertices(b), normal, penetration))
            {
                handleCollisionResult(a, b, normal, penetration);
            }
        }

        if ((boxA || polyA) && circB)
        {
            if (checkCirclePolygon(b, circB, getWorldVertices(a), normal, penetration))
            {
                handleCollisionResult(a, b, -normal, penetration);
            }
        }
    }

    void CollisionSystem::handleCollisionResult(Entity *a, Entity *b, glm::vec2 normal, float penetration)
    {
        if (!canCollide(a, b))
            return;

        if (isTriggerPair(a, b))
        {
            fireTriggerEvents(a, b);
            return;
        }

        resolveCollision(a, b, normal, penetration);
    }

    bool CollisionSystem::canCollide(Entity *a, Entity *b)
    {
        uint32_t lA = 0, mA = 0, lB = 0, mB = 0;

        auto getFlags = [](Entity *e, uint32_t &l, uint32_t &m)
        {
            if (auto c = e->getComponent<BoxColliderComponent>())
            {
                l = c->layer;
                m = c->mask;
                return;
            }
            if (auto c = e->getComponent<CircleColliderComponent>())
            {
                l = c->layer;
                m = c->mask;
                return;
            }
            if (auto c = e->getComponent<PolygonColliderComponent>())
            {
                l = c->layer;
                m = c->mask;
                return;
            }
        };

        getFlags(a, lA, mA);
        getFlags(b, lB, mB);

        return (lA & mB) && (lB & mA);
    }

    bool CollisionSystem::isTriggerPair(Entity *a, Entity *b)
    {
        auto isT = [](Entity *e)
        {
            auto ba = e->getComponent<BoxColliderComponent>();
            auto ca = e->getComponent<CircleColliderComponent>();
            auto pa = e->getComponent<PolygonColliderComponent>();
            return (ba && ba->isTrigger) || (ca && ca->isTrigger) || (pa && pa->isTrigger);
        };
        return isT(a) || isT(b);
    }

    void CollisionSystem::fireTriggerEvents(Entity *a, Entity *b)
    {
        auto notify = [](Entity *self, Entity *other)
        {
            if (auto c = self->getComponent<BoxColliderComponent>())
                if (c->onTriggerEnter)
                    c->onTriggerEnter(other);
            if (auto c = self->getComponent<CircleColliderComponent>())
                if (c->onTriggerEnter)
                    c->onTriggerEnter(other);
            if (auto c = self->getComponent<PolygonColliderComponent>())
                if (c->onTriggerEnter)
                    c->onTriggerEnter(other);
        };
        notify(a, b);
        notify(b, a);
    }

    std::vector<glm::vec2> CollisionSystem::getWorldVertices(Entity *ent)
    {
        std::vector<glm::vec2> out;
        if (!ent)
            return out;

        glm::mat4 world = GetWorldMatrix(ent);

        // Polygon collider
        if (auto poly = ent->getComponent<PolygonColliderComponent>())
        {
            // collider local transform: offset + collider rotation
            glm::mat4 col(1.0f);
            col = glm::translate(col, glm::vec3(poly->offset, 0.0f));
            col = glm::rotate(col, glm::radians(poly->rotation), glm::vec3(0, 0, 1));

            glm::mat4 M = world * col;

            out.reserve(poly->vertices.size());
            for (const auto &v : poly->vertices)
            {
                glm::vec4 p = M * glm::vec4(v.x, v.y, 0.0f, 1.0f);
                out.push_back(glm::vec2(p.x, p.y));
            }
            return out;
        }

        // Box collider (treat as 4 vertices around origin)
        if (auto box = ent->getComponent<BoxColliderComponent>())
        {
            glm::vec2 half = box->size * 0.5f;

            glm::vec2 corners[4] = {
                {-half.x, -half.y},
                {half.x, -half.y},
                {half.x, half.y},
                {-half.x, half.y}};

            glm::mat4 col(1.0f);
            col = glm::translate(col, glm::vec3(box->offset, 0.0f));
            col = glm::rotate(col, glm::radians(box->rotation), glm::vec3(0, 0, 1));

            glm::mat4 M = world * col;

            out.reserve(4);
            for (int i = 0; i < 4; i++)
            {
                glm::vec4 p = M * glm::vec4(corners[i].x, corners[i].y, 0.0f, 1.0f);
                out.push_back(glm::vec2(p.x, p.y));
            }
            return out;
        }

        return out;
    }

    bool CollisionSystem::checkPolygonPolygon(const std::vector<glm::vec2> &vertsA, const std::vector<glm::vec2> &vertsB, glm::vec2 &normal, float &penetration)
    {
        if (vertsA.empty() || vertsB.empty())
            return false;

        float minOverlap = std::numeric_limits<float>::max();
        glm::vec2 smallestAxis;

        auto getAxes = [](const std::vector<glm::vec2> &verts)
        {
            std::vector<glm::vec2> axes;
            for (size_t i = 0; i < verts.size(); i++)
            {
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

        for (const auto &axis : allAxes)
        {
            float minA = std::numeric_limits<float>::max(), maxA = -std::numeric_limits<float>::max();
            float minB = std::numeric_limits<float>::max(), maxB = -std::numeric_limits<float>::max();

            for (const auto &v : vertsA)
            {
                float p = glm::dot(v, axis);
                minA = std::min(minA, p);
                maxA = std::max(maxA, p);
            }
            for (const auto &v : vertsB)
            {
                float p = glm::dot(v, axis);
                minB = std::min(minB, p);
                maxB = std::max(maxB, p);
            }

            if (maxA < minB || maxB < minA)
                return false;

            float overlap = std::min(maxA, maxB) - std::max(minA, minB);
            if (overlap < minOverlap)
            {
                minOverlap = overlap;
                smallestAxis = axis;
            }
        }

        penetration = minOverlap;
        normal = smallestAxis;

        glm::vec2 centerA(0), centerB(0);
        for (auto &v : vertsA)
            centerA += v;
        centerA /= (float)vertsA.size();
        for (auto &v : vertsB)
            centerB += v;
        centerB /= (float)vertsB.size();
        if (glm::dot(centerA - centerB, normal) < 0)
            normal = -normal;

        return true;
    }

    bool Engine::CollisionSystem::checkCircleCircle(
        Entity *a, CircleColliderComponent *ca,
        Entity *b, CircleColliderComponent *cb,
        glm::vec2 &normal, float &penetration)
    {
        if (!a || !b || !ca || !cb)
            return false;

        // World matrices (include parent transform + scale)
        glm::mat4 wA = GetWorldMatrix(a);
        glm::mat4 wB = GetWorldMatrix(b);

        // Circle centers in world space (offset is local to entity)
        glm::vec2 centerA = TransformPoint(wA, ca->offset);
        glm::vec2 centerB = TransformPoint(wB, cb->offset);

        // World radius (conservative for non-uniform scale)
        glm::vec2 sA = ExtractScaleXY(wA);
        glm::vec2 sB = ExtractScaleXY(wB);
        float rA = ca->radius * std::max(std::abs(sA.x), std::abs(sA.y));
        float rB = cb->radius * std::max(std::abs(sB.x), std::abs(sB.y));

        glm::vec2 delta = centerA - centerB;
        float distSq = glm::dot(delta, delta);
        float rSum = rA + rB;

        if (distSq >= rSum * rSum)
            return false;

        float dist = std::sqrt(distSq);
        if (dist > 1e-8f)
        {
            penetration = rSum - dist;
            normal = delta / dist; // points from B -> A
        }
        else
        {
            // Same center: pick stable normal
            penetration = rSum;
            normal = glm::vec2(1.0f, 0.0f);
        }

        return true;
    }

    bool Engine::CollisionSystem::checkCirclePolygon(
        Entity *circleEnt, CircleColliderComponent *circ,
        const std::vector<glm::vec2> &polyVerts,
        glm::vec2 &normal, float &penetration)
    {
        if (!circleEnt || !circ || polyVerts.size() < 3)
            return false;

        // World center + radius
        glm::mat4 wC = GetWorldMatrix(circleEnt);
        glm::vec2 center = TransformPoint(wC, circ->offset);

        glm::vec2 sC = ExtractScaleXY(wC);
        float radius = circ->radius * std::max(std::abs(sC.x), std::abs(sC.y));

        float minOverlap = std::numeric_limits<float>::max();
        glm::vec2 bestAxis(0.0f);

        // 1) SAT vs polygon edge normals
        for (size_t i = 0; i < polyVerts.size(); i++)
        {
            const glm::vec2 &p1 = polyVerts[i];
            const glm::vec2 &p2 = polyVerts[(i + 1) % polyVerts.size()];
            glm::vec2 edge = p2 - p1;

            float len2 = glm::dot(edge, edge);
            if (len2 < 1e-10f)
                continue;

            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

            float minP, maxP, minC, maxC;
            ProjectPolygon(polyVerts, axis, minP, maxP);
            ProjectCircle(center, radius, axis, minC, maxC);

            if (maxP < minC || maxC < minP)
                return false; // separated

            float overlap = std::min(maxP, maxC) - std::max(minP, minC);
            if (overlap < minOverlap)
            {
                minOverlap = overlap;
                bestAxis = axis;
            }
        }

        // 2) Extra axis: center -> closest point on polygon (fixes corner cases)
        glm::vec2 closest = polyVerts[0];
        float bestDistSq = std::numeric_limits<float>::max();

        for (size_t i = 0; i < polyVerts.size(); i++)
        {
            const glm::vec2 &a = polyVerts[i];
            const glm::vec2 &b = polyVerts[(i + 1) % polyVerts.size()];
            glm::vec2 cp = ClosestPointOnSegment(a, b, center);
            float d2 = glm::dot(center - cp, center - cp);
            if (d2 < bestDistSq)
            {
                bestDistSq = d2;
                closest = cp;
            }
        }

        glm::vec2 toCenter = center - closest;
        float toLen2 = glm::dot(toCenter, toCenter);
        if (toLen2 > 1e-10f)
        {
            glm::vec2 axis = glm::normalize(toCenter);

            float minP, maxP, minC, maxC;
            ProjectPolygon(polyVerts, axis, minP, maxP);
            ProjectCircle(center, radius, axis, minC, maxC);

            if (maxP < minC || maxC < minP)
                return false;

            float overlap = std::min(maxP, maxC) - std::max(minP, minC);
            if (overlap < minOverlap)
            {
                minOverlap = overlap;
                bestAxis = axis;
            }
        }

        // Finalize
        penetration = minOverlap;
        normal = bestAxis;

        glm::vec2 polyCenter(0.0f);
        for (auto &v : polyVerts)
            polyCenter += v;
        polyCenter /= (float)polyVerts.size();

        if (glm::dot(center - polyCenter, normal) < 0.0f)
            normal = -normal;

        return true;
    }

    void CollisionSystem::resolveCollision(Entity *a, Entity *b, glm::vec2 normal, float penetration)
    {
        auto rbA = a->getComponent<RigidBodyComponent>();
        auto rbB = b->getComponent<RigidBodyComponent>();
        auto transA = a->getComponent<TransformComponent>();
        auto transB = b->getComponent<TransformComponent>();

        bool aDyn = rbA && rbA->bodyType == BodyType::Dynamic;
        bool bDyn = rbB && rbB->bodyType == BodyType::Dynamic;
        if (!aDyn && !bDyn)
            return;

        if (aDyn && !bDyn)
        {
            transA->position += normal * penetration;
        }
        else if (!aDyn && bDyn)
        {
            transB->position -= normal * penetration;
        }
        else
        {
            transA->position += normal * (penetration * 0.5f);
            transB->position -= normal * (penetration * 0.5f);
        }

        if (aDyn)
        {
            float dot = glm::dot(rbA->velocity, normal);
            if (dot < 0)
                rbA->velocity -= normal * dot;
        }
        if (bDyn)
        {
            float dot = glm::dot(rbB->velocity, normal);
            if (dot > 0)
                rbB->velocity -= normal * dot;
        }
    }
}