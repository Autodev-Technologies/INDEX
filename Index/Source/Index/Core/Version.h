#pragma once

namespace Index
{
    struct Version
    {
        double major = 1.;
        double minor = 0.;
        int patch = 0;
    };

    constexpr Version const IndexVersion = Version();
}
