# T-006 key-protection implementation and SECURITY review packet

Date: 2026-09-11  
Status: SECURITY approved and human-approved; complete within stated limits  
Authorized data: random synthetic test buffers only

## Scope

T-006 adds a narrow `IKeyProtection` boundary and a Windows current-user implementation. It does
not connect to the vault, plugin, WebAuthn, Windows Hello, registration, signing, or any real
credential material. It does not write files or emit protected or plaintext bytes to logs.

Files in the implementation review:

- `src/Vault/KeyProtection.h`
- `src/Vault/KeyProtection.cpp`
- `tests/unit/KeyProtectionTests.cpp`
- `build/KeyProtectionTests.vcxproj`
- the `KeyProtectionTests` solution/build-wrapper entries

## Construction

Each protection call generates a new 256-bit AES data-encryption key and 96-bit GCM nonce with
Windows CNG's system-preferred RNG. AES-256-GCM encrypts the caller's buffer. Windows DPAPI
`CryptProtectData`, without `CRYPTPROTECT_LOCAL_MACHINE`, wraps the data key for the current user;
both protect and unprotect pass `CRYPTPROTECT_UI_FORBIDDEN` and fixed purpose entropy.

The binary envelope has magic, version, algorithm, wrapped-key length, nonce length, tag length and
ciphertext length, followed by the wrapped key, nonce, tag and ciphertext. Parsing is bounded and
requires the exact total size. The GCM authenticated data is the complete header, complete DPAPI
wrapped-key blob and fixed purpose label. Consequently, mutation of serialized DPAPI metadata is
also rejected even if DPAPI itself tolerates that metadata mutation.

The first exhaustive mutation run exposed exactly that DPAPI-metadata behavior. The implementation
was changed to authenticate the entire wrapped blob as GCM associated data, after which every
one deterministic bit flip at every byte position of the complete envelope failed closed in both
Debug and Release.

## Plaintext lifetime

Raw AES keys, CNG key-object storage, DPAPI clear-key allocations, transient ciphertext working
storage and decrypted plaintext working storage are zeroized by RAII cleanup. Successful unprotect
returns a move-only `SensitiveBuffer` exposing a read-only span and zeroizing its storage on
destruction. Callers can still deliberately copy bytes from that span; this boundary cannot erase
copies created outside it.

## Verification

The focused suite uses random synthetic buffers of 1, 32 and 4096 bytes and checks:

- protect/unprotect round trips;
- protected output differs from plaintext and does not contain plaintext for practical key-sized
  buffers;
- repeated protection of the same input produces different envelopes (this does not independently
  prove that both random values changed);
- empty and over-limit inputs fail;
- wrong purpose fails closed;
- direct DPAPI unprotect with random wrong entropy fails;
- one deterministic bit flip at every envelope byte position fails;
- every truncation and appended trailing data fail.

The serial `scripts/build.ps1` runs passed in Debug and Release, including contract guards, solution
build, COM activation, vault, management-model and key-protection tests. Both solution builds
reported zero warnings and zero errors. A focused Release rebuild with MSVC `/analyze`, `/W4` and
`/WX` also reported zero warnings and zero errors.

Reviewed implementation hashes after the final comment correction:

- `KeyProtection.h`: `5BAFF270B5C621AD03AC9313729B34CBE9AF654D82450C618A456D4D57B70C80`
- `KeyProtection.cpp`: `15A974D9435252804B8FA6D1D5EAE2FE6A7634DE4E5498CF58E4F7CA9C95D802`
- `KeyProtectionTests.cpp`: `E6FBCDD2E4426737BD66096C7D66285D5E91CF53D7C042026CC81DDAFCAA27A9`
- `KeyProtectionTests.vcxproj`: `B874BD94B68D32D7F30C70FF11DD248DECCD5F355F4A206632FAB0E01DB3EF2F`

## Claim limits and remaining gate

This verifies behavior on the current Windows host and current user with random synthetic data. It
does not prove cross-user rejection with a second Windows account, hardware/TPM backing, resistance
to a process already executing as the user, secure deletion of caller-created copies, at-rest vault
integration, power-loss behavior, or any Windows Hello/user-verification property. DPAPI purpose
entropy is domain separation, not an authorization signal.

## Independent SECURITY disposition

The independent read-only review returned `APPROVE` with no HIGH, MEDIUM or LOW blocking findings.
It independently reran the Release `/analyze`, `/W4`, `/WX` build and random-buffer suite and
confirmed the current-user DPAPI flags and absence of a machine-scope fallback. Its two
non-blocking claim-precision notes are incorporated above: the tamper loop is one deterministic bit
flip per position, not all replacements, and distinct envelopes do not independently prove that
both random values changed.

The owner approved the completed T-006 milestone on 2026-09-11 within the exact limits above.
