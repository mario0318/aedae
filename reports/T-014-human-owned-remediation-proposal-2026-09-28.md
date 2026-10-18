# T-014 protected-path remediation proposal for the human code owner

Date: 2026-09-28  
Status: proposal only; **no protected file edited by an agent**

## Required fail-closed correction

In `scripts/ContractCheck.psm1`, the two `Select-String` calls inside
`Invoke-AeDaeSourceCheck` currently end with `-ErrorAction SilentlyContinue`.
The code owner should replace **both** with `-ErrorAction Stop`, preserving
the existing prohibited-token checks and diagnostics. A suppressed read error
currently yields a false clean result (confirmed with a missing-file probe).

Add a negative test to `scripts/test-webauthnplugin-contract-guard.ps1`:

1. Create a disposable source fixture under a disposable test root.
2. Hold the selected file open with `FileShare.None` while calling the same
   production `Invoke-AeDaeSourceCheck` function.
3. Require a terminating read failure, not a clean return; release the handle
   in `finally` and remove the disposable fixture.
4. Confirm `scripts/verify-webauthnplugin-contract.ps1` and
   `scripts/build.ps1` propagate that failure to a nonzero process exit.

The human-owned amendment should also decide whether the four SQLite literal
tokens cover ADR-002's intended dependency ban and whether generated source
in `build/` or `artifacts/` needs scanning. T-018 MED-02's distinct diagnostic
for stabilized upstream v2 names remains a separate protected-file proposal.

## Gate before merge

Recompute SHA-256 for all four owner-only paths, review the exact resulting
diff independently, run the contract guard and both Debug/Release builds,
and approve the PR through the human code-owner path. Do not transplant the
2026-09-11 hash set or weaken the manifest/SDK check. No operation signing,
request decoding, plugin registration, or real credentials are authorized by
this remediation.
