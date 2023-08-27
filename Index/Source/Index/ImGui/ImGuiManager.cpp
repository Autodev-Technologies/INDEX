#include "Precompiled.h"
#include "ImGuiManager.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Core/Application.h"
#include "Graphics/RHI/IMGUIRenderer.h"
#include "Core/VFS.h"
#include "ImGuiUtilities.h"

#include "IconsMaterialDesignIcons.h"

#include <imgui/imgui.h>
#include <imgui/Plugins/ImGuizmo.h>
#include <imgui/Plugins/ImGuiAl/fonts/MaterialDesign.inl>
#include <imgui/Plugins/ImGuiAl/fonts/RobotoMedium.inl>
#include <imgui/Plugins/ImGuiAl/fonts/RobotoRegular.inl>
#include <imgui/Plugins/ImGuiAl/fonts/RobotoBold.inl>
#include <imgui/misc/freetype/imgui_freetype.h>

#if defined(INDEX_PLATFORM_MACOS) || defined(INDEX_PLATFORM_WINDOWS) || defined(INDEX_PLATFORM_LINUX)
#define USING_GLFW
#endif

#ifdef USING_GLFW
#include <GLFW/glfw3.h>
#endif

namespace Index
{
    ImGuiManager::ImGuiManager(bool clearScreen)
    {
        m_ClearScreen = clearScreen;
        m_FontSize    = 15.0f;

#ifdef INDEX_PLATFORM_IOS
        m_FontSize *= 2.0f;
#endif
    }

    ImGuiManager::~ImGuiManager()
    {
    }

#ifdef USING_GLFW
    static const char* ImGui_ImplGlfw_GetClipboardText(void*)
    {
        return glfwGetClipboardString((GLFWwindow*)Application::Get().GetWindow()->GetHandle());
    }

    static void ImGui_ImplGlfw_SetClipboardText(void*, const char* text)
    {
        glfwSetClipboardString((GLFWwindow*)Application::Get().GetWindow()->GetHandle(), text);
    }
#endif

    void ImGuiManager::OnInit()
    {
        INDEX_PROFILE_FUNCTION();

        INDEX_LOG_INFO("ImGui Version : {0}", IMGUI_VERSION);
#ifdef IMGUI_USER_CONFIG
        INDEX_LOG_INFO("ImConfig File : {0}", std::string(IMGUI_USER_CONFIG));
#endif
        Application& app = Application::Get();
        ImGuiIO& io      = ImGui::GetIO();
        io.DisplaySize   = ImVec2(static_cast<float>(app.GetWindow()->GetWidth()), static_cast<float>(app.GetWindow()->GetHeight()));
        // io.DisplayFramebufferScale = ImVec2(app.GetWindow()->GetDPIScale(), app.GetWindow()->GetDPIScale());
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        m_DPIScale = app.GetWindow()->GetDPIScale();
#ifdef INDEX_PLATFORM_IOS
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
#endif
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        m_FontSize *= app.GetWindow()->GetDPIScale();

        SetImGuiKeyCodes();
        SetImGuiStyle();

#ifdef INDEX_PLATFORM_IOS
        ImGui::GetStyle().ScaleAllSizes(1.5f);
        ImGuiStyle& style = ImGui::GetStyle();

        style.ScrollbarSize = 20;
#endif
#ifdef INDEX_PLATFORM_MACOS
        ImGui::GetStyle().ScaleAllSizes(m_DPIScale);
#endif

        m_IMGUIRenderer = UniquePtr<Graphics::IMGUIRenderer>(Graphics::IMGUIRenderer::Create(app.GetWindow()->GetWidth(), app.GetWindow()->GetHeight(), m_ClearScreen));

        if(m_IMGUIRenderer)
            m_IMGUIRenderer->Init();

#ifdef USING_GLFW
        io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
        io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
#endif
    }

    void ImGuiManager::OnUpdate(const TimeStep& dt, Scene* scene)
    {
        INDEX_PROFILE_FUNCTION();
        if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
        {
            m_IMGUIRenderer->NewFrame();
        }

        ImGuizmo::BeginFrame();

        Application::Get().OnImGui();
    }

