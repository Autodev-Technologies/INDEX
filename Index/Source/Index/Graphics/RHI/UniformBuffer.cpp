#include "Precompiled.h"
#include "UniformBuffer.h"

namespace Index
{
    namespace Graphics
    {
        UniformBuffer* (*UniformBuffer::CreateFunc)()                          = nullptr;
        UniformBuffer* (*UniformBuffer::CreateDataFunc)(uint32_t, const void*) = nullptr;

        UniformBuffer* UniformBuffer::Create()
        {
            INDEX_ASSERT(CreateFunc, "No UniformBuffer Create Function");

            return CreateFunc();
        }

        UniformBuffer* UniformBuffer::Create(uint32_t size, const void* data)
        {
            INDEX_ASSERT(CreateFunc, "No UniformBuffer Create Function");

            return CreateDataFunc(size, data);
        }
    }
}
