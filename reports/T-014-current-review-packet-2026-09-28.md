# T-014 protected contract review packet

Original packet: 2026-09-28. Refreshed: 2026-11-18.
Status: **code-owner sign-off recorded; independent security review pending**.
The 2026-09-28 `APPROVE_WITH_REQUIRED_CHANGES` finding applies only to the
former candidate and its pre-remediation hashes; see the file history and
`reports/security-findings.md` for that review.

## Exact current candidate

PR #8 head `35d6b403a908151eec1a44a7ddecd136ab41f754` contains the protected
changes from parent commit `8ddf6030cf210fb09646bb63b9035541e52633bc`.
These are the SHA-256 values of the four protected files at that head:

| File | SHA-256 |
| --- | --- |
| `reports/webauthnplugin-abi-manifest.json` | `E19DC299190B71A287AA237591F95E1876B9771660075F3F4052F6E682ED49FE` |
| `scripts/ContractCheck.psm1` | `6CDC4522B5739FD4570519F05518C6AA66F9830BA74724509E665E2BC4F5016A` |
| `scripts/verify-webauthnplugin-contract.ps1` | `829B4F0D7AB801127CFC5DC9D7544D19996E5D667C3F3BD0D305292065CDC3DF` |
| `scripts/test-webauthnplugin-contract-guard.ps1` | `718E431841ECAAB1B1F48711F25574ABF2F7D1A81593043C172FFEC65C575CF1` |

Reproduce the protected diff against the historical main baseline:

```powershell
git diff --no-ext-diff --unified=5 ef173ca 35d6b40 -- `
  reports/webauthnplugin-abi-manifest.json `
  scripts/ContractCheck.psm1 `
  scripts/verify-webauthnplugin-contract.ps1 `
  scripts/test-webauthnplugin-contract-guard.ps1
```

The source scanner rejects literal `EXPERIMENTAL_` and four SQLite tokens in
selected source/build files. Both `Select-String` calls now use
`-ErrorAction Stop`. The negative guard holds a selected source file open with
`FileShare.None` and requires the production `Invoke-AeDaeSourceCheck` function
to throw. The pinned SDK, header, IID and method-order checks remain in scope.

## Evidence and remaining gate

On 2026-11-18, the contract guard negative tests and the normal contract
verifier passed locally against SDK 10.0.26100.0. The added
`tests/contract-locked-propagation.ps1` then held a selected source file open
with `FileShare.None` and proved both the verifier and `scripts/build.ps1`
exit 1 on that read failure; it removed the fixture afterward. An independent
security review of these exact hashes remains open. The code owner recorded
approval for PR #7
at `18b6074` in its discussion on 2026-11-18; GitHub rejects a formal Approve
review from the PR author. The COM, VaultStore, ManagementModel, and
KeyProtection suites pass locally. A full Debug solution build reached WinUI
projection but this machine's `cppwinrt.exe` could not find the installed SDK
path in the registry; no registry setting was changed. Earlier hosted CI and
Release build results refer to older candidates.

The reviewer should also assess enumeration and read failures, literal-token
coverage for ADR-001, whether the SQLite tokens and selected artifact types
cover ADR-002, production/negative-guard parity, and any regression in the
SDK/header/IID/method-order checks. Return a disposition of `APPROVE`,
`APPROVE_WITH_REQUIRED_CHANGES`, or `BLOCKED` for these exact hashes.

No agent may edit the four protected paths. The owner's sign-off is not a
security review or a waiver of the remaining gate. This packet concerns the narrow
contract-integrity/bootstrap-lifetime T-014 scope; it does not approve
T-015/T-016/T-019 protocol work, plugin registration, or real credentials.