    void ImGuiManager::OnEvent(Event& event)
    {
        INDEX_PROFILE_FUNCTION();
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseButtonPressedEvent));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseButtonReleasedEvent));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseMovedEvent));
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseScrolledEvent));
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(ImGuiManager::OnKeyPressedEvent));
        dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(ImGuiManager::OnKeyReleasedEvent));
        dispatcher.Dispatch<KeyTypedEvent>(BIND_EVENT_FN(ImGuiManager::OnKeyTypedEvent));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(ImGuiManager::OnWindowResizeEvent));
    }

    void ImGuiManager::OnRender(Scene* scene)
    {
        INDEX_PROFILE_FUNCTION();
        if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
        {
            m_IMGUIRenderer->Render(nullptr);
        }
    }

    void ImGuiManager::OnNewScene(Scene* scene)
    {
        INDEX_PROFILE_FUNCTION();
        m_IMGUIRenderer->Clear();
    }

    int IndexMouseButtonToImGui(Index::InputCode::MouseKey key)
    {
        switch(key)
        {
        case Index::InputCode::MouseKey::ButtonLeft:
            return 0;
        case Index::InputCode::MouseKey::ButtonRight:
            return 1;
        case Index::InputCode::MouseKey::ButtonMiddle:
            return 2;
        default:
            return 4;
        }

        return 4;
    }

    bool ImGuiManager::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e)
    {
        ImGuiIO& io                                               = ImGui::GetIO();
        io.MouseDown[IndexMouseButtonToImGui(e.GetMouseButton())] = true;

        return false;
    }

    bool ImGuiManager::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e)
    {
        ImGuiIO& io                                               = ImGui::GetIO();
        io.MouseDown[IndexMouseButtonToImGui(e.GetMouseButton())] = false;

        return false;
    }

    bool ImGuiManager::OnMouseMovedEvent(MouseMovedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        if(Input::Get().GetMouseMode() == MouseMode::Visible)
            io.MousePos = ImVec2(e.GetX() * m_DPIScale, e.GetY() * m_DPIScale);

        return false;
    }

    bool ImGuiManager::OnMouseScrolledEvent(MouseScrolledEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel += e.GetYOffset() / 10.0f;
        io.MouseWheelH += e.GetXOffset() / 10.0f;

        return false;
    }

    bool ImGuiManager::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        ImGuiIO& io                      = ImGui::GetIO();
        io.KeysDown[(int)e.GetKeyCode()] = true;

        io.KeyCtrl  = io.KeysDown[(int)Index::InputCode::Key::LeftControl] || io.KeysDown[(int)Index::InputCode::Key::RightControl];
        io.KeyShift = io.KeysDown[(int)Index::InputCode::Key::LeftShift] || io.KeysDown[(int)Index::InputCode::Key::RightShift];
        io.KeyAlt   = io.KeysDown[(int)Index::InputCode::Key::LeftAlt] || io.KeysDown[(int)Index::InputCode::Key::RightAlt];

#ifdef _WIN32
        io.KeySuper = false;
#else
        io.KeySuper = io.KeysDown[(int)Index::InputCode::Key::LeftSuper] || io.KeysDown[(int)Index::InputCode::Key::RightSuper];
