# T-018 immutable SDK source investigation

Date: 2026-09-28  
Status: negative candidate checks; no SDK installation, CI edit, or manifest repin

## Question

Can the red `windows-2025` CI jobs consume a Microsoft-published, immutable
`10.0.26100.0` payload with the three header bytes pinned by
`reports/webauthnplugin-abi-manifest.json`?

## Candidate checks

| Source | Integrity check | Plugin-header result |
| --- | --- | --- |
| Microsoft-owned NuGet `Microsoft.Windows.SDK.CPP` 10.0.26100.1882 | Downloaded 155,784,059-byte package; SHA-512 matched NuGet blob metadata `GDH0VYGL4DKgJwgvx7gYkHsRjPmHwIMV3iOG6k4XZwz4hjNh2GFP85uv8VUbwwaceU1UlvyRdAxhAHdGELYdRw==` | Contains `webauthn.h` but not `webauthnplugin.h` or `pluginauthenticator.h`. |
| Microsoft-owned NuGet `Microsoft.Windows.SDK.CPP` 10.0.26100.7627 | Downloaded 160,365,066-byte package; SHA-512 matched NuGet blob metadata `33q5l4JXH1jcFCdO5kFBvw0CX+TsZlzmVUxaYXLlosbXH1hIWP+oZ7HVzCxuDDycpAn1o2wPTJ9XgylwJDK+TQ==` | Also lacks both plugin headers. |
| Public `microsoft/webauthn` commit `c65eccdf929112c884e8529398cee8979d375736` (2026-01-09) | Downloaded its three header files from Microsoft's repository and compared SHA-256 to the protected manifest, including a CRLF-normalized comparison | None of the three match. The local `pluginauthenticator.h` includes additional experimental interface declarations absent from that commit. |

The NuGet packages are legitimate and versioned but **not complete ABI inputs**
for this project. The public GitHub commit is likewise not the exact installed
header set. Do not copy or overlay any of these into CI merely to make it green.

## Remaining acceptable paths

1. Acquire a Microsoft Windows SDK ISO/installer payload that actually includes
   all three matching headers. First verify the payload hash and extract only
   into an ephemeral, non-system location; verify each pinned header hash before
   configuring MSBuild to consume it. This requires a separate implementation
   and security review. No installer was run here.
2. Use a secured self-hosted runner with the exact already-reviewed SDK bytes,
   with runner integrity/availability and secret-exposure risks reviewed first.
3. Review a newer official contract and have the human owner author a protected
   manifest change. This is a contract re-pin, not a CI workaround.

Until one path is approved and independently green, PR #3 remains correctly
red. This finding does not resolve T-019's signing envelope or authorize the
system-wide plugin protocol operations.

The downloaded comparison payloads remain in ignored
`artifacts/sdk-candidate/` because the local execution policy denied their
targeted removal. They are scratch evidence only, not build inputs or tracked
source; no installer was executed.
