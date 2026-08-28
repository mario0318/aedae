# aeDae

aeDae is a Windows 11 personal authenticator from R3 Labs. This repository currently contains the bootstrap milestone only: a native COM activation skeleton, a management executable, package layout, generated product identifiers, and build checks.

It does not create, store, export, or sign with credentials. Plugin registration is intentionally disabled until the required Windows WebAuthn Plugin SDK contract is installed and security-reviewed.

## Build

Run `scripts/build.ps1`. The build requires Visual Studio 2022 Build Tools with the C++ workload and a Windows 11 SDK. The package script additionally requires `MakeAppx.exe` from Windows SDK 10.0.26100.7175 or later.

## Security boundary

The next milestone must map the official Microsoft Passkey Manager sample before implementing the WebAuthn plugin interface. Do not add credential code until the vault, DPAPI protection, and Windows Hello verification tasks are reviewed under `security.md`.

