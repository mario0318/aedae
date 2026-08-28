#pragma once

#include <windows.h>

namespace aedae
{
void AddModuleObjectRef();
void ReleaseModuleObjectRef();
void AddServerLock();
bool ReleaseServerLock();
bool CanUnloadModule();
}
