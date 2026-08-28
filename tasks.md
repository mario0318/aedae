# Windows Personal Authenticator – Task Backlog

## Operating rules

- Every task must be small, testable, and associated with exact files or modules.
- Agents update only files within their authorized ownership boundary.
- No code task is complete without a build/test result or an explicit explanation of why the result cannot run.
- Security-sensitive tasks require Security review before merge.

## Status values

`TODO` | `IN_PROGRESS` | `BLOCKED` | `IN_REVIEW` | `DONE`

## T-001 – Initialize repository and build baseline

- Status: IN_REVIEW
- Owner: ARCHITECT
- Scope: Create the top-level solution, `src/`, `tests/`, `reports/`, `scripts/`, external-reference lock, and packaging layout. Generate unique AAGUID and CLSID.
- Done when: The solution builds a placeholder COM plugin DLL, management executable, and COM activation harness; no credential operation is present.

## T-002 – Import and map Microsoft sample patterns

- Status: IN_REVIEW
- Owner: ARCHITECT
- Scope: Document which Passkey Manager sample components are adopted, adapted, or intentionally excluded.
- Output: `reports/sample-mapping.md`.
- Done when: The architecture identifies plugin activation, registration, authenticator operations, cancellation, test points, and the SDK/API-version gate.

## T-003 – COM plugin skeleton

- Status: IN_REVIEW
- Owner: CODER
- Scope: Implement a loadable COM class implementing the `IPluginAuthenticator` surface with safe nonfunctional stubs.
- Files: `src/PluginAuthenticator/PluginAuthenticatorImpl.*`, `dllmain.cpp`.
- Done when: A harness instantiates the bootstrap COM class; it exposes no credential operation or unreviewed WebAuthn interface.

## T-004 – Plugin registration manager

- Status: BLOCKED
- Owner: CODER
- Scope: Add a wrapper for Windows WebAuthn plugin registration/unregistration and feature detection.
- Files: `src/PluginAuthenticator/PluginRegistrationManager.*`.
- Dependencies: T-014, T-015, T-016.
- Done when: The registration flow is implemented only after the contract review is closed, the request-authentication design is implemented and tested, and the lifecycle design is implemented and tested.

## T-005 – Vault schema and CRUD

- Status: TODO
- Owner: CODER
- Scope: Create the credential database schema and `IVaultStore` implementation.
- Files: `src/Vault/VaultStore.*`, tests.
- Done when: Unit tests cover add, find, list, update-last-used, delete, schema migration, and corruption handling.

## T-006 – DPAPI-backed key protection

- Status: TODO
- Owner: CODER
- Scope: Implement `IKeyProtection` with DPAPI-backed wrapped key storage.
- Files: `src/Vault/KeyProtection.*`, tests.
- Done when: Protected data cannot be read as a private key at rest and wrong-context/tampered data fails closed.
- Review: SECURITY required.

## T-007 – Windows Hello user verification wrapper

- Status: TODO
- Owner: CODER
- Scope: Implement the minimal documented Windows Hello/WebAuthn plugin verification call with cancellation/error normalization.
- Files: `src/Common/WebAuthnHelpers.*`.
- Done when: A controlled test confirms no success result is returned after cancellation or failed verification.
- Review: SECURITY required.

## T-008 – Credential creation path

- Status: TODO
- Owner: CODER
- Scope: Implement `MakeCredential` using approved WebAuthn protocol helpers and the encrypted vault.
- Dependencies: T-005, T-006.
- Done when: A controlled local relying party completes passkey registration.
- Review: SECURITY required.

## T-009 – Assertion path

- Status: TODO
- Owner: CODER
- Scope: Implement `GetAssertion`, including RP ID validation, credential selection, Windows Hello verification, transient key use, signing, and last-used update.
- Dependencies: T-006, T-007, T-008.
- Done when: A controlled local relying party completes a successful authentication and rejected/canceled cases emit no assertion.
- Review: SECURITY required.

## T-010 – Management UI: This PC

- Status: TODO
- Owner: CODER
- Scope: Build the first WinUI page showing provider state, Windows Hello availability, key-protection status, and local credential count.
- Done when: It reads real, nonsecret data from the service layer.

## T-011 – Management UI: Identity Health

- Status: TODO
- Owner: CODER
- Scope: Display totals, local-only credentials, duplicate candidates, and known limitations of foreign-provider visibility.
- Done when: Every displayed warning can link to its computed rule and source data.

## T-012 – Security review gate

- Status: TODO
- Owner: SECURITY
- Scope: Review deltas involving WebAuthn request handling, private-key generation/storage, DPAPI, user verification, logs, and error paths.
- Output: `reports/security-findings.md`.
- Done when: High-severity issues are fixed or explicitly accepted by the human owner.

## T-013 – MSIX package and release checks

- Status: IN_REVIEW
- Owner: PACKAGER
- Scope: Add reproducible MSIX packaging, versioning, release-mode hardening, and a preflight checklist.
- Done when: Package metadata and an unsigned-package script exist; the script fails closed without MakeAppx, final assets, full-trust validation, or an authorized signing certificate.

## T-014 – Security review of the WebAuthn Plugin contract

- Status: IN_REVIEW
- Owner: SECURITY
- Scope: Review the locked official `webauthnplugin.h` and `pluginauthenticator.h` contract with the Microsoft sample. Cover the `IPluginAuthenticator` operation methods, cancellation and lock-state behavior, user-verification calls, operation-signing key handling, and experimental API suitability.
- Dependencies: T-002
- Output: `reports/contract-security-review.md`.
- Done when: `reports/contract-security-review.md` records required remediation and has no unaddressed high-severity finding.

## T-015 – Operation signature-verification gate design

- Status: IN_REVIEW
- Owner: ARCHITECT
- Scope: Define and review the fail-closed order for authenticating platform operation and cancellation requests before all protocol side effects.
- Output: `reports/operation-signature-gate-design.md` and test-plan stubs.
- Dependencies: T-014.
- Done when: A security reviewer approves the exact gate and a future implementation task can bind it to the official signing envelope without fallback behavior.

## T-016 – Transaction lifecycle and lock-state design

- Status: IN_REVIEW
- Owner: ARCHITECT
- Scope: Define an operation state machine, cancellation race semantics, conservative lock-state semantics, COM lifetime ownership, and unload constraints.
- Output: `reports/operation-lifecycle-design.md` and test-plan stubs.
- Dependencies: T-014.
- Done when: A security reviewer approves the state model and the test plan covers cancellation, lock-state, completion, destruction, and unload races.

## Protocol implementation block

T-004, T-007, T-008, and T-009 remain blocked. No task may implement WebAuthn decoding, credential storage, cryptographic signing, Windows Hello invocation, or experimental v2 APIs until T-014 through T-016 are approved and their implementation tasks pass security review.
