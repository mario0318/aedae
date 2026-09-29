#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace aedae::management
{
struct AuthenticatorLocation
{
    std::wstring providerId;
    std::wstring deviceId;
    bool thisPc{false};
};

struct IdentityCredentialMetadata
{
    std::wstring credentialRef;
    std::wstring rpId;
    std::wstring accountRef;
    std::vector<AuthenticatorLocation> knownLocations;
};

struct IdentityHealthInput
{
    bool synthetic{true};
    std::vector<IdentityCredentialMetadata> credentials;
};

enum class IdentityHealthStatus { ok, refusedLiveData, invalidSyntheticMetadata };
enum class IdentityHealthWarningKind { localOnly, singleKnownAuthenticator, duplicateCandidate };

struct IdentityHealthWarning
{
    IdentityHealthWarningKind kind;
    std::wstring ruleId;
    std::vector<std::wstring> sourceCredentialRefs;
};

struct IdentityHealthSummary
{
    std::uint32_t totalCredentials{0};
    std::uint32_t knownProviders{0};
    std::uint32_t knownDevices{0};
    std::vector<IdentityHealthWarning> warnings;
    std::wstring visibilityLimit;
};

struct IdentityHealthResult
{
    IdentityHealthStatus status{IdentityHealthStatus::invalidSyntheticMetadata};
    std::optional<IdentityHealthSummary> summary;
};

inline bool CanPresentIdentityHealthSummary(const IdentityHealthResult& result)
{
    return result.status == IdentityHealthStatus::ok && result.summary.has_value();
}

inline IdentityHealthResult ComputeIdentityHealth(const IdentityHealthInput& input)
{
    if (!input.synthetic) return {IdentityHealthStatus::refusedLiveData, std::nullopt};

    using AccountKey = std::pair<std::wstring, std::wstring>;
    std::set<std::wstring> credentialRefs;
    std::set<std::wstring> providers;
    std::set<std::wstring> devices;
    std::map<AccountKey, std::vector<const IdentityCredentialMetadata*>> accounts;
    IdentityHealthSummary summary;

    for (const auto& credential : input.credentials)
    {
        if (credential.credentialRef.empty() || credential.rpId.empty() || credential.accountRef.empty() ||
            credential.knownLocations.empty() || !credentialRefs.insert(credential.credentialRef).second)
        {
            return {IdentityHealthStatus::invalidSyntheticMetadata, std::nullopt};
        }
        for (const auto& location : credential.knownLocations)
        {
            if (location.providerId.empty() || location.deviceId.empty())
                return {IdentityHealthStatus::invalidSyntheticMetadata, std::nullopt};
            providers.insert(location.providerId);
            devices.insert(location.deviceId);
        }
        accounts[{credential.rpId, credential.accountRef}].push_back(&credential);

        const bool localOnly = std::all_of(credential.knownLocations.begin(), credential.knownLocations.end(),
            [](const AuthenticatorLocation& location) { return location.thisPc; });
        if (localOnly)
        {
            summary.warnings.push_back({IdentityHealthWarningKind::localOnly,
                L"IH-LOCAL-ONLY-001", {credential.credentialRef}});
        }
    }

    for (const auto& [account, records] : accounts)
    {
        (void)account;
        std::set<std::pair<std::wstring, std::wstring>> authenticators;
        std::vector<std::wstring> sources;
        for (const auto* record : records)
        {
            sources.push_back(record->credentialRef);
            for (const auto& location : record->knownLocations)
                authenticators.emplace(location.providerId, location.deviceId);
        }
        std::sort(sources.begin(), sources.end());
        if (authenticators.size() == 1)
        {
            summary.warnings.push_back({IdentityHealthWarningKind::singleKnownAuthenticator,
                L"IH-SINGLE-AUTH-001", sources});
        }
        if (records.size() > 1)
        {
            summary.warnings.push_back({IdentityHealthWarningKind::duplicateCandidate,
                L"IH-DUPLICATE-CANDIDATE-001", sources});
        }
    }

    summary.totalCredentials = static_cast<std::uint32_t>(input.credentials.size());
    summary.knownProviders = static_cast<std::uint32_t>(providers.size());
    summary.knownDevices = static_cast<std::uint32_t>(devices.size());
    summary.visibilityLimit = L"Synthetic known metadata only; third-party authenticator visibility may be incomplete.";
    return {IdentityHealthStatus::ok, std::move(summary)};
}
}
