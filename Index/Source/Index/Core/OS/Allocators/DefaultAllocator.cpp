#include "Precompiled.h"
#include "DefaultAllocator.h"

#include "Core/OS/MemoryManager.h"
// #define TRACK_ALLOCATIONS

#define INDEX_MEMORY_ALIGNMENT 16
#define INDEX_ALLOC(size) _aligned_malloc(size, INDEX_MEMORY_ALIGNMENT)
#define INDEX_FREE(block) _aligned_free(block);

namespace Index
{
    void* DefaultAllocator::Malloc(size_t size, const char* file, int line)
    {
#ifdef TRACK_ALLOCATIONS
        INDEX_ASSERT(size < 1024 * 1024 * 1024, "Allocation more than max size");

        Index::MemoryManager::Get()->m_MemoryStats.totalAllocated += size;
        Index::MemoryManager::Get()->m_MemoryStats.currentUsed += size;
        Index::MemoryManager::Get()->m_MemoryStats.totalAllocations++;

        size_t actualSize = size + sizeof(size_t);
        void* mem         = malloc(actualSize + sizeof(void*));

        uint8_t* result = (uint8_t*)mem;
        if(result == NULL)
        {
            INDEX_LOG_ERROR("Aligned malloc failed");
            return NULL;
        }

        memset(result, 0, actualSize);
        memcpy(result, &size, sizeof(size_t));
        result += sizeof(size_t);

        return result;

#else
        return malloc(size);
#endif
    }

    void DefaultAllocator::Free(void* location)
    {
#ifdef TRACK_ALLOCATIONS
        uint8_t* memory = ((uint8_t*)location) - sizeof(size_t);
        if(location && memory)
        {
            uint8_t* memory = ((uint8_t*)location) - sizeof(size_t);
            size_t size     = *(size_t*)memory;
            free(((void**)memory));
            Index::MemoryManager::Get()->m_MemoryStats.totalFreed += size;
            Index::MemoryManager::Get()->m_MemoryStats.currentUsed -= size;
        }
        else
        {
            free(location);
        }

#else
        free(location);
#endif
    }
}
