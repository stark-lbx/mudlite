// clang-format off
#pragma once

#include <unistd.h>

namespace mudlite
{
namespace currthread
{
    extern __thread int t_cachedTid;

    void cacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }
}
} // namespace mudlite