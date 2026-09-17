# T-005 independent security-review handoff

Updated: 2026-09-11. Status: IN_PROGRESS, verified fix candidate; not security-approved or DONE.

**2026-09-11 update:** The owner-approved handle-bound repair is implemented. The writer retains
the exclusive `DELETE`-capable temp handle through `FileRenameInfo` and performs failure cleanup
with `FileDispositionInfo` on that exact handle. Release `--test-substitution` now blocks all three
previously reproduced attacks, and independent-process attempts are blocked across provision,
migration, add, update and delete. A fresh bypass review found no remaining source-temp or
pathname-cleanup substitution. Review the current candidate and evidence in
`T-005-handle-commit-fix.md`; the 2026-09-10 paragraph below is retained as pre-fix history.

**2026-09-10 update:** The residual race is now confirmed, not merely an open review question.
The focused --test-substitution command fails with exit 1. Reproduction covers substituted bytes,
symlink publication, and deletion of substituted temp during failed commit. Claude's packet-only
review and a separate read-only investigator agree. Production storage code is unchanged pending
explicit approval of the mechanism change described in
[the handle-bound commit proposal](T-005-handle-commit-proposal.md). Do not treat the earlier
ordinary-suite green results as security acceptance. The review packet and output are in
T-005-race-review-packet.txt and T-005-race-peer-review.txt.

## Review request

Review the current files without editing them. Classify each original finding as fixed,
partially fixed, or open. For remaining findings provide severity, file/line, concrete failure
condition, required remediation and a reproducer. Passing tests are evidence, not approval.

The previous handoff's claims about directory pinning and crash coverage were superseded by
the follow-up tests. The exact reproduced failures, repairs, coverage and limits are recorded in
[T-005 adversarial validation](T-005-adversarial-validation.md).

## Scope

- src/Vault/IVaultStore.h, VaultStore.h, VaultStore.cpp
- tests/unit/VaultStoreTests.cpp
- build/VaultStoreTests.vcxproj and only its AeDae.sln entry
- scripts/build.ps1
- tests/unit/Test-BuildWrapper.ps1 and fixtures/gate-success.ps1, fixtures/gate-fail-23.ps1
- T-005 notes in tasks.md, ADR-002 and these T-005 evidence reports

Other dirty files are concurrent work. Protected contract scripts, ABI manifest, plugin,
lifecycle and website files were not changed by this continuation.

Synthetic records only. Fabricated private-key-field bytes are NOT DPAPI-protected. No actual
credentials, crypto, Hello, protocol integration, registration, authenticated management IPC,
third-party dependency or production release is implemented or authorized.

## Original finding map

| Finding | Current response | Important evidence / limits |
| --- | --- | --- |
| H1 stale wait error | Explicit wait-result switch | Timeout and abandoned-owner tests; WAIT_FAILED not injected |
| H2 path aliases | Mutex uses volume serial and opened directory file identity, shared across every file in dedicated directory | Cross-process short-directory alias, short-filename update, hardlink refusal |
| H3 shared error race | Immutable configuration and per-call result/status | No shared last_error member |
| H4 lock leaks / infinite wait | RAII ownership, constructor failure cleanup, 2-second timeout | Validation failure then success; timeout; killed writer recovery |
| M1 missing owner check | Handle owner validation | Production descriptor validator rejects foreign owner; Windows refused foreign on-disk owner fixture with 1307 |
| M2 ACL/open race | Read through checked handle, normalized parent/file, reparse/hardlink refusal | Directory open now includes FILE_LIST_DIRECTORY after reproducing rename bypass; direct/ancestor rename attempts pass at all crash stages |
| M3 fake migration | Explicit legacy v1/v2 layout to v3 with per-record schema field | Exact byte conversion, forced rename failure, 6 migration crash points and restart |
| M4 mutating reads | Snapshot default; explicit writer Provision/Recover/Migrate | Reads do not create, migrate, or delete leftovers |
| M5 ambiguous empty/error | Optional value plus explicit error | Successful empty result is distinguishable |
| M6 inconsistent identity | Global credential_id and record id uniqueness | Cross-RP duplicate rejected; RP lookup isolation |
| M7 temp cleanup/ACL | Exact temp handle retained through rename and same-handle failure disposition | Failed rename cleanup; competing CREATE_NEW sentinel preserved; three original substitutions and five independent-process caller attempts blocked |
| M8 stale deletion error | Per-call result | Failed delete followed by successful delete |
| M9 auditability | Named parsing, ACL, handle, serialization and commit helpers | No second/superseded implementation retained |

