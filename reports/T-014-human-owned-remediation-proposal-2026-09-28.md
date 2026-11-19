# T-014 protected-path remediation proposal for the human code owner

Date: 2026-09-28  
Status: historical proposal; code-owner sign-off is recorded, while the
current candidate remains in review.
Update (2026-11-18): commit `8ddf603` applied `-ErrorAction Stop` at both scan
sites and added the locked-file negative fixture. The guard test passes. The
current hashes are in `T-014-current-review-packet-2026-09-28.md`.
`tests/contract-locked-propagation.ps1` now proves that both the verifier and
build wrapper exit 1 on a selected locked file. Independent security review
remains open. The code owner recorded approval on PR #7 on 2026-11-18; GitHub rejects
a formal Approve review from the PR author.

## Required fail-closed correction

The 2026-09-28 candidate used `-ErrorAction SilentlyContinue` at both
`Select-String` calls inside `Invoke-AeDaeSourceCheck`. Commit `8ddf603`
replaced both with `-ErrorAction Stop`, preserving the prohibited-token checks
and diagnostics. The earlier suppressed read error was confirmed with a
missing-file probe; it is not a finding against the current candidate.

The locked-file negative fixture now in `scripts/test-webauthnplugin-contract-guard.ps1`
implements steps 1 to 3 below; the new propagation test covers step 4:

1. Create a disposable source fixture under a disposable test root.
2. Hold the selected file open with `FileShare.None` while calling the same
   production `Invoke-AeDaeSourceCheck` function.
3. Require a terminating read failure, not a clean return; release the handle
   in `finally` and remove the disposable fixture.
4. Confirm `scripts/verify-webauthnplugin-contract.ps1` and
   `scripts/build.ps1` propagate that failure to a nonzero process exit.

The remaining protected-file review should also decide whether the four SQLite literal
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
