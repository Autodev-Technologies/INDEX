#include "ToolbarPanel.h"
#include "Editor.h"
#include <Index/Graphics/Camera/Camera.h>
#include <Index/Core/Application.h>
#include <Index/Scene/SceneManager.h>
#include <Index/Core/Engine.h>
#include <Index/Core/Profiler.h>
#include <Index/Graphics/RHI/GraphicsContext.h>
#include <Index/Graphics/RHI/Texture.h>
#include <Index/Graphics/RHI/SwapChain.h>
#include <Index/Graphics/Renderers/RenderPasses.h>
#include <Index/Graphics/GBuffer.h>
#include <Index/Graphics/Light.h>
#include <Index/Scene/Component/SoundComponent.h>
#include <Index/Graphics/Renderers/GridRenderer.h>
#include <Index/Physics/IndexPhysicsEngine/IndexPhysicsEngine.h>
#include <Index/Physics/B2PhysicsEngine/B2PhysicsEngine.h>
#include <Index/Core/OS/Input.h>
#include <Index/Graphics/Renderers/DebugRenderer.h>
#include <Index/ImGui/IconsMaterialDesignIcons.h>
#include <Index/Graphics/Camera/EditorCamera.h>
#include <Index/ImGui/ImGuiUtilities.h>
#include <Index/Events/ApplicationEvent.h>

#include <box2d/box2d.h>
#include <imgui/imgui_internal.h>
#include <imgui/Plugins/ImGuizmo.h>
#include "EditorPanel.h"

#include <imgui/imgui_internal.h>
#include <imgui/Plugins/ImGuizmo.h>
#include <imgui/Plugins/ImGuiAl/button/imguial_button.h>
#include <imgui/Plugins/ImTextEditor.h>
#include <imgui/Plugins/ImFileBrowser.h>
#include <iomanip>
#include <glm/gtx/matrix_decompose.hpp>

#include <cereal/version.hpp>

namespace Index
{

    /*Editor* m_Editor = nullptr;*/

    ToolbarPanel::ToolbarPanel()
    {
		m_Name = "Toolbar###toolbar";
        m_SimpleName   = "Toolbar";
    }

    void ToolbarPanel::OnImGui()
    {
        INDEX_PROFILE_FUNCTION();
        Application& app = Application::Get();

        auto Window_Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse; //| ImGuiWindowFlags_NoDocking;

        if (!ImGui::Begin(m_Name.c_str(), &m_Active, Window_Flags))
        {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.64f, 0.64f, 0.64f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (0.68f, 0.68f, 0.68f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (0.85f, 0.85f, 0.85f, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_Button, (0.85f, 0.85f, 0.85f, 1.00f));

            ImGui::PopStyleColor(4);

            ImGui::End();
            return;
        }

        {
            ToolBarMenu();
        }
    }

    void ToolbarPanel::ToolBarMenu()
    {
        INDEX_PROFILE_FUNCTION();
        ImGui::Indent();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(32.0f, 32.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 12));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        bool selected = false;

        {
            selected = m_Editor->GetImGuizmoOperation() == 81;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Button, (0.42f, 0.42f, 0.42f, 1.00f));
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_HAND))
                m_Editor->SetImGuizmoOperation(4);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Hand Tool");
        }
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::TRANSLATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Button, (0.42f, 0.42f, 0.42f, 1.00f));
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::TRANSLATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Move Tool");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::ROTATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Button, (0.42f, 0.42f, 0.42f, 1.00f));

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ROTATE_ORBIT))
                m_Editor->SetImGuizmoOperation(ImGuizmo::ROTATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Rotate Tool");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::SCALE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Button, (0.42f, 0.42f, 0.42f, 1.00f));

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_EXPAND_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::SCALE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Scale Tool");
        }
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::BOUNDS;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Button, (0.42f, 0.42f, 0.42f, 1.00f));

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_BORDER_NONE))
                m_Editor->SetImGuizmoOperation(ImGuizmo::BOUNDS);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Rect Tool");
        }
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::UNIVERSAL;
            if (selected)
                ImGui::PushStyleColor(ImGuiCol_Button, (0.42f, 0.42f, 0.42f, 1.00f));

            ImGui::SameLine();
            if (ImGui::Button(ICON_MDI_CROP_ROTATE))
                m_Editor->SetImGuizmoOperation(ImGuizmo::UNIVERSAL);

            if (selected)
                ImGui::PopStyleColor();
            ImGuiUtilities::Tooltip("Move, Rotate or Scale seleted objects.");
        }
        ImGui::SameLine();

        ImGui::PopStyleVar(4);

        ImGui::Unindent();
        ImGui::End();
    }
}
