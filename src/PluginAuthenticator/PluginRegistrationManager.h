#pragma once

#include <windows.h>

namespace aedae
{
enum class PluginApiAvailability { unavailable, available };

class PluginRegistrationManager final
{
public:
    PluginApiAvailability GetAvailability() const;
    HRESULT Register() const;
    HRESULT Unregister() const;
};
}

