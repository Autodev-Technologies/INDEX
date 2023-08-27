#pragma once
#include "EditorPanel.h"
#include "Editor.h"
#include <Index/Maths/Maths.h>
#include <Index/Maths/Frustum.h>
#include <Index/Maths/Transform.h>
#include <Index/Graphics/Camera/Camera.h>
#include <Index/Core/StringUtilities.h>
#include <Index/ImGui/ImGuiUtilities.h>
#include <Index/Graphics/Renderers/RenderPasses.h>
#include <imgui/imgui.h>

namespace Index
{
    class GameViewPanel : public EditorPanel
    {
    public:
        GameViewPanel();
        ~GameViewPanel() = default;

        void OnImGui() override;
        void OnNewScene(Scene* scene) override;
        void OnRender() override;
        void DrawGizmos(float width, float height, float xpos, float ypos, Scene* scene);

        void Resize(uint32_t width, uint32_t height);
        void ToolBar();

    private:
        SharedPtr<Graphics::Texture2D> m_GameViewTexture = nullptr;
        Scene* m_CurrentScene                            = nullptr;
        uint32_t m_Width, m_Height;
        UniquePtr<Graphics::RenderPasses> m_RenderPasses;
        bool m_GameViewVisible = false;
        bool m_ShowStats       = true;
    };
}
