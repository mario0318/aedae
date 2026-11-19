# Microsoft Passkey Manager sample mapping

Status: repair candidate for T-002 architecture rereview

## Scope and safety boundary

This ledger records which roles and patterns observed in Microsoft's Passkey Manager sample are
adopted, adapted, or intentionally excluded. No sample source is copied into product code. A sample
observation is not a Windows contract, and nothing here authorizes registration, request decoding,
Windows Hello, credential operations, signing, cancellation handling, or an experimental API.

The protocol lane remains paused on T-019/T-015. In particular, the request-signature and
user-verification constructions described in
`reports/T-019-operation-signing-envelope-finding.md` are research observations only.

## Restored T-002 acceptance criteria

T-002 is reviewable only if this ledger identifies all six areas from the original task:

1. plugin activation;
2. plugin registration;
3. authenticator operations;
4. cancellation;
5. concrete test points; and
6. the SDK/API-version gate.

Each row below also states whether the sample pattern is adopted, adapted, or excluded. A planned
test is not presented as executable evidence.

## Pattern ledger

| Area | Sample observation | aeDae decision | Current implementation boundary | Test point and present evidence |
| --- | --- | --- | --- | --- |
| COM activation and lifetime | The sample is a COM plugin DLL implementing the platform authenticator surface. | **ADAPTED.** Keep ordinary class-factory, reference-count, server-lock, and unload-accounting patterns; do not copy the operation implementation. | The bootstrap currently exposes only `IUnknown`. It is not an `IPluginAuthenticator` implementation and must not be described as one. T-003 owns the unresolved bootstrap-scope decision. | **Executable:** `tests/integration/ComActivationHarness.cpp` loads the DLL and checks factory, instance, server-lock, and `DllCanUnloadNow` transitions. It does not cover the missing negative COM cases or the official interface. |
| Registration | The sample registers and removes an authenticator through the WebAuthn plugin API. | **ADAPTED, DISABLED.** Retain feature detection only; defer all state-changing registration calls. | `PluginRegistrationManager::GetAvailability` probes the two stable exports from system `webauthn.dll`. `Register` and `Unregister` return `ERROR_NOT_SUPPORTED`. T-004 remains paused. | **Build coverage only:** the manager is compiled into the app and plugin projects. There is no direct executable registration-manager test, and no registration call is made. |
| MakeCredential and GetAssertion | The sample decodes requests, selects mock credentials, performs user verification, and builds responses. | **EXCLUDED.** Do not copy the sample operation bodies, mock vault, response construction, or ordering. | T-008 and T-009 remain paused. No request decoding, credential selection, authenticator-data construction, signing, or response publication is authorized. | **No executable protocol test exists by design.** Future fail-closed fakes are specified by OL-001 through OL-004 and OL-010 through OL-013 in `tests/integration/OperationLifecycleTestPlan.md`. |
| Operation-request signature | The sample verifies a SHA-256 digest of the raw encoded request using key-blob-driven CNG parameters, but performs decoding and other effects before verification and never gates on the verdict. | **OBSERVATION RETAINED; SECURITY PATTERN EXCLUDED.** The byte construction is research input to T-019, not an adopted contract. The fail-open ordering is forbidden. | T-015 remains paused until Microsoft confirms or rejects the exact signed-message contract. There is no fallback to sample behavior. | **Planned only:** OL-001 and OL-002 require invalid, missing, malformed, oversized, and over-limit requests to cause no decode, UI, Hello, vault, metadata, or response effect. |
| Windows user verification | The sample calls the stable user-verification API and appears to verify a response over the same request buffer, but stores the verdict without gating on it and may skip verification when its mock vault is unlocked. | **MECHANISM OBSERVED; BYPASS EXCLUDED.** Never treat sample behavior as proof of user verification or authorization. | T-007 remains paused with the protocol lane. Every future assertion still requires fresh successful verification and a verified response. | **Planned only:** OL-003, OL-012, and OL-013 cover cancellation, one-time verification consumption, expiry, transaction binding, and pending-callback behavior using fakes. |
| Cancellation | The cancellation request contains a transaction ID and signature field, but the sample does not verify the signature and only compares the transaction ID. | **EXCLUDED.** Do not adopt transaction-ID-only cancellation or the ignored signature. | G2 in T-019 is unresolved because the public contract provides no cancellation message to verify. T-015 must either define an approved envelope or explicitly constrain cancellation as an unauthenticated, non-destructive signal. | **Planned only:** OL-003, OL-005, OL-010, and OL-014 cover side-effect races, mismatch/replay, pre-registration cancellation, and retired IDs. They are design specifications, not passing tests. |
| Lock state | The interface requires a lock-state response, while the sample does not define aeDae's authorization or privacy policy. | **PROJECT POLICY, NOT ADOPTED FROM SAMPLE.** Return a conservative locked state on ambiguity or error; never use it to authorize work. | T-016 owns later implementation after T-015. The approved lifecycle design defines the policy without implementing it. | **Planned only:** OL-006 requires indistinguishable conservative results across nonterminal states. |
| Credential metadata and mock storage | The sample uses mock credential state to demonstrate flows. | **EXCLUDED.** Do not import sample credential records, key handling, persistence, or unlocked-vault behavior. | T-005 and T-006 are separate, human-approved local-lane implementations tested with synthetic records/random buffers only. They are not sample adoption and are not protocol-connected. | Vault and key-protection suites cover their own approved boundaries; they are not evidence for MakeCredential or GetAssertion. |
| Management UI | The sample contains UI for its demonstration scenario. | **EXCLUDED.** aeDae's management experience is independently specified. | T-010/T-011 use simulated local data and explicitly refuse live provenance. | **Executable, unrelated to sample adoption:** `tests/unit/ManagementModelTests.cpp` covers the mock presentation/model boundary. It provides no credential or protocol evidence. |
| SDK and API version | The sample requires Windows SDK 10.0.26100.7175 or later and a supported Windows 11 build. The locked local SDK is `10.0.26100.0` and exposes the stable plugin headers. | **ADAPTED TO A FAIL-CLOSED CONTRACT PIN.** Use only the reviewed stable declarations; never recreate them and never infer semantic guarantees from header presence. Experimental v2 remains prohibited by ADR-001. | The project/headers/hashes and stable interface shape are governed by the human-approval-only ABI manifest and contract scripts. Any repin requires review. | **Executable:** `scripts/verify-webauthnplugin-contract.ps1` and `scripts/test-webauthnplugin-contract-guard.ps1` run before compilation through `scripts/build.ps1`. These protected files are evidence only and are not changed by T-002. |

