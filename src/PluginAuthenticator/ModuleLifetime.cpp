#include "ModuleLifetime.h"

namespace
{
volatile LONG g_live_object_count = 0;
volatile LONG g_server_lock_count = 0;
}

namespace aedae
{
void AddModuleObjectRef()
{
    InterlockedIncrement(&g_live_object_count);
}

void ReleaseModuleObjectRef()
{
    InterlockedDecrement(&g_live_object_count);
}

void AddServerLock()
{
    InterlockedIncrement(&g_server_lock_count);
}

bool ReleaseServerLock()
{
    LONG current = InterlockedCompareExchange(&g_server_lock_count, 0, 0);
    while (current > 0)
    {
        const LONG observed = InterlockedCompareExchange(&g_server_lock_count, current - 1, current);
        if (observed == current) return true;
        current = observed;
    }
    return false;
}

bool CanUnloadModule()
{
    return InterlockedCompareExchange(&g_live_object_count, 0, 0) == 0 &&
           InterlockedCompareExchange(&g_server_lock_count, 0, 0) == 0;
}
}
