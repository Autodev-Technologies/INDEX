#pragma once

namespace Index
{
    class AINode
    {
    public:
        AINode()          = default;
        virtual ~AINode() = default;

        void Update(float dt) {};
    };
}
