#include <iostream>
#include "MockThisPcService.h"
#include "../PluginAuthenticator/PluginRegistrationManager.h"
#include "../Common/Identifiers.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && std::wstring(argv[1]) == L"--mock-status")
    {
        const aedae::management::MockThisPcStatusService service;
        std::wcout << aedae::management::RenderMockStatus(service.GetSnapshot());
        return 0;
    }
    aedae::PluginRegistrationManager registration;
    const bool api_available = registration.GetAvailability() == aedae::PluginApiAvailability::available;
    std::wcout << aedae::kProductName << L" bootstrap\n"
               << L"AAGUID: " << aedae::kAaguid << L"\n"
               << L"WebAuthn Plugin API: " << (api_available ? L"present" : L"unavailable") << L"\n"
               << L"Credential operations: disabled pending security-reviewed implementation\n";
    return 0;
}
