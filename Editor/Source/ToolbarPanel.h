#include "EditorPanel.h"
#include "Editor.h"
#include <Index/Maths/Maths.h>
#include <Index/Maths/Frustum.h>
#include <Index/Maths/Transform.h>
#include <Index/Graphics/Camera/Camera.h>
#include <Index/Core/StringUtilities.h>
#include <Index/ImGui/ImGuiUtilities.h>

#include <imgui/imgui.h>
#include <Index/Scene/SceneManager.h>
#include <Index/Scene/Entity.h>
#include <Index/Scene/EntityManager.h>

#include <imgui/imgui.h>
DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entt.hpp>
DISABLE_WARNING_POP

namespace Index
{
    class ToolbarPanel : public EditorPanel
    {
    public:
        ToolbarPanel();
        ~ToolbarPanel() = default;

        void OnImGui() override;
        void ToolBarMenu();

    private:

    };
}
