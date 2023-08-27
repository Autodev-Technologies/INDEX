#include "Precompiled.h"
#include "ImGui/ImGuiUtilities.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Core/OS/OS.h"
#include "Core/Application.h"
#include "Core/OS/Input.h"
#include <imgui/imgui_internal.h>

#ifdef INDEX_RENDER_API_VULKAN
#include "Platform/Vulkan/VKTexture.h"
#endif

namespace Index
{

    glm::vec4 SelectedColour       = glm::vec4(0.28f, 0.56f, 0.9f, 1.0f);
    glm::vec4 IconColour           = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
    static char* s_MultilineBuffer = nullptr;
    static uint32_t s_Counter      = 0;
    static char s_IDBuffer[16]     = "##";
    static char s_LabelIDBuffer[1024];
    static int s_UIContextID = 0;

    const char* ImGuiUtilities::GenerateID()
    {
        sprintf(s_IDBuffer + 2, "%x", s_Counter++);
        //_itoa(s_Counter++, s_IDBuffer + 2, 16);
        return s_IDBuffer;
    }

    const char* ImGuiUtilities::GenerateLabelID(std::string_view label)
    {
        *fmt::format_to_n(s_LabelIDBuffer, std::size(s_LabelIDBuffer), "{}##{}", label, s_Counter++).out = 0;
        return s_LabelIDBuffer;
    }

    void ImGuiUtilities::PushID()
    {
        ImGui::PushID(s_UIContextID++);
        s_Counter = 0;
    }

    void ImGuiUtilities::PopID()
    {
        ImGui::PopID();
        s_UIContextID--;
    }

