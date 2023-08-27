#include "Precompiled.h"
#include "VKRenderDevice.h"

namespace Index::Graphics
{
    void VKRenderDevice::Init()
    {
    }

    void VKRenderDevice::MakeDefault()
    {
        CreateFunc = CreateFuncVulkan;
    }

    RenderDevice* VKRenderDevice::CreateFuncVulkan()
    {
        return new VKRenderDevice();
    }
}
