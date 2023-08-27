#pragma once

#include "Graphics/RHI/GraphicsContext.h"

namespace Index
{
    namespace Graphics
    {
        class INDEX_EXPORT GLContext : public GraphicsContext
        {
        public:
            GLContext();
            ~GLContext();

            void Present() override;
            void Init() override {};

            size_t GetMinUniformBufferOffsetAlignment() const override { return 256; }

            bool FlipImGUITexture() const override { return true; }

            float GetGPUMemoryUsed() override { return 0.0f; }
            float GetTotalGPUMemory() override { return 0.0f; }

            void WaitIdle() const override { }

            void OnImGui() override;
            static void MakeDefault();

        protected:
            static GraphicsContext* CreateFuncGL();
        };
    }
}
