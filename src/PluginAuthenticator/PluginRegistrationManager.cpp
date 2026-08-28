#include "PluginRegistrationManager.h"

namespace aedae
{
namespace
{
bool HasPluginApi(const char* name)
{
    HMODULE webauthn = LoadLibraryExW(L"webauthn.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (webauthn == nullptr) return false;
    const bool present = GetProcAddress(webauthn, name) != nullptr;
    FreeLibrary(webauthn);
    return present;
}
}

PluginApiAvailability PluginRegistrationManager::GetAvailability() const
{
    return HasPluginApi("WebAuthNPluginAddAuthenticator") && HasPluginApi("WebAuthNPluginRemoveAuthenticator")
        ? PluginApiAvailability::available
        : PluginApiAvailability::unavailable;
}

HRESULT PluginRegistrationManager::Register() const
{
    // The official webauthnplugin.h types are intentionally not recreated here.
    // This prevents accidental registration with an unreviewed ABI contract.
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

HRESULT PluginRegistrationManager::Unregister() const
{
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}
}

