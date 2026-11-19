# T-014 current protected-diff review packet

Date: 2026-09-11

Disposition: **PENDING INDEPENDENT SECURITY REVIEW AND HUMAN APPROVAL**

## Exact candidate

Review the working-tree versions of these four human-protected contract files against
`origin/main` at `254cdf6e`:

| File | Current SHA-256 | Difference from `origin/main` |
| --- | --- | --- |
| `reports/webauthnplugin-abi-manifest.json` | `78859F16F2CFCF92E21452CFCE1ED837E1F91C6F14A3C667DC678944BBEFEC0E` | unchanged |
| `scripts/ContractCheck.psm1` | `9548BB061D5D8715CC036944E4DE64B2F60EBB04329BC6A6B802D81795B44FED` | 26 insertions, 1 replacement |
| `scripts/verify-webauthnplugin-contract.ps1` | `63392F4F1D9EF36A07847BB632B95464B9D67D92C9206540E156CD6517F9A0F0` | 1 replacement |
| `scripts/test-webauthnplugin-contract-guard.ps1` | `7E2FA898B6F90CA25E668E80EFEDD1622C35866B894F89C02092053D99741BAB` | 3 insertions |

Reproduce the candidate diff with:

```powershell
git diff --no-ext-diff --unified=5 origin/main -- `
  reports/webauthnplugin-abi-manifest.json `
  scripts/ContractCheck.psm1 `
  scripts/verify-webauthnplugin-contract.ps1 `
  scripts/test-webauthnplugin-contract-guard.ps1
```

Do not review a later working-tree state under these hashes. Do not edit these files from an agent
role; `AGENTS.md` and CODEOWNERS reserve them for the human owner.

## Intended behavior

The candidate adds `Invoke-AeDaeSourceCheck` and invokes it from both the normal contract verifier
and negative guard. It scans all files below `src/`, plus `*.map`, `*.vcxproj`, `*.props` and `*.def`
below `build/` and `artifacts/`, for literal `EXPERIMENTAL_` references. It also rejects references
to four SQLite/amalgamation tokens because ADR-002 selected the single-file store and did not approve
SQLite. The negative guard creates one synthetic fixture for each prohibition.

The ABI manifest, pinned SDK version, header hashes, stable interface IID and method-order checks are
unchanged. The candidate does not implement MED-02's distinct stabilized-symbol diagnostic. MED-04
is an unprotected build-wrapper change in PR #3 and is outside this exact protected diff.

## Verification already performed

- `scripts/verify-webauthnplugin-contract.ps1`: pass against installed SDK 10.0.26100.0.
- `scripts/test-webauthnplugin-contract-guard.ps1`: all header-mutation, project-SDK,
  `EXPERIMENTAL_` and SQLite negative fixtures pass.
- Complete Debug and Release `scripts/build.ps1` gates: pass; both builds report zero warnings and
  zero errors, followed by passing COM, vault, management-model and random-buffer key-protection
  suites.
- PR #3 hosted CI is not approval evidence for this candidate: it deliberately excludes the
  protected working-tree changes and currently fails closed on unrelated hosted-SDK header drift.

## Required reviewer questions

1. Does enumeration and file reading fail closed for every selected source/build artifact, including
   inaccessible, malformed or concurrently changed files? Pay particular attention to the candidate's
   `-ErrorAction SilentlyContinue` on `Select-String`.
2. Is literal-token scanning sufficient for ADR-001's compile-time prohibition, or must compiled
   import/symbol artifacts receive a separate verifier?
3. Is adding the SQLite token prohibition to this protected contract module approved scope, and do
   the four tokens cover every dependency reference that ADR-002 intends to prohibit without
   unacceptable false positives?
4. Do the verifier and negative guard invoke the same production function in a way that proves a
   failure stops the build wrapper?
5. Does any change weaken or bypass the existing SDK version, header hash, IID, method-order or
   unprefixed-experimental-symbol checks?

Required output: severity, affected lines, exploit or failure condition, required remediation and a
reproducible verification step. The reviewer must explicitly return `APPROVE`,
`APPROVE_WITH_REQUIRED_CHANGES` or `BLOCKED` for these exact hashes.

## Remaining gate

The older `reports/contract-security-review.md` describes a broader historical block and is stale
relative to the later task split. The reviewer must reconcile it with the narrow T-014 scope recorded
in `tasks.md` without treating T-015/T-016 protocol work as T-014 implementation. After a clean
independent disposition, the human owner must approve the exact protected-file diff and limited
contract-integrity/bootstrap-lifetime scope before T-014 can be `DONE`.
