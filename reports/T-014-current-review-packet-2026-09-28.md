# T-014 protected contract review packet

Original packet: 2026-09-28. Refreshed: 2026-11-18.
Status: **current candidate pending independent review and human code-owner approval**.
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
verifier passed locally against SDK 10.0.26100.0. This proves the direct
locked-file fixture and the clean verifier path. It does **not** prove that
the verifier and `scripts/build.ps1` exit nonzero when a selected file is
locked during their own scans. That propagation test, an independent review
of these exact hashes, and human code-owner approval remain open. Earlier
Debug/Release build results and hosted CI findings refer to older candidates;
rerun the relevant build gates for this candidate before approval.

The reviewer should also assess enumeration and read failures, literal-token
coverage for ADR-001, whether the SQLite tokens and selected artifact types
cover ADR-002, production/negative-guard parity, and any regression in the
SDK/header/IID/method-order checks. Return a disposition of `APPROVE`,
`APPROVE_WITH_REQUIRED_CHANGES`, or `BLOCKED` for these exact hashes.

No agent may edit the four protected paths. This packet concerns the narrow
contract-integrity/bootstrap-lifetime T-014 scope; it does not approve
T-015/T-016/T-019 protocol work, plugin registration, or real credentials.
