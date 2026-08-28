# Windows Personal Authenticator – Architecture

## 1. Design principles

- Use Windows WebAuthn Plugin APIs as the integration boundary.
- Keep the plugin thin, deterministic, and auditable.
- Keep credential storage and key protection behind narrow internal interfaces.
- Use Windows Hello for user verification and Windows-supported protection for at-rest secrets.
- Keep v1 local-first and offline-capable after installation.

## 2. Repository layout

```text
src/
  PluginAuthenticator/   WebAuthn plugin DLL and COM activation
  Vault/                 Encrypted credential store and key protection
  App.UI/                WinUI management dashboard
  Common/                Logging, configuration, and WebAuthn wrappers
tests/
  unit/                  Vault and key-protection tests
  integration/           Controlled registration and assertion tests
spec/                    Product, security, architecture, and task contracts
reports/                 Generated review and release reports
scripts/                 Agent entrypoints and developer helpers
external/                Pinned, reviewed upstream dependencies
```

## 3. Components

### PluginAuthenticator

Responsibilities:

- Implement COM activation and `IPluginAuthenticator`.
- Handle `MakeCredential`, `GetAssertion`, `CancelOperation`, and `GetLockStatus`.
- Register and unregister the authenticator through the Windows WebAuthn Plugin APIs.
- Validate incoming requests, call Windows Hello verification, delegate vault access, and construct protocol responses.

This component does not own database access details, UI rendering, sync, analytics, or broad system-management work.

### Vault

Responsibilities:

- Store encrypted `CredentialRecord` entities.
- Provide transactional create, query, list, update-last-used, and deletion operations.
- Bind encryption/decryption to the security policy through `KeyProtection`.
- Return only the minimum data required by callers.

### KeyProtection

Responsibilities:

- Encrypt and decrypt credential key material using DPAPI-backed protection.
- Use hardware-backed or TPM-associated protection where Windows supports it and the feature is reliably available.
- Provide a small API with no opportunity for callers to persist plaintext key material.

### App.UI

Responsibilities:

- Show the status of THIS PC as an authenticator.
- List locally managed credentials.
- Show an Identity Health dashboard derived from allowed metadata.
- Enable credential deletion and provider enable/disable actions with clear confirmation.

The UI never signs assertions directly and never receives raw private keys.

### Common

Responsibilities:

- Redacted logging.
- Feature detection for Windows build/API availability.
- Configuration defaults.
- Thin wrappers around WebAuthn APIs that make error handling consistent.

### Operation lifecycle and request authentication

Before any WebAuthn payload is decoded, an `OperationCoordinator` owns the transaction and an operation-signature verifier authenticates the raw platform request. The coordinator has one atomic terminal transition: completed, failed, or canceled. Cancellation is authenticated and transaction-bound; a terminal cancellation prevents every later side effect and response publication. `GetLockStatus` remains conservative and does not authorize work.

The detailed design is maintained in `reports/operation-signature-gate-design.md` and `reports/operation-lifecycle-design.md`. Experimental v2 plugin APIs are prohibited until a separate architecture and security decision approves them.

## 4. Key interfaces

```text
IVaultStore
  AddCredential(record)
  GetCredentialById(rpId, credentialId)
  FindCredentialsForRp(rpId)
  ListCredentials()
  UpdateLastUsed(credentialId)
  DeleteCredential(credentialId)

IKeyProtection
  ProtectPrivateKey(plaintextKey) -> protectedBlob
  UnprotectPrivateKey(protectedBlob, verificationContext) -> transient plaintextKey

IAuthenticatorService
  CreateCredential(request) -> attestation response
  GetAssertion(request) -> assertion response
```

## 5. Simplified data model

```text
CredentialRecord
  id
  rpId
  accountLabel
  userHandleEncryptedOrMinimized
  credentialId
  publicKeyCose
  protectedPrivateKey
  createdAt
  lastUsedAt
  userVerificationPolicy
  originSource
  backupStatus
  schemaVersion
```

## 6. Process boundaries

- The plugin DLL calls the vault only through `IVaultStore`.
- The vault invokes key protection only through `IKeyProtection`.
- The UI uses a safe read/manage service and cannot request unprotected private keys.
- All signing routes through the plugin’s assertion path after Windows Hello verification.
- All protocol entry points require authenticated platform operation requests before decoding, UI, Hello, vault access, metadata change, or response construction.

## 7. Technology baseline

- Plugin/vault core: C++/WinRT or native C++ because the plugin and COM boundary are native Windows concerns.
- UI: WinUI 3; C# is acceptable for the UI if the native boundary remains isolated.
- Storage: SQLite or another embedded transactional database, subject to a dependency/security review.
- Packaging: MSIX.
- CI: build, static analysis, unit tests, integration checks, dependency pin verification, and package signing checks.
