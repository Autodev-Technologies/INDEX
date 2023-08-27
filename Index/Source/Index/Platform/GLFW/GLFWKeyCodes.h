#pragma once

#include "Core/OS/KeyCodes.h"

#include <GLFW/glfw3.h>

namespace Index
{
    namespace GLFWKeyCodes
    {
        static Index::InputCode::Key GLFWToIndexKeyboardKey(uint32_t glfwKey)
        {
            static std::map<uint32_t, Index::InputCode::Key> keyMap = {
                { GLFW_KEY_A, Index::InputCode::Key::A },
                { GLFW_KEY_B, Index::InputCode::Key::B },
                { GLFW_KEY_C, Index::InputCode::Key::C },
                { GLFW_KEY_D, Index::InputCode::Key::D },
                { GLFW_KEY_E, Index::InputCode::Key::E },
                { GLFW_KEY_F, Index::InputCode::Key::F },
                { GLFW_KEY_G, Index::InputCode::Key::G },
                { GLFW_KEY_H, Index::InputCode::Key::H },
                { GLFW_KEY_I, Index::InputCode::Key::I },
                { GLFW_KEY_J, Index::InputCode::Key::J },
                { GLFW_KEY_K, Index::InputCode::Key::K },
                { GLFW_KEY_L, Index::InputCode::Key::L },
                { GLFW_KEY_M, Index::InputCode::Key::M },
                { GLFW_KEY_N, Index::InputCode::Key::N },
                { GLFW_KEY_O, Index::InputCode::Key::O },
                { GLFW_KEY_P, Index::InputCode::Key::P },
                { GLFW_KEY_Q, Index::InputCode::Key::Q },
                { GLFW_KEY_R, Index::InputCode::Key::R },
                { GLFW_KEY_S, Index::InputCode::Key::S },
                { GLFW_KEY_T, Index::InputCode::Key::T },
                { GLFW_KEY_U, Index::InputCode::Key::U },
                { GLFW_KEY_V, Index::InputCode::Key::V },
                { GLFW_KEY_W, Index::InputCode::Key::W },
                { GLFW_KEY_X, Index::InputCode::Key::X },
                { GLFW_KEY_Y, Index::InputCode::Key::Y },
                { GLFW_KEY_Z, Index::InputCode::Key::Z },

                { GLFW_KEY_0, Index::InputCode::Key::D0 },
                { GLFW_KEY_1, Index::InputCode::Key::D1 },
                { GLFW_KEY_2, Index::InputCode::Key::D2 },
                { GLFW_KEY_3, Index::InputCode::Key::D3 },
                { GLFW_KEY_4, Index::InputCode::Key::D4 },
                { GLFW_KEY_5, Index::InputCode::Key::D5 },
                { GLFW_KEY_6, Index::InputCode::Key::D6 },
                { GLFW_KEY_7, Index::InputCode::Key::D7 },
                { GLFW_KEY_8, Index::InputCode::Key::D8 },
                { GLFW_KEY_9, Index::InputCode::Key::D9 },

                { GLFW_KEY_F1, Index::InputCode::Key::F1 },
                { GLFW_KEY_F2, Index::InputCode::Key::F2 },
                { GLFW_KEY_F3, Index::InputCode::Key::F3 },
                { GLFW_KEY_F4, Index::InputCode::Key::F4 },
                { GLFW_KEY_F5, Index::InputCode::Key::F5 },
                { GLFW_KEY_F6, Index::InputCode::Key::F6 },
                { GLFW_KEY_F7, Index::InputCode::Key::F7 },
                { GLFW_KEY_F8, Index::InputCode::Key::F8 },
                { GLFW_KEY_F9, Index::InputCode::Key::F9 },
                { GLFW_KEY_F10, Index::InputCode::Key::F10 },
                { GLFW_KEY_F11, Index::InputCode::Key::F11 },
                { GLFW_KEY_F12, Index::InputCode::Key::F12 },

                { GLFW_KEY_MINUS, Index::InputCode::Key::Minus },
                { GLFW_KEY_DELETE, Index::InputCode::Key::Delete },
                { GLFW_KEY_SPACE, Index::InputCode::Key::Space },
                { GLFW_KEY_LEFT, Index::InputCode::Key::Left },
                { GLFW_KEY_RIGHT, Index::InputCode::Key::Right },
                { GLFW_KEY_UP, Index::InputCode::Key::Up },
                { GLFW_KEY_DOWN, Index::InputCode::Key::Down },
                { GLFW_KEY_LEFT_SHIFT, Index::InputCode::Key::LeftShift },
                { GLFW_KEY_RIGHT_SHIFT, Index::InputCode::Key::RightShift },
                { GLFW_KEY_ESCAPE, Index::InputCode::Key::Escape },
                { GLFW_KEY_KP_ADD, Index::InputCode::Key::A },
                { GLFW_KEY_COMMA, Index::InputCode::Key::Comma },
                { GLFW_KEY_BACKSPACE, Index::InputCode::Key::Backspace },
                { GLFW_KEY_ENTER, Index::InputCode::Key::Enter },
                { GLFW_KEY_LEFT_SUPER, Index::InputCode::Key::LeftSuper },
                { GLFW_KEY_RIGHT_SUPER, Index::InputCode::Key::RightSuper },
                { GLFW_KEY_LEFT_ALT, Index::InputCode::Key::LeftAlt },
                { GLFW_KEY_RIGHT_ALT, Index::InputCode::Key::RightAlt },
                { GLFW_KEY_LEFT_CONTROL, Index::InputCode::Key::LeftControl },
                { GLFW_KEY_RIGHT_CONTROL, Index::InputCode::Key::RightControl },
                { GLFW_KEY_TAB, Index::InputCode::Key::Tab }
            };

            return keyMap[glfwKey];
        }

        static Index::InputCode::MouseKey GLFWToIndexMouseKey(uint32_t glfwKey)
        {

            static std::map<uint32_t, Index::InputCode::MouseKey> keyMap = {
                { GLFW_MOUSE_BUTTON_LEFT, Index::InputCode::MouseKey::ButtonLeft },
                { GLFW_MOUSE_BUTTON_RIGHT, Index::InputCode::MouseKey::ButtonRight },
                { GLFW_MOUSE_BUTTON_MIDDLE, Index::InputCode::MouseKey::ButtonMiddle }
            };

            return keyMap[glfwKey];
        }
    }
}
