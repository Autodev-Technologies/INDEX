#pragma once

#include "Physics/IndexPhysicsEngine/RigidBody3D.h"

namespace Index
{

    struct INDEX_EXPORT CollisionPair
    {
        RigidBody3D* pObjectA;
        RigidBody3D* pObjectB;
    };

    class INDEX_EXPORT Broadphase
    {
    public:
        virtual ~Broadphase()                                                                                                             = default;
        virtual void FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount, std::vector<CollisionPair>& collisionPairs) = 0;
        virtual void DebugDraw()                                                                                                          = 0;
    };
}