    bool ImGuiUtilities::ToggleButton(const char* label, bool state, ImVec2 size, float alpha, float pressedAlpha, ImGuiButtonFlags buttonFlags)
    {
        if(state)
        {
            ImVec4 color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];

            color.w = pressedAlpha;
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
        }
        else
        {
            ImVec4 color        = ImGui::GetStyle().Colors[ImGuiCol_Button];
            ImVec4 hoveredColor = ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered];
            color.w             = alpha;
            hoveredColor.w      = pressedAlpha;
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
            color.w = pressedAlpha;
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
        }

        bool clicked = ImGui::ButtonEx(label, size, buttonFlags);

        ImGui::PopStyleColor(3);

        return clicked;
    }

    bool ImGuiUtilities::Property(const char* name, std::string& value, PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::AlignTextToFramePadding();

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::TextUnformatted(value.c_str());
        }
        else
        {
            if(ImGuiUtilities::InputText(value))
            {
                updated = true;
            }
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    void ImGuiUtilities::PropertyConst(const char* name, const char* value)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::AlignTextToFramePadding();
        {
            ImGui::TextUnformatted(value);
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();
    }

    bool ImGuiUtilities::PropertyMultiline(const char* label, std::string& value)
    {
        bool modified = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::AlignTextToFramePadding();

        if(!s_MultilineBuffer)
        {
            s_MultilineBuffer = new char[1024 * 1024]; // 1KB
            memset(s_MultilineBuffer, 0, 1024 * 1024);
        }

        strcpy(s_MultilineBuffer, value.c_str());

        // std::string id = "##" + label;
        if(ImGui::InputTextMultiline(GenerateID(), s_MultilineBuffer, 1024 * 1024))
        {
            value    = s_MultilineBuffer;
            modified = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return modified;
    }

    bool ImGuiUtilities::Property(const char* name, bool& value, PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::TextUnformatted(value ? "True" : "False");
        }
        else
        {
            // std::string id = "##" + std::string(name);
            if(ImGui::Checkbox(GenerateID(), &value))
                updated = true;
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, int& value, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%i", value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragInt(GenerateID(), &value))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, uint32_t& value, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%uui", value);
        }
        else
        {
            // std::string id = "##" + name;
            int valueInt = (int)value;
            if(ImGui::DragInt(GenerateID(), &valueInt))
            {
                updated = true;
                value   = (uint32_t)valueInt;
            }
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, float& value, float min, float max, float delta, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f", value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragFloat(GenerateID(), &value, delta, min, max))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, double& value, double min, double max, PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f", (float)value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragScalar(GenerateID(), ImGuiDataType_Double, &value))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, int& value, int min, int max, PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%i", value);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragInt(GenerateID(), &value, 1, min, max))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec2& value, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        return ImGuiUtilities::Property(name, value, -1.0f, 1.0f, flags);
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec2& value, float min, float max, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f", value.x, value.y);
        }
        else
        {
            // std::string id = "##" + name;
            if(ImGui::DragFloat2(GenerateID(), glm::value_ptr(value)))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec3& value, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        return ImGuiUtilities::Property(name, value, -1.0f, 1.0f, flags);
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec3& value, float min, float max, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f, %.2f", value.x, value.y, value.z);
        }
        else
        {
            // std::string id = "##" + name;
            if((int)flags & (int)PropertyFlag::ColourProperty)
            {
                if(ImGui::ColorEdit3(GenerateID(), glm::value_ptr(value)))
                    updated = true;
            }
            else
            {
                if(ImGui::DragFloat3(GenerateID(), glm::value_ptr(value)))
                    updated = true;
            }
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec4& value, bool exposeW, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        return Property(name, value, -1.0f, 1.0f, exposeW, flags);
    }

    bool ImGuiUtilities::Property(const char* name, glm::vec4& value, float min, float max, bool exposeW, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f, %.2f , %.2f", value.x, value.y, value.z, value.w);
        }
        else
        {

            // std::string id = "##" + name;
            if((int)flags & (int)PropertyFlag::ColourProperty)
            {
                if(ImGui::ColorEdit4(GenerateID(), glm::value_ptr(value)))
                    updated = true;
            }
            else if((exposeW ? ImGui::DragFloat4(GenerateID(), glm::value_ptr(value)) : ImGui::DragFloat4(GenerateID(), glm::value_ptr(value))))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    bool ImGuiUtilities::PorpertyTransform(const char* name, glm::vec3& vector, float width)
    {
        const float labelIndetation = ImGui::GetFontSize();
        bool updated                = false;

        auto& style = ImGui::GetStyle();

        const auto showFloat = [&](int axis, float* value)
        {
            const float label_float_spacing = ImGui::GetFontSize();
            const float step                = 0.01f;
            static const std::string format = "%.4f";

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(axis == 0 ? "X" : axis == 1 ? "Y"
                                                               : "Z");
            ImGui::SameLine(label_float_spacing);
            glm::vec2 posPostLabel = ImGui::GetCursorScreenPos();

            ImGui::PushItemWidth(width);
            ImGui::PushID(static_cast<int>(ImGui::GetCursorPosX() + ImGui::GetCursorPosY()));

            if(ImGui::InputFloat("##no_label", value, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max(), format.c_str()))
                updated = true;

            ImGui::PopID();
            ImGui::PopItemWidth();

            static const ImU32 colourX = IM_COL32(168, 46, 2, 255);
            static const ImU32 colourY = IM_COL32(112, 162, 22, 255);
            static const ImU32 colourZ = IM_COL32(51, 122, 210, 255);

            const glm::vec2 size   = glm::vec2(ImGui::GetFontSize() / 4.0f, ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y);
            posPostLabel           = posPostLabel + glm::vec2(-1.0f, ImGui::GetStyle().FramePadding.y / 2.0f);
            ImRect axis_color_rect = ImRect(posPostLabel.x, posPostLabel.y, posPostLabel.x + size.x, posPostLabel.y + size.y);
            ImGui::GetWindowDrawList()->AddRectFilled(axis_color_rect.Min, axis_color_rect.Max, axis == 0 ? colourX : axis == 1 ? colourY
                                                                                                                                : colourZ);
        };

        ImGui::BeginGroup();
        ImGui::Indent(labelIndetation);
        ImGui::TextUnformatted(name);
        ImGui::Unindent(labelIndetation);
        showFloat(0, &vector.x);
        showFloat(1, &vector.y);
        showFloat(2, &vector.z);
        ImGui::EndGroup();

        return updated;
    }

    bool ImGuiUtilities::Property(const char* name, glm::quat& value, ImGuiUtilities::PropertyFlag flags)
    {
        INDEX_PROFILE_FUNCTION();
        bool updated = false;

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if((int)flags & (int)PropertyFlag::ReadOnly)
        {
            ImGui::Text("%.2f , %.2f, %.2f , %.2f", value.x, value.y, value.z, value.w);
        }
        else
        {

            // std::string id = "##" + name;
            if(ImGui::DragFloat4(GenerateID(), glm::value_ptr(value)))
                updated = true;
        }
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return updated;
    }

    void ImGuiUtilities::Tooltip(const char* text)
    {
        INDEX_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(text);
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Tooltip(Graphics::Texture2D* texture, const glm::vec2& size)
    {
        INDEX_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
            ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Tooltip(Graphics::Texture2D* texture, const glm::vec2& size, const char* text)
    {
        INDEX_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
            ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::TextUnformatted(text);
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Tooltip(Graphics::TextureDepthArray* texture, uint32_t index, const glm::vec2& size)
    {
        INDEX_PROFILE_FUNCTION();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));

        if(ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();

            ImTextureID texID = texture ? texture->GetHandle() : nullptr;
#ifdef INDEX_RENDER_API_VULKAN
            if(texture && Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
                texID = ((Graphics::VKTextureDepthArray*)texture)->GetImageView(index);
#endif

            bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
            ImGui::Image(texID, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
            ImGui::EndTooltip();
        }

        ImGui::PopStyleVar();
    }

    void ImGuiUtilities::Image(Graphics::Texture2D* texture, const glm::vec2& size)
    {
        INDEX_PROFILE_FUNCTION();
        bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
        ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    void ImGuiUtilities::Image(Graphics::TextureCube* texture, const glm::vec2& size)
    {
        INDEX_PROFILE_FUNCTION();
        bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();
        ImGui::Image(texture ? texture->GetHandle() : nullptr, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    void ImGuiUtilities::Image(Graphics::TextureDepthArray* texture, uint32_t index, const glm::vec2& size)
    {
        INDEX_PROFILE_FUNCTION();
        bool flipImage = Graphics::Renderer::GetGraphicsContext()->FlipImGUITexture();

        ImTextureID texID = texture ? texture->GetHandle() : nullptr;
#ifdef INDEX_RENDER_API_VULKAN
        if(texture && Graphics::GraphicsContext::GetRenderAPI() == Graphics::RenderAPI::VULKAN)
            texID = ((Graphics::VKTextureDepthArray*)texture)->GetImageView(index);
#endif
        ImGui::Image(texID, ImVec2(size.x, size.y), ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
    }

    bool ImGuiUtilities::BufferingBar(const char* label, float value, const glm::vec2& size_arg, const uint32_t& bg_col, const uint32_t& fg_col)
    {
        INDEX_PROFILE_FUNCTION();
        auto g                  = ImGui::GetCurrentContext();
        auto drawList           = ImGui::GetWindowDrawList();
        const ImGuiStyle& style = ImGui::GetStyle();
        const ImGuiID id        = ImGui::GetID(label);

        ImVec2 pos  = ImGui::GetCursorPos();
        ImVec2 size = { size_arg.x, size_arg.y };
        size.x -= style.FramePadding.x * 2;

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if(!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        const float circleStart = size.x * 0.7f;
        const float circleEnd   = size.x;
        const float circleWidth = circleEnd - circleStart;

        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
        drawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);

        const float t     = float(g->Time);
        const float r     = size.y * 0.5f;
        const float speed = 1.5f;

        const float a = speed * 0.f;
        const float b = speed * 0.333f;
        const float c = speed * 0.666f;

        const float o1 = (circleWidth + r) * (t + a - speed * (int)((t + a) / speed)) / speed;
        const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
        const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;

        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
        drawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);

        return true;
    }

    bool ImGuiUtilities::Spinner(const char* label, float radius, int thickness, const uint32_t& colour)
    {
        INDEX_PROFILE_FUNCTION();
        auto g                  = ImGui::GetCurrentContext();
        const ImGuiStyle& style = g->Style;
        const ImGuiID id        = ImGui::GetID(label);
        auto drawList           = ImGui::GetWindowDrawList();

        ImVec2 pos = ImGui::GetCursorPos();
        ImVec2 size((radius)*2, (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if(!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        drawList->PathClear();

        int num_segments = 30;
        float start      = abs(ImSin(float(g->Time) * 1.8f) * (num_segments - 5));

        const float a_min = IM_PI * 2.0f * (start / float(num_segments));
        const float a_max = IM_PI * 2.0f * (float(num_segments) - 3.0f) / (float)num_segments;

        const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

        for(int i = 0; i < num_segments; i++)
        {
            const float a = a_min + (float(i) / float(num_segments)) * (a_max - a_min);
            drawList->PathLineTo(ImVec2(centre.x + ImCos(a + float(g->Time) * 8) * radius,
                                        centre.y + ImSin(a + float(g->Time) * 8) * radius));
        }

        drawList->PathStroke(colour, false, float(thickness));

        return true;
    }

    void ImGuiUtilities::DrawRowsBackground(int row_count, float line_height, float x1, float x2, float y_offset, ImU32 col_even, ImU32 col_odd)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        float y0              = ImGui::GetCursorScreenPos().y + (float)(int)y_offset;

        int row_display_start = 0;
        int row_display_end   = 0;
        // ImGui::CalcListClipping(row_count, line_height, &row_display_start, &row_display_end);
        for(int row_n = row_display_start; row_n < row_display_end; row_n++)
        {
            ImU32 col = (row_n & 1) ? col_odd : col_even;
            if((col & IM_COL32_A_MASK) == 0)
                continue;
            float y1 = y0 + (line_height * row_n);
            float y2 = y1 + line_height;
            draw_list->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), col);
        }
    }

    void ImGuiUtilities::TextCentred(const char* text)
    {
        auto windowWidth = ImGui::GetWindowSize().x;
        auto textWidth   = ImGui::CalcTextSize(text).x;

        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextUnformatted(text);
    }

    void ImGuiUtilities::SetTheme(Theme theme)
	{
		static const float max = 255.0f;

		auto& style = ImGui::GetStyle();
		ImVec4* colours = style.Colors;
		SelectedColour = glm::vec4(0.28f, 0.56f, 0.9f, 1.0f);

		// colours[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		// colours[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

		if(theme == Dark)
		{
			ImGui::StyleColorsDark();

            ImVec4* colours = ImGui::GetStyle().Colors;
            colours[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
            colours[ImGuiCol_WindowBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colours[ImGuiCol_PopupBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colours[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colours[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.09f, 0.09f, 0.67f);
            colours[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            colours[ImGuiCol_ButtonActive] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(0.46f, 0.46f, 0.46f, 0.80f);
            colours[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            colours[ImGuiCol_Separator] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_SeparatorHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_SeparatorActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(0.30f, 0.30f, 0.30f, 0.17f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.23f, 0.23f, 0.23f, 0.67f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(0.21f, 0.21f, 0.21f, 0.95f);
            colours[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_TabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            colours[ImGuiCol_TabActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
            colours[ImGuiCol_TabUnfocused] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
            colours[ImGuiCol_DockingPreview] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
            colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.29f, 0.29f, 0.29f, 1.00f);
		}

		else if(theme == Light)
		{
			ImGui::StyleColorsLight();
            colours[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_WindowBg] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colours[ImGuiCol_PopupBg] = ImVec4(0.76f, 0.76f, 0.76f, 1.00f);
            colours[ImGuiCol_FrameBg] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            colours[ImGuiCol_FrameBgHovered] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            colours[ImGuiCol_FrameBgActive] = ImVec4(0.42f, 0.42f, 0.42f, 0.67f);
            colours[ImGuiCol_TitleBg] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_TitleBgActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_MenuBarBg] = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
            colours[ImGuiCol_ScrollbarBg] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
            colours[ImGuiCol_ScrollbarGrab] = ImVec4(0.49f, 0.56f, 0.66f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.56f, 0.66f, 1.00f);
            colours[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.30f, 0.49f, 0.78f, 1.00f);
            colours[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colours[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colours[ImGuiCol_SliderGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colours[ImGuiCol_Button] = ImVec4(0.87f, 0.86f, 0.87f, 1.00f);
            colours[ImGuiCol_ButtonHovered] = ImVec4(0.87f, 0.86f, 0.87f, 1.00f);
            colours[ImGuiCol_ButtonActive] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
            colours[ImGuiCol_Header] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            colours[ImGuiCol_HeaderHovered] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            colours[ImGuiCol_HeaderActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colours[ImGuiCol_Separator] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_SeparatorHovered] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_SeparatorActive] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_ResizeGrip] = ImVec4(0.54f, 0.54f, 0.54f, 0.17f);
            colours[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.55f, 0.55f, 0.17f);
            colours[ImGuiCol_ResizeGripActive] = ImVec4(1.00f, 1.00f, 1.00f, 0.95f);
            colours[ImGuiCol_Tab] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_TabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
            colours[ImGuiCol_TabActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
            colours[ImGuiCol_TabUnfocused] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);
            colours[ImGuiCol_TabUnfocusedActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
            colours[ImGuiCol_DockingPreview] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
            colours[ImGuiCol_DockingEmptyBg] = ImVec4(0.64f, 0.64f, 0.64f, 1.00f);

		}
	}

    glm::vec4 ImGuiUtilities::GetSelectedColour()
    {
        return SelectedColour;
    }

    glm::vec4 ImGuiUtilities::GetIconColour()
    {
        return IconColour;
    }

    bool ImGuiUtilities::PropertyDropdown(const char* label, const char** options, int32_t optionCount, int32_t* selected)
    {
        const char* current = options[*selected];
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        auto drawItemActivityOutline = [](float rounding = 0.0f, bool drawWhenInactive = false)
        {
            auto* drawList = ImGui::GetWindowDrawList();
            if(ImGui::IsItemHovered() && !ImGui::IsItemActive())
            {
                drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                  ImColor(60, 60, 60), rounding, 0, 1.5f);
            }
            if(ImGui::IsItemActive())
            {
                drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                  ImColor(80, 80, 80), rounding, 0, 1.0f);
            }
            else if(!ImGui::IsItemHovered() && drawWhenInactive)
            {
                drawList->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                  ImColor(50, 50, 50), rounding, 0, 1.0f);
            }
        };

        bool changed = false;

        // const std::string id = "##" + std::string(label);

        if(ImGui::BeginCombo(GenerateID(), current))
        {
            for(int i = 0; i < optionCount; i++)
            {
                const bool is_selected = (current == options[i]);
                if(ImGui::Selectable(options[i], is_selected))
                {
                    current   = options[i];
                    *selected = i;
                    changed   = true;
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        drawItemActivityOutline(2.5f);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        return changed;
    }

    void ImGuiUtilities::DrawItemActivityOutline(float rounding, bool drawWhenInactive, ImColor colourWhenActive)
    {
        auto* drawList = ImGui::GetWindowDrawList();

        ImRect expandedRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        expandedRect.Min.x -= 1.0f;
        expandedRect.Min.y -= 1.0f;
        expandedRect.Max.x += 1.0f;
        expandedRect.Max.y += 1.0f;

        const ImRect rect = expandedRect;
        if(ImGui::IsItemHovered() && !ImGui::IsItemActive())
        {
            drawList->AddRect(rect.Min, rect.Max,
                              ImColor(60, 60, 60), rounding, 0, 1.5f);
        }
        if(ImGui::IsItemActive())
        {
            drawList->AddRect(rect.Min, rect.Max,
                              colourWhenActive, rounding, 0, 1.0f);
        }
        else if(!ImGui::IsItemHovered() && drawWhenInactive)
        {
            drawList->AddRect(rect.Min, rect.Max,
                              ImColor(50, 50, 50), rounding, 0, 1.0f);
        }
    }

    bool ImGuiUtilities::InputText(std::string& currentText)
    {
        ImGuiUtilities::ScopedStyle frameBorder(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGuiUtilities::ScopedColour frameColour(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
        char buffer[256];
        memset(buffer, 0, 256);
        memcpy(buffer, currentText.c_str(), currentText.length());

        bool updated = ImGui::InputText("##SceneName", buffer, 256);

        ImGuiUtilities::DrawItemActivityOutline(2.0f, false);

        if(updated)
            currentText = std::string(buffer);

        return updated;
    }

    void ImGuiUtilities::ClippedText(const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect, float wrap_width)
    {
        const char* text_display_end = ImGui::FindRenderedTextEnd(text, text_end);
        const int text_len           = static_cast<int>(text_display_end - text);
        if(text_len == 0)
            return;

        ImGuiContext& g     = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiUtilities::ClippedText(window->DrawList, pos_min, pos_max, text, text_display_end, text_size_if_known, align, clip_rect, wrap_width);
        if(g.LogEnabled)
            ImGui::LogRenderedText(&pos_min, text, text_display_end);
    }

    void ImGuiUtilities::ClippedText(ImDrawList* draw_list, const ImVec2& pos_min, const ImVec2& pos_max, const char* text, const char* text_display_end, const ImVec2* text_size_if_known, const ImVec2& align, const ImRect* clip_rect, float wrap_width)
    {
        // Perform CPU side clipping for single clipped element to avoid using scissor state
        ImVec2 pos             = pos_min;
        const ImVec2 text_size = text_size_if_known ? *text_size_if_known : ImGui::CalcTextSize(text, text_display_end, false, wrap_width);

        const ImVec2* clip_min = clip_rect ? &clip_rect->Min : &pos_min;
        const ImVec2* clip_max = clip_rect ? &clip_rect->Max : &pos_max;

        // Align whole block. We should defer that to the better rendering function when we'll have support for individual line alignment.
        if(align.x > 0.0f)
            pos.x = ImMax(pos.x, pos.x + (pos_max.x - pos.x - text_size.x) * align.x);

        if(align.y > 0.0f)
            pos.y = ImMax(pos.y, pos.y + (pos_max.y - pos.y - text_size.y) * align.y);

        // Render
        ImVec4 fine_clip_rect(clip_min->x, clip_min->y, clip_max->x, clip_max->y);
        draw_list->AddText(nullptr, 0.0f, pos, ImGui::GetColorU32(ImGuiCol_Text), text, text_display_end, wrap_width, &fine_clip_rect);
    }

    // from https://github.com/ocornut/imgui/issues/2668
    void ImGuiUtilities::AlternatingRowsBackground(float lineHeight)
    {
        const ImU32 im_color = ImGui::GetColorU32(ImGuiCol_TableRowBgAlt);

        auto* draw_list   = ImGui::GetWindowDrawList();
        const auto& style = ImGui::GetStyle();

        if(lineHeight < 0)
        {
            lineHeight = ImGui::GetTextLineHeight();
        }

        lineHeight += style.ItemSpacing.y;

        float scroll_offset_h    = ImGui::GetScrollX();
        float scroll_offset_v    = ImGui::GetScrollY();
        float scrolled_out_lines = std::floor(scroll_offset_v / lineHeight);
        scroll_offset_v -= lineHeight * scrolled_out_lines;

        ImVec2 clip_rect_min(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y);
        ImVec2 clip_rect_max(clip_rect_min.x + ImGui::GetWindowWidth(), clip_rect_min.y + ImGui::GetWindowHeight());

        if(ImGui::GetScrollMaxX() > 0)
        {
            clip_rect_max.y -= style.ScrollbarSize;
        }

        draw_list->PushClipRect(clip_rect_min, clip_rect_max);

        const float y_min = clip_rect_min.y - scroll_offset_v + ImGui::GetCursorPosY();
        const float y_max = clip_rect_max.y - scroll_offset_v + lineHeight;
        const float x_min = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMin().x;
        const float x_max = clip_rect_min.x + scroll_offset_h + ImGui::GetWindowContentRegionMax().x;

        bool is_odd = (static_cast<int>(scrolled_out_lines) % 2) == 0;
        for(float y = y_min; y < y_max; y += lineHeight, is_odd = !is_odd)
        {
            if(is_odd)
            {
                draw_list->AddRectFilled({ x_min, y - style.ItemSpacing.y }, { x_max, y + lineHeight }, im_color);
            }
        }

        draw_list->PopClipRect();
    }

    ImRect ImGuiUtilities::GetItemRect()
    {
        return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }

    ImRect ImGuiUtilities::RectExpanded(const ImRect& rect, float x, float y)
    {
        ImRect result = rect;
        result.Min.x -= x;
        result.Min.y -= y;
        result.Max.x += x;
        result.Max.y += y;
        return result;
    }

    ImRect ImGuiUtilities::RectOffset(const ImRect& rect, float x, float y)
    {
        ImRect result = rect;
        result.Min.x += x;
        result.Min.y += y;
        result.Max.x += x;
        result.Max.y += y;
        return result;
    }

    ImRect ImGuiUtilities::RectOffset(const ImRect& rect, ImVec2 xy)
    {
        return RectOffset(rect, xy.x, xy.y);
    }

    void ImGuiUtilities::DrawBorder(ImVec2 rectMin, ImVec2 rectMax, const ImVec4& borderColour, float thickness, float offsetX, float offsetY)
    {
        auto min = rectMin;
        min.x -= thickness;
        min.y -= thickness;
        min.x += offsetX;
        min.y += offsetY;
        auto max = rectMax;
        max.x += thickness;
        max.y += thickness;
        max.x += offsetX;
        max.y += offsetY;

        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(borderColour), 0.0f, 0, thickness);
    }

    void ImGuiUtilities::DrawBorder(const ImVec4& borderColour, float thickness, float offsetX, float offsetY)
    {
        DrawBorder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), borderColour, thickness, offsetX, offsetY);
    }

    void ImGuiUtilities::DrawBorder(float thickness, float offsetX, float offsetY)
    {
        DrawBorder(ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    }

    void ImGuiUtilities::DrawBorder(ImVec2 rectMin, ImVec2 rectMax, float thickness, float offsetX, float offsetY)
    {
        DrawBorder(rectMin, rectMax, ImGui::GetStyleColorVec4(ImGuiCol_Border), thickness, offsetX, offsetY);
    }

    void ImGuiUtilities::DrawBorder(ImRect rect, float thickness, float rounding, float offsetX, float offsetY)
    {
        auto min = rect.Min;
        min.x -= thickness;
        min.y -= thickness;
        min.x += offsetX;
        min.y += offsetY;
        auto max = rect.Max;
        max.x += thickness;
        max.y += thickness;
        max.x += offsetX;
        max.y += offsetY;

        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Border)), rounding, 0, thickness);
    }
}

namespace ImGui
{
    bool DragFloatN_Coloured(const char* label, float* v, int components, float v_speed, float v_min, float v_max, const char* display_format)
    {
        INDEX_PROFILE_FUNCTION();
        ImGuiWindow* window = GetCurrentWindow();
        if(window->SkipItems)
            return false;

        ImGuiContext& g    = *GImGui;
        bool value_changed = false;
        BeginGroup();
        PushID(label);
        PushMultiItemsWidths(components, CalcItemWidth());
        for(int i = 0; i < components; i++)
        {
            static const ImU32 colours[] = {
                0xBB0000FF, // red
                0xBB00FF00, // green
                0xBBFF0000, // blue
                0xBBFFFFFF, // white for alpha?
            };

            PushID(i);
            value_changed |= DragFloat("##v", &v[i], v_speed, v_min, v_max, display_format);

            const ImVec2 min        = GetItemRectMin();
            const ImVec2 max        = GetItemRectMax();
            const float spacing     = g.Style.FrameRounding;
            const float halfSpacing = spacing / 2;

            // This is the main change
            window->DrawList->AddLine({ min.x + spacing, max.y - halfSpacing }, { max.x - spacing, max.y - halfSpacing }, colours[i], 4);

            SameLine(0, g.Style.ItemInnerSpacing.x);
            PopID();
            PopItemWidth();
        }
        PopID();

        TextUnformatted(label, FindRenderedTextEnd(label));
        EndGroup();

        return value_changed;
    }

    void PushMultiItemsWidthsAndLabels(const char* labels[], int components, float w_full)
    {
        ImGuiWindow* window     = GetCurrentWindow();
        const ImGuiStyle& style = GImGui->Style;
        if(w_full <= 0.0f)
            w_full = GetContentRegionAvail().x;

        const float w_item_one = ImMax(1.0f, (w_full - (style.ItemInnerSpacing.x * 2.0f) * (components - 1)) / (float)components) - style.ItemInnerSpacing.x;
        for(int i = 0; i < components; i++)
            window->DC.ItemWidthStack.push_back(w_item_one - CalcTextSize(labels[i]).x);
        window->DC.ItemWidth = window->DC.ItemWidthStack.back();
    }

    bool DragFloatNEx(const char* labels[], float* v, int components, float v_speed, float v_min, float v_max,
                      const char* display_format)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if(window->SkipItems)
            return false;

        ImGuiContext& g    = *GImGui;
        bool value_changed = false;
        BeginGroup();

        PushMultiItemsWidthsAndLabels(labels, components, 0.0f);
        for(int i = 0; i < components; i++)
        {
            PushID(labels[i]);
            PushID(i);
            TextUnformatted(labels[i], FindRenderedTextEnd(labels[i]));
            SameLine();
            value_changed |= DragFloat("", &v[i], v_speed, v_min, v_max, display_format);
            SameLine(0, g.Style.ItemInnerSpacing.x);
            PopID();
            PopID();
            PopItemWidth();
        }

        EndGroup();

        return value_changed;
    }

    bool DragFloat3Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        INDEX_PROFILE_FUNCTION();
        return DragFloatN_Coloured(label, v, 3, v_speed, v_min, v_max);
    }

    bool DragFloat4Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        INDEX_PROFILE_FUNCTION();
        return DragFloatN_Coloured(label, v, 4, v_speed, v_min, v_max);
    }

    bool DragFloat2Coloured(const char* label, float* v, float v_speed, float v_min, float v_max)
    {
        INDEX_PROFILE_FUNCTION();
        return DragFloatN_Coloured(label, v, 2, v_speed, v_min, v_max);
    }
}
