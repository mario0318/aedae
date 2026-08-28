# WebAuthn Plugin contract lock

Checked: 2026-08-27  
Status: present locally, implementation still gated on security review

## Installed contract

| Item | Value |
| --- | --- |
| SDK include revision | `10.0.26100.0` |
| WebAuthn dependency header | `C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\webauthn.h` |
| Plugin API header | `C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\webauthnplugin.h` |
| COM interface header | `C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\um\pluginauthenticator.h` |
| `webauthn.h` SHA-256 | `FB575592CE15D8F672386AAB5E6B3D3AAC101F4EC11A8C52C8085CD6D59A9665` |
| `webauthnplugin.h` SHA-256 | `8B8897A5FE7D4575B5DE8287C7F0E79CED3D96CAF6D273BC7E473E225AC873B8` |
| `pluginauthenticator.h` SHA-256 | `3B5C60E4972AA9FCD3BD6499335F5BE241A757DD628E231FF0AE7ADBC7F859E7` |
| `_WIN32_WINNT` default in `sdkddkver.h` | `0x0A00` |
| `NTDDI_VERSION` default without an explicit target | `0x0A000010` |

The bootstrap and future protocol build target the pinned Windows SDK `10.0.26100.0`. Any SDK update must be deliberately re-pinned and security reviewed.

## Required API surface present

- `IPluginAuthenticator` defines `MakeCredential`, `GetAssertion`, `CancelOperation`, and `GetLockStatus`.
- Its pinned IID is `d26bcf6f-b54c-43ff-9f06-d5bf148625f7`; the method order is checked before compilation.
- The x64 build statically checks selected registration-option structure sizes and offsets.
- `WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_OPTIONS` contains the authenticator name, CLSID, plugin RP ID, logo data, CBOR authenticator information, and supported RP list.
- `WebAuthNPluginAddAuthenticator` and `WebAuthNPluginPerformUserVerification` are present.
- Version-two declarations are present with the exact names `EXPERIMENTAL_WebAuthNPluginAddAuthenticator2` and `EXPERIMENTAL_WebAuthNPluginPerformUserVerification2`. They are not declared under the non-experimental names.
- The v2 add options use pointer CLSID fields and add `pwszUserVerificationKeyName`; the v2 verification request uses a pointer transaction GUID and adds a caller-supplied buffer-to-sign.

## Contract guard

`scripts/verify-webauthnplugin-contract.ps1` pins and compares all three required header hashes, checks the stable interface IID and method order, and rejects unprefixed v2 declarations. `scripts/build.ps1` runs it before compilation, so missing or substituted headers stop the build.

## Open security questions

This report locks observed declarations only. The contract security review must decide whether any experimental v2 API is eligible for use, define cancellation and lock-state behavior, and validate operation-signing key handling before protocol implementation.
