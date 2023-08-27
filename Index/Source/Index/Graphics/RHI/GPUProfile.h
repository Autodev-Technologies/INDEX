#pragma once
#include "Graphics/RHI/GraphicsContext.h"

#if defined(INDEX_RENDER_API_VULKAN)
#include "Platform/Vulkan/VKDevice.h"
#include "Platform/Vulkan/VKCommandBuffer.h"
#endif

#if defined(INDEX_PROFILE_GPU_ENABLED) && defined(INDEX_RENDER_API_VULKAN) && defined(INDEX_PROFILE) && defined(TRACY_ENABLE)
#define INDEX_PROFILE_GPU(name) TracyVkZone(Index::Graphics::VKDevice::Get().GetTracyContext(), static_cast<Index::Graphics::VKCommandBuffer*>(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer())->GetHandle(), name)
#else
#define INDEX_PROFILE_GPU(name)
#endif