## Design boundaries

- Writer mode is a caller convention, not authenticated broker/IPC authorization.
- The lock deliberately serializes a whole dedicated local directory, avoiding filename aliases.
- Existing filenames normalize under the lock. Hardlinks and file reparse points fail closed.
- Allowed owners/writers are current user, SYSTEM, Administrators, per the human owner's ACL correction.
- Unknown callback/object ACE types are refused. Inherit-only writes and generic writes are checked.
- Parser bounds: 16 MiB file, 1 MiB blobs, 32768 UTF-16 code units per string, 10000 records.
- v1/v2 describe this unreleased foundation's legacy layout; v3 adds per-record schema.
- Enumeration/lookup strip private/public key blobs and user handle. No production key retrieval.
- Temp files are discarded only by explicit writer operations, never merged.
- Cleanup is best-effort if Windows itself rejects handle disposition. The exact temp handle stays
  open through handle-bound rename and cleanup; no pathname deletion fallback exists. Reviewers
  should verify this current mechanism without inferring complete same-user malware resistance.
- Named-object squatting may deny service. Wait is bounded; mutex creator is not authenticated.
- No power-cut or OS-crash claim follows from process-termination tests.

## Current verification

The ordinary build wrapper now executes both contract gates, MSBuild, COM harness and vault suite.
Before the repair, null/stale LASTEXITCODE could make it return before compilation. Its success
is now corroborated by the build and harness output, not merely exit 0.

Run from C:/Projects/aeDae:

```powershell
pwsh -NoProfile -File tests/unit/Test-BuildWrapper.ps1
pwsh -NoProfile -File scripts/build.ps1 -Configuration Debug
pwsh -NoProfile -File scripts/build.ps1 -Configuration Release
./artifacts/Release/VaultStoreTests.exe --test-substitution
./artifacts/Debug/VaultStoreTests.exe --collision-only
```

Debug and Release full wrapper runs returned 0, including all 12 crash cases and the collision
regression. Real file/temp symbolic-link cases ran. Descriptor tests cover foreign owner,
null DACL, callback and object ACEs; on-disk tests cover generic/inherit-only/deny+allow/read-only
ACEs. The isolated gate regressions prove rejection stops compilation with exit 23.

Test-only checkpoints run inside the actual commit code and are enabled only by the test
project's AEDAE_VAULT_TEST_HOOKS define. A separate /p:VaultTestHooks=false Release build/run
checks the ordinary implementation without injection support. No plugin or UI hook was added.
See the adversarial report for final logs, stale-exit-code regression, and exact platform limits.
See `T-005-handle-commit-fix.md` for the later handle-bound security regression, independent-process
caller coverage, current source hashes and the final empty-findings Claude review.

## Remaining review gate

1. Independently approve or reject the current handle-bound candidate after checking the source,
   the focused substitution test and every-caller independent-process coverage.
2. Assess the foreign-owner integration limit: Windows returned 1307. The descriptor-level test
   must not be mistaken for an actual foreign-owned file open. Callback/object ACE coverage is
   descriptor-level, not installed on-disk conditional-ACE coverage.
3. Assess process-crash coverage without extending its claims to power failure or all races.
4. SQLite's previously unapproved source directory remains present after the earlier blocked
   removal; it is not adopted or linked. Do not bypass that safeguard.
5. Keep T-005 unapproved and real credentials disabled until independent security review and
   human milestone approval. Do not start T-006 based solely on the green tests.
