#pragma once

#include "EditorPanel.h"

namespace Index
{
    class SettingsPanel : public EditorPanel
    {
    public:
        SettingsPanel();
        ~SettingsPanel() = default;

        void OnNewScene(Scene* scene) override { m_CurrentScene = scene; }
        void OnImGui() override;

    private:
        Scene* m_CurrentScene = nullptr;
    };
}