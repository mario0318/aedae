#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace aedae::management
{
// Presentation-only fixture states. No vault, plugin, OS capability or key access.
enum class Availability { unknown, available, unavailable };
struct ThisPcSnapshot
{
    bool synthetic{true};
    bool registered{false};
    Availability hello{Availability::unknown};
    Availability keyProtection{Availability::unavailable};
    std::optional<std::uint32_t> localCredentialCount;
};
struct ThisPcPresentationDecision
{
    bool showMockProvenance{false};
    bool showRefusal{true};
};
inline ThisPcPresentationDecision DecideThisPcPresentation(const ThisPcSnapshot& snapshot)
{
    return snapshot.synthetic ? ThisPcPresentationDecision{true, false}
                              : ThisPcPresentationDecision{false, true};
}
class IThisPcStatusService
{
public:
    virtual ~IThisPcStatusService() = default;
    virtual ThisPcSnapshot GetSnapshot() const = 0;
};
class MockThisPcStatusService final : public IThisPcStatusService
{
public:
    ThisPcSnapshot GetSnapshot() const override
    {
        // A known zero is not substituted for an unknown count.
        return {};
    }
};
inline std::wstring AvailabilityLabel(Availability value)
{
    switch (value)
    {
    case Availability::available: return L"Available (simulated)";
    case Availability::unavailable: return L"Unavailable (simulated)";
    default: return L"Unknown (not queried)";
    }
}
inline std::wstring RenderMockStatus(const ThisPcSnapshot& snapshot)
{
    // Reject misrouted live data rather than dressing it as an authorized mock.
    if (DecideThisPcPresentation(snapshot).showRefusal)
        return L"Preview refused: live data is not supported.\n";
    return L"This PC - MOCK DATA ONLY; not a live security assessment\nProvider: " +
        std::wstring(snapshot.registered ? L"Registered (simulated)" : L"Not registered (simulated)") +
        L"\nWindows Hello: " + AvailabilityLabel(snapshot.hello) +
        L"\nKey protection: " + AvailabilityLabel(snapshot.keyProtection) +
        L"\nLocal credentials: " +
        (snapshot.localCredentialCount ? std::to_wstring(*snapshot.localCredentialCount) + L" (synthetic)"
                                       : L"Unknown (not queried)") +
        L"\nCredential operations: disabled; no registration, signing, or key access\n";
}
}
