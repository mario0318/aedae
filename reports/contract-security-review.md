# T-014 Contract security review

Reviewed: 2026-08-27  
Status: IN REVIEW - protocol and registration implementation remain blocked

## Contract baseline

- Reviewed SDK: `10.0.26100.0`.
- Reviewed headers: `webauthnplugin.h` SHA-256 `8B8897A5FE7D4575B5DE8287C7F0E79CED3D96CAF6D273BC7E473E225AC873B8`; `pluginauthenticator.h` SHA-256 `3B5C60E4972AA9FCD3BD6499335F5BE241A757DD628E231FF0AE7ADBC7F859E7`.
- `IPluginAuthenticator` inherits `IUnknown` and requires `MakeCredential`, `GetAssertion`, `CancelOperation`, and `GetLockStatus`.
- The v2 APIs are explicitly named `EXPERIMENTAL_WebAuthNPluginAddAuthenticator2` and `EXPERIMENTAL_WebAuthNPluginPerformUserVerification2`.

Experimental declarations are not approved for product use. A future missing or renamed symbol must stop the build. There is no fallback to an unprefixed or older API without a new contract review.

## Findings

### HIGH - operation request signatures must fail closed before all further processing

The operation request carries a signature and the platform provides an operation-signing public-key API. The Microsoft Passkey Manager sample calculates signature verification status but continues to `PerformUserVerification` without requiring the result to succeed. This must not be adopted.

Required remediation before any registration or protocol task:

1. Retrieve the platform operation-signing key using the official API.
2. Reject absent, malformed, or invalid signatures before decoding CBOR, displaying UI, invoking Hello, touching vault state, selecting a credential, or producing a response.
3. Authenticate cancellation requests under the same rule.
4. Add negative tests that prove no UI, Hello prompt, key access, or response occurs after verification failure.

### HIGH - cancellation and lock state require a concurrency design

The official interface requires `CancelOperation` and `GetLockStatus`; its headers do not supply the project’s transaction-state or threading policy. Before broker activation, define a transaction-bound state machine with an atomic terminal canceled state, no response after cancellation wins, and a conservative locked-on-error status. Cover concurrent cancel, completion, and shutdown in integration tests.

### MEDIUM - the header check is presence-only, not a pinned ABI integrity gate

Remediation implemented, pending security review: the verifier now pins SDK `10.0.26100.0`, compares both approved header hashes, verifies the stable interface IID and method order, and rejects unprefixed v2 declarations. `WebAuthnContractAssertions.cpp` adds x64 compile-time assertions for selected stable registration structure sizes and offsets. Future SDK re-pinning requires a new security review.

### MEDIUM - v2 experimental APIs carry ABI and buffer-binding risk

The v2 declarations change CLSID and transaction-ID pointer conventions. Its user-verification request additionally accepts a caller-provided buffer to sign without hashing. Do not use v2 until an explicit architecture and security decision defines algorithm, exact data binding, buffer lifetime, downgrade behavior, and supported Windows build policy.

### MEDIUM - COM unload accounting is incomplete

Remediation implemented for the bootstrap, pending security review: module-level live-object and server-lock accounting make `DllCanUnloadNow` return `S_FALSE` while a class factory, authenticator instance, or server lock is retained. The activation harness verifies the unload transitions. Future operation lifetime accounting remains a T-016 requirement.

## Required security invariants for the next implementation task

- Every assertion requires fresh successful Windows Hello verification. Cancellation, timeout, ambiguity, or failure yields no signature or response.
- Private keys remain local-only and protected at rest as required by `security.md`; this review approves no temporary plaintext-key exception.
- RP ID validation, request authentication, and operation cancellation all fail closed.
- Logs contain neither credential data, challenges, signatures, raw key material, nor unnecessary identifiers.

## Evidence reviewed

- `security.md`, `functional.md`, `architecture.md`, and the bootstrap sources.
- Windows SDK `webauthnplugin.h` and `pluginauthenticator.h` at the locked paths in `reports/webauthnplugin-contract.md`.
- Microsoft Passkey Manager sample registration and authenticator implementation, especially the request-signature, user-verification, cancellation, and lock-state paths.

## Disposition

No protocol or provider-registration implementation is approved. Close the two HIGH findings and the contract-integrity MEDIUM finding with design, implementation, and negative-test evidence before moving T-004 or any `MakeCredential`/`GetAssertion` task to implementation.
