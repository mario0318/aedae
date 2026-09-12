# T-018 protected contract amendment proposal

Date: 2026-09-11

Status: proposal only; no protected file changed

Human-only paths: `scripts/ContractCheck.psm1`, `scripts/verify-webauthnplugin-contract.ps1`,
`scripts/test-webauthnplugin-contract-guard.ps1`

## Why this is a proposal

T-017 now prohibits every agent role from directly editing the pinned ABI manifest and three
contract scripts. This document supplies the exact review intent for the remaining T-018 findings
without bypassing that rule. A human must author the protected-file commit and approve its pull
request.

## MED-02: distinguish upstream API stabilization

`Invoke-AeDaeHeaderCheck` currently requires the two `EXPERIMENTAL_` v2 spellings before it checks
for forbidden unprefixed spellings. If Microsoft removes the prefix, the first check emits the
ordinary `Missing required WebAuthn Plugin API symbol` message and masks the security-significant
contract transition.

Human-authored amendment:

1. Check `WebAuthNPluginAddAuthenticator2(` and `WebAuthNPluginPerformUserVerification2(` as paired
   prefixed/unprefixed names before the generic required-symbol loop.
2. If an unprefixed spelling exists while its `EXPERIMENTAL_` spelling does not, throw a distinct
   message beginning `SECURITY CONTRACT CHANGE:` and require a new architecture/security review.
3. Retain generic missing-symbol failure when neither spelling exists.
4. In the negative guard, copy the pinned headers to a temporary directory, replace one exact
   `EXPERIMENTAL_` declaration with its unprefixed spelling, call `Invoke-AeDaeHeaderCheck`, and
   require the distinct `SECURITY CONTRACT CHANGE:` diagnostic.

Acceptance evidence must show the stabilized-symbol fixture fails for the new reason and an
ordinary missing-symbol fixture still fails for the old reason.

## MED-04: fail by process exit, not ambient shell state

The committed `scripts/build.ps1` invokes both contract scripts in-process and immediately inspects
`LASTEXITCODE`. Successful PowerShell scripts do not reliably initialize that variable, so a null or
stale value can stop the wrapper before compilation or misreport the result. The unprotected wrapper
amendment in the companion CI pull request runs each gate in a fresh PowerShell process and preserves
its explicit exit code. `tests/unit/Test-BuildWrapper.ps1` substitutes exit-23 fixtures for each gate
and proves compilation never begins and exit 23 is preserved.

No protected-file change is required for this finding.

## MED-05: source-level experimental API prohibition

The current development checkout contains an uncommitted protected-script candidate that:

- adds `Invoke-AeDaeSourceCheck` to scan `src/` plus relevant build/artifact listings for
  `EXPERIMENTAL_` references;
- rejects SQLite dependency references required by ADR-002;
- calls the source check from both verification and negative-guard scripts;
- adds temporary negative fixtures for the experimental API and SQLite cases.

Those changes predate T-017 but were never merged or independently approved. They must not be copied
blindly. The human-authored protected PR should extract only the reviewed MED-05/ADR-002 behavior,
add the MED-02 ordering fix above, and then run a narrow security diff review over the exact three
protected files. The review must verify bounded file selection, no secret-file traversal, distinct
diagnostics, negative-fixture cleanup, and no weakening of header hashes, SDK pinning, IID checks,
method order, or the stable-v1 prohibition.

## Companion CI scope

The companion unprotected change adds:

- a least-privilege GitHub Actions workflow on `windows-2025` for Debug and Release;
- an exact full-commit pin of the official `actions/checkout` v4 action;
- an explicit pinned-SDK presence check;
- build-wrapper rejection fixtures;
- the existing contract gates, solution build, and COM activation harness through
  `scripts/build.ps1`.

The hosted-image inventory currently lists Windows SDK `10.0.26100.0` and Visual Studio 2022 on
`windows-2025`. The first live workflow run remains the acceptance oracle; image inventory alone is
not a passing build.

## Local companion-change verification

Before publication, the isolated clean checkout passed:

- both build-wrapper failure fixtures, each preserving exit 23 and preventing MSBuild;
- Debug contract verification and negative guards;
- Debug solution build with zero warnings/errors and COM activation harness;
- Release contract verification and negative guards;
- Release solution build with zero warnings/errors and COM activation harness;
- `git diff --check`.

These local results validate the companion change but do not substitute for the first GitHub-hosted
workflow run.

## First hosted run: fail-closed SDK drift

[GitHub Actions run 34666333286](https://github.com/mario0318/aedae/actions/runs/34666333286)
executed both matrix jobs. In both Debug and Release:

- checkout succeeded;
- the `10.0.26100.0` include directory was present;
- both build-wrapper rejection fixtures passed;
- the protected contract verifier stopped before compilation because `webauthnplugin.h` hashed to
  `91E7218EA4BDB54ECE36561D98155B0741D62CBC26940FDC7357E9D6F3D87AF7`, not the reviewed
  `8B8897A5FE7D4575B5DE8287C7F0E79CED3D96CAF6D273BC7E473E225AC873B8`.

This proves that the hosted image's directory label is not an immutable contract input. The red run
must not be suppressed with `continue-on-error`, and the protected manifest must not be silently
repinned. Microsoft publishes archived 26100 SDK installers, but using one in CI requires a separate
decision about download integrity, extraction versus installation, caching, and runner cost.

Acceptable resolution paths are:

1. approve a hash-pinned official archived SDK payload, extract it into the ephemeral workspace,
   verify the three header hashes before use, and configure MSBuild to consume the same payload;
2. approve a secured self-hosted runner whose SDK bytes match the manifest; or
3. conduct a new architecture/security review of the current Microsoft header bytes and have a
   human author the protected manifest update.

Until one is approved, the CI is correctly red and T-018 remains in progress. The local matching-SDK
build evidence does not override the independent runner mismatch.
