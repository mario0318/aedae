#include "../../src/App.UI/MockThisPcService.h"
#include "../../src/App.UI/IdentityHealthModel.h"
#include <iostream>
#include <set>
#include <stdexcept>
using namespace aedae::management;
void Check(bool pass, const char* label)
{
    if (!pass) throw std::runtime_error(label);
    std::cout << "PASS " << label << '\n';
}
int main()
{
    try
    {
        const MockThisPcStatusService mock;
        const IThisPcStatusService& service = mock;
        auto snapshot = service.GetSnapshot();
        Check(snapshot.synthetic && !snapshot.registered, "mock service defaults fail closed");
        Check(!snapshot.localCredentialCount && snapshot.hello == Availability::unknown,
            "unknown capability and count are not invented");
        Check(snapshot.keyProtection == Availability::unavailable, "no key protection claim");
        auto rendered = RenderMockStatus(snapshot);
        Check(rendered.find(L"MOCK DATA ONLY") != std::wstring::npos &&
            rendered.find(L"Credential operations: disabled") != std::wstring::npos,
            "mock provenance and disabled operations visible");
        snapshot.localCredentialCount = 0;
        Check(RenderMockStatus(snapshot).find(L"0 (synthetic)") != std::wstring::npos,
            "known zero differs from unknown");
        snapshot.localCredentialCount = 12;
        snapshot.hello = Availability::available;
        Check(RenderMockStatus(snapshot).find(L"12 (synthetic)") != std::wstring::npos &&
            RenderMockStatus(snapshot).find(L"Available (simulated)") != std::wstring::npos,
            "populated scenario remains explicitly simulated");
        snapshot.synthetic = false;
        const auto refusedPresentation = DecideThisPcPresentation(snapshot);
        const auto refusedStatus = RenderMockStatus(snapshot);
        Check(refusedPresentation.showRefusal && !refusedPresentation.showMockProvenance &&
            refusedStatus.find(L"refused") != std::wstring::npos &&
            refusedStatus.find(L"MOCK DATA ONLY") == std::wstring::npos,
            "live data refusal excludes mock provenance");
        Check(AvailabilityLabel(static_cast<Availability>(99)).find(L"Unknown") != std::wstring::npos,
            "unknown state is conservative");

        IdentityHealthInput healthInput;
        healthInput.credentials = {
            {L"synthetic-cred-1", L"example.test", L"synthetic-account-a",
                {{L"aeDae", L"this-pc", true}}},
            {L"synthetic-cred-2", L"example.test", L"synthetic-account-a",
                {{L"external-provider", L"synthetic-device-2", false}}},
            {L"synthetic-cred-3", L"other.test", L"synthetic-account-b",
                {{L"aeDae", L"this-pc", true}}}
        };
        const auto health = ComputeIdentityHealth(healthInput);
        Check(health.status == IdentityHealthStatus::ok && health.summary.has_value(),
            "synthetic identity-health input accepted");
        Check(health.summary->totalCredentials == 3 && health.summary->knownProviders == 2 &&
            health.summary->knownDevices == 2, "identity-health totals derive from source metadata");
        Check(health.summary->visibilityLimit.find(L"may be incomplete") != std::wstring::npos,
            "foreign-provider visibility limit is explicit");

        std::set<std::wstring> localOnlySources;
        bool foundSingle = false;
        bool foundDuplicate = false;
        for (const auto& warning : health.summary->warnings)
        {
            Check(!warning.ruleId.empty() && !warning.sourceCredentialRefs.empty(),
                "every identity-health warning links rule and source data");
            if (warning.kind == IdentityHealthWarningKind::localOnly)
                localOnlySources.insert(warning.sourceCredentialRefs.begin(), warning.sourceCredentialRefs.end());
            foundSingle |= warning.kind == IdentityHealthWarningKind::singleKnownAuthenticator &&
                warning.sourceCredentialRefs == std::vector<std::wstring>{L"synthetic-cred-3"};
            foundDuplicate |= warning.kind == IdentityHealthWarningKind::duplicateCandidate &&
                warning.sourceCredentialRefs ==
                    std::vector<std::wstring>{L"synthetic-cred-1", L"synthetic-cred-2"};
        }
        Check(localOnlySources == std::set<std::wstring>{L"synthetic-cred-1", L"synthetic-cred-3"} &&
            foundSingle && foundDuplicate, "all local-only, single-authenticator, and duplicate rules are computed");

        IdentityHealthInput emptyInput;
        const auto emptyHealth = ComputeIdentityHealth(emptyInput);
        Check(emptyHealth.status == IdentityHealthStatus::ok && emptyHealth.summary->totalCredentials == 0 &&
            emptyHealth.summary->warnings.empty(), "known empty synthetic input remains known zero");

        healthInput.synthetic = false;
        const auto refusedHealth = ComputeIdentityHealth(healthInput);
        Check(refusedHealth.status == IdentityHealthStatus::refusedLiveData && !refusedHealth.summary,
            "live identity-health data is refused");
        Check(!CanPresentIdentityHealthSummary(refusedHealth),
            "live identity-health result cannot be projected");

        healthInput.synthetic = true;
        healthInput.credentials[1].credentialRef = healthInput.credentials[0].credentialRef;
        const auto invalidHealth = ComputeIdentityHealth(healthInput);
        Check(invalidHealth.status == IdentityHealthStatus::invalidSyntheticMetadata && !invalidHealth.summary,
            "ambiguous synthetic source identifiers fail closed");
        Check(!CanPresentIdentityHealthSummary(invalidHealth),
            "invalid identity-health result cannot be projected");

        IdentityHealthInput emptyLocationsInput;
        emptyLocationsInput.credentials = {{L"synthetic-empty-locations", L"example.test", L"account", {}}};
        Check(!ComputeIdentityHealth(emptyLocationsInput).summary,
            "credential without known locations fails closed");

        IdentityHealthInput emptyProviderInput;
        emptyProviderInput.credentials = {{L"synthetic-empty-provider", L"example.test", L"account",
            {{L"", L"device", true}}}};
        Check(!ComputeIdentityHealth(emptyProviderInput).summary,
            "empty provider identifier fails closed");

        IdentityHealthInput emptyDeviceInput;
        emptyDeviceInput.credentials = {{L"synthetic-empty-device", L"example.test", L"account",
            {{L"provider", L"", true}}}};
        Check(!ComputeIdentityHealth(emptyDeviceInput).summary,
            "empty device identifier fails closed");
        std::cout << "ManagementModelTests passed\n";
        return 0;
    }
    catch (const std::exception& error) { std::cerr << "FAIL " << error.what() << '\n'; return 1; }
}