## Concrete evidence versus future test points

| Test point | Status | What it proves | What it does not prove |
| --- | --- | --- | --- |
| Contract verifier and negative guard | Executable | The selected SDK/header contract and recorded negative fixtures fail closed at build time. | Runtime request semantics, signing envelopes, platform support, or registration success. |
| COM activation harness | Executable | Bootstrap factory/instance/server-lock lifetime accounting for the current `IUnknown` scope. | `IPluginAuthenticator` conformance, operation behavior, cancellation, or registration. |
| Registration manager compilation | Build-only | The feature-probe wrapper compiles and the state-changing methods remain disabled in source. | Runtime export availability on another host or any successful registration. |
| OL-001 through OL-015 | Planned specifications | The required future oracle for request gating, lifecycle, cancellation, lock state, callbacks, and cleanup. | No runtime behavior until an implementation and controlled fakes exist. |
| Management model tests | Executable local-lane evidence | Synthetic UI/model refusal and presentation rules. | Live metadata, Windows Hello, vault connection, or plugin behavior. |

## Architecture consequence

The only sample-derived material eligible for reuse today is ordinary bootstrap structure and
feature-detection shape, both adapted behind fail-closed project gates. Every protocol-semantic
behavior remains excluded or research-only. The sample is useful for locating questions and future
test seams; it is not the security oracle for answering them.
