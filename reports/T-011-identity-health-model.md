# T-011 synthetic Identity Health model

Date: 2026-09-13

Status: DONE; independently approved and human-owner accepted 2026-09-13, with direct specialized
visual/accessibility inspection retained as a documented limitation and no live metadata source

`src/App.UI/IdentityHealthModel.h` computes the Identity Health values required by `functional.md`
section 3.3 from caller-supplied synthetic metadata. It counts credential records and distinct known
provider/device identifiers, then emits local-only, single-known-authenticator and same-RP/account
duplicate-candidate warnings.

Each warning carries a stable rule ID and the exact synthetic credential references that caused it.
The model also carries an explicit statement that third-party visibility may be incomplete. It does
not infer that absent metadata means an absent credential, provider or device.

The model refuses non-synthetic input and invalid or ambiguous synthetic records without producing a
summary. It contains no key material, user handles, credential blobs, Windows Hello calls, vault
access, registration calls, network access or machine capability probing.

`src/App.UI/ManagementApp.cpp` now projects the model into the real WinUI management page. The
section labels itself `SYNTHETIC PREVIEW`, renders the three computed totals, preserves the
third-party visibility disclaimer, and gives every review-signal card its warning label, stable rule
ID and exact synthetic source-record references. Non-OK results render a refusal message instead of
a partial summary. There are no Identity Health actions.

Independent review follow-up replaced non-peer warning borders with content-view
`ContentControl`s whose combined accessible names include warning label, stable rule and every
source record. A testable presentation guard now prevents projection for live, invalid or
summary-less results. Tests also cover empty provider IDs, empty device IDs, missing locations and
both local-only records in the synthetic fixture.

This does not complete T-011 because direct pixel, 200% scale, high-contrast, keyboard and
screen-reader inspection remains. Live metadata still requires a separately reviewed read-only
service boundary.

The post-remediation independent rereview returned `APPROVE` with no findings remaining in the final
source. The reviewed hashes are recorded in `reports/T-010-T-011-qwen-rereview.md`.

The human owner accepted the final T-010/T-011 milestone on 2026-09-13 without expanding the
synthetic-only boundary or representing the remaining direct inspection items as completed.

## Verification

Both complete `scripts/build.ps1` configurations passed. Each run independently passed the pinned
contract check and its negative guard, compiled the full solution with warnings as errors, then ran
the COM, vault, management-model and random-buffer key-protection suites. Debug and Release each
reported zero compiler warnings and zero errors. A focused Release rebuild of
`ManagementModelTests.vcxproj` with native code analysis enabled also reported zero warnings and
zero errors, and the resulting management-model tests passed.

After the WinUI projection was added, both complete Debug and Release wrappers passed again with
zero build warnings and errors. The Release executable created a responsive top-level WinUI window
titled `aeDae — This PC`; no package registration was performed. This proves real window creation,
not pixel or assistive-technology behavior.

The focused assertions cover computed totals, all three warning rules, stable rule/source links,
the incomplete-visibility statement, known-empty input, non-synthetic refusal and duplicate source
identifier refusal. Repository whitespace checks pass for the model, tests, report and task update.
