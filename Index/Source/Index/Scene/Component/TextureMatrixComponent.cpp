#include "Precompiled.h"
#include "TextureMatrixComponent.h"

namespace Index
{
    TextureMatrixComponent::TextureMatrixComponent(const glm::mat4& matrix)
        : m_TextureMatrix(matrix)
    {
    }

    void TextureMatrixComponent::OnImGui()
    {
    }
}
