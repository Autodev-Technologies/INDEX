#include "Precompiled.h"
#include "ModelComponent.h"
#include "Core/Application.h"

namespace Index::Graphics
{
    ModelComponent::ModelComponent(const std::string& path)
    {
        LoadFromLibrary(path);
    }

    void ModelComponent::LoadFromLibrary(const std::string& path)
    {
        ModelRef = Application::Get().GetModelLibrary()->GetResource(path);
    }

}
