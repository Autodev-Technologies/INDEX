#pragma once

#include "EditorPanel.h"

namespace Index
{
    class ProfilerPanel : public EditorPanel
    {
    public:
        ProfilerPanel();
        ~ProfilerPanel() = default;

        void OnImGui() override;

        std::vector<float> m_FPSData;
    };
}
