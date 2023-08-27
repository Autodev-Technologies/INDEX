#include "Precompiled.h"
#include "VertexBuffer.h"

namespace Index
{
    namespace Graphics
    {
        VertexBuffer* (*VertexBuffer::CreateFunc)(const BufferUsage&) = nullptr;

        VertexBuffer* VertexBuffer::Create(const BufferUsage& usage)
        {
            INDEX_ASSERT(CreateFunc, "No VertexBuffer Create Function");

            return CreateFunc(usage);
        }
    }
}
