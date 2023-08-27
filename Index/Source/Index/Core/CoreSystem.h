#pragma once

#include "Core/Core.h"

namespace Index
{
    namespace Internal
    {
        // Low-level System operations
        class INDEX_EXPORT CoreSystem
        {
        public:
            static bool Init(int argc = 0, char** argv = nullptr);
            static void Shutdown();
        };

    }

};