#endif

        return io.WantTextInput;
    }

    bool ImGuiManager::OnKeyReleasedEvent(KeyReleasedEvent& e)
    {
        ImGuiIO& io                      = ImGui::GetIO();
        io.KeysDown[(int)e.GetKeyCode()] = false;

        return false;
    }

    bool ImGuiManager::OnKeyTypedEvent(KeyTypedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        int keycode = (int)e.Character;
        if(keycode > 0 && keycode < 0x10000)
            io.AddInputCharacter((unsigned short)keycode);

        return false;
    }

    bool ImGuiManager::OnWindowResizeEvent(WindowResizeEvent& e)
    {
        INDEX_PROFILE_FUNCTION();
        ImGuiIO& io = ImGui::GetIO();

        uint32_t width  = Maths::Max(1u, e.GetWidth());
        uint32_t height = Maths::Max(1u, e.GetHeight());

        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
        // io.DisplayFramebufferScale = ImVec2(e.GetDPIScale(), e.GetDPIScale());
        m_DPIScale = e.GetDPIScale();
        m_IMGUIRenderer->OnResize(width, height);

        return false;
    }

    void ImGuiManager::SetImGuiKeyCodes()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.KeyMap[ImGuiKey_Tab]        = (int)Index::InputCode::Key::Tab;
        io.KeyMap[ImGuiKey_LeftArrow]  = (int)Index::InputCode::Key::Left;
        io.KeyMap[ImGuiKey_RightArrow] = (int)Index::InputCode::Key::Right;
        io.KeyMap[ImGuiKey_UpArrow]    = (int)Index::InputCode::Key::Up;
        io.KeyMap[ImGuiKey_DownArrow]  = (int)Index::InputCode::Key::Down;
        io.KeyMap[ImGuiKey_PageUp]     = (int)Index::InputCode::Key::PageUp;
        io.KeyMap[ImGuiKey_PageDown]   = (int)Index::InputCode::Key::PageDown;
        io.KeyMap[ImGuiKey_Home]       = (int)Index::InputCode::Key::Home;
        io.KeyMap[ImGuiKey_End]        = (int)Index::InputCode::Key::End;
        io.KeyMap[ImGuiKey_Insert]     = (int)Index::InputCode::Key::Insert;
        io.KeyMap[ImGuiKey_Delete]     = (int)Index::InputCode::Key::Delete;
        io.KeyMap[ImGuiKey_Backspace]  = (int)Index::InputCode::Key::Backspace;
        io.KeyMap[ImGuiKey_Space]      = (int)Index::InputCode::Key::Space;
        io.KeyMap[ImGuiKey_Enter]      = (int)Index::InputCode::Key::Enter;
        io.KeyMap[ImGuiKey_Escape]     = (int)Index::InputCode::Key::Escape;
        io.KeyMap[ImGuiKey_A]          = (int)Index::InputCode::Key::A;
        io.KeyMap[ImGuiKey_C]          = (int)Index::InputCode::Key::C;
        io.KeyMap[ImGuiKey_V]          = (int)Index::InputCode::Key::V;
        io.KeyMap[ImGuiKey_X]          = (int)Index::InputCode::Key::X;
        io.KeyMap[ImGuiKey_Y]          = (int)Index::InputCode::Key::Y;
        io.KeyMap[ImGuiKey_Z]          = (int)Index::InputCode::Key::Z;
        io.KeyRepeatDelay              = 0.400f;
        io.KeyRepeatRate               = 0.05f;
    }

    void ImGuiManager::SetImGuiStyle()
    {
        INDEX_PROFILE_FUNCTION();
        ImGuiIO& io = ImGui::GetIO();

        //ImGui::StyleColorsDark();

        io.FontGlobalScale = 1.0f;

        ImFontConfig icons_config;
        icons_config.MergeMode   = false;
        icons_config.PixelSnapH  = true;
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.GlyphMinAdvanceX                       = 4.0f;
        icons_config.SizePixels                             = 12.0f;

        static const ImWchar ranges[] = {
            0x0020,
            0x00FF,
            0x0400,
            0x044F,
            0,
        };

        io.Fonts->AddFontFromMemoryCompressedTTF(RobotoRegular_compressed_data, RobotoRegular_compressed_size, m_FontSize, &icons_config, ranges);
        AddIconFont();

        io.Fonts->AddFontFromMemoryCompressedTTF(RobotoBold_compressed_data, RobotoBold_compressed_size, m_FontSize + 2.0f, &icons_config, ranges);

        io.Fonts->AddFontFromMemoryCompressedTTF(RobotoRegular_compressed_data, RobotoRegular_compressed_size, m_FontSize * 0.8f, &icons_config, ranges);
        AddIconFont();

        io.Fonts->AddFontDefault();
        AddIconFont();

        io.Fonts->TexGlyphPadding = 1;
        for(int n = 0; n < io.Fonts->ConfigData.Size; n++)
        {
            ImFontConfig* font_config       = (ImFontConfig*)&io.Fonts->ConfigData[n];
            font_config->RasterizerMultiply = 1.0f;
        }

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowPadding     = ImVec2(0, 4);
        style.FramePadding      = ImVec2(4.0f, 3.5f);
        style.ItemSpacing       = ImVec2(6, 2);
        style.ItemInnerSpacing  = ImVec2(2, 2);
        style.IndentSpacing     = 6.0f;
        style.WindowMenuButtonPosition = ImGuiDir_Right;
        style.TouchExtraPadding = ImVec2(4, 4);

        style.ScrollbarSize = 15;

        style.WindowBorderSize = 0;
        style.ChildBorderSize  = 1;
        style.PopupBorderSize  = 0;
        style.FrameBorderSize  = 0.0f;

        const int roundingAmount = 2;
        style.PopupRounding      = 0;
        style.WindowRounding     = roundingAmount;
        style.ChildRounding      = 0;
        style.FrameRounding      = roundingAmount;
        style.ScrollbarRounding  = 12;
        style.GrabRounding       = roundingAmount;
        style.WindowMinSize      = ImVec2(49, 49);
        style.WindowTitleAlign   = ImVec2(0.5f, 0.5f);

#ifdef IMGUI_HAS_DOCK
        style.TabBorderSize = 0.0f;
        style.TabRounding   = roundingAmount; // + 4;

        if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding              = roundingAmount;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
#endif

        ImGuiUtilities::SetTheme(ImGuiUtilities::Theme::Light);
    }

    void ImGuiManager::AddIconFont()
    {
        ImGuiIO& io = ImGui::GetIO();

        static const ImWchar icons_ranges[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 };
        ImFontConfig icons_config;
        // merge in icons from Font Awesome
        icons_config.MergeMode     = true;
        icons_config.PixelSnapH    = true;
        icons_config.GlyphOffset.y = 1.0f;
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.GlyphMinAdvanceX                       = 4.0f;
        icons_config.SizePixels                             = 12.0f;

        io.Fonts->AddFontFromMemoryCompressedTTF(MaterialDesign_compressed_data, MaterialDesign_compressed_size, m_FontSize, &icons_config, icons_ranges);
    }
}
