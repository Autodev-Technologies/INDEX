#pragma once

#include "AI/AINode.h"

namespace Index
{
    class INDEX_EXPORT AIComponent
    {
    public:
        AIComponent();
        explicit AIComponent(SharedPtr<AINode>& aiNode);

        void OnImGui();

    private:
        SharedPtr<AINode> m_AINode;
    };
}
