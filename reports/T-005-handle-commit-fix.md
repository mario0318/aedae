# T-005 handle-bound commit fix

Date: 2026-09-11. Outcome: **SECURITY APPROVED WITH CONDITIONS; human milestone approval remains.**

## Security invariant

Once the vault writer creates and validates its sibling temporary file, every subsequent write,
flush, rename, and failure cleanup must act on that exact file object. A noncooperating same-user
process must not be able to replace the temporary pathname and cause different bytes, a reparse
point, or another actor's file to be published or deleted.

## Original exploit path

The previous implementation closed the exclusive `CREATE_NEW` handle before calling pathname-based
`MoveFileExW`. At the actual `before-rename` checkpoint, the regression could rename the original
temporary file away and replace its pathname. The writer then published substituted bytes or a
symbolic link while reporting success. On a forced commit failure, pathname-based cleanup deleted
the replacement file. The focused pre-fix security expectation exited 1.

## Approved repair

The human owner explicitly approved the ADR-002 amendment on 2026-09-11. `Commit` now:

1. opens the new temp with `GENERIC_WRITE | READ_CONTROL | DELETE`, exclusive sharing and
   `FILE_FLAG_OPEN_REPARSE_POINT`;
2. validates and retains that exact handle through both writes and `FlushFileBuffers`;
3. replaces the canonical absolute destination using
   `SetFileInformationByHandle(FileRenameInfo)` on the retained handle; and
4. on failure, marks that same handle with `FileDispositionInfo`, with no pathname cleanup fallback.

The checked directory handle stays open without delete sharing during the operation. A proposed
directory-relative `RootDirectory` rename was rejected because it returned
`ERROR_INVALID_PARAMETER` (87) on the supported host. ADR-002 records the tested absolute-destination
form rather than claiming that the rejected form is in use.

Files changed for this repair:

- `src/Vault/VaultStore.cpp`
- `tests/unit/VaultStoreTests.cpp`
- `reports/ADR-002-single-file-vault-store.md`
- T-005 notes and evidence reports

No plugin, WebAuthn, Windows Hello, DPAPI, signing, registration, real credential, or dependency
work is included. Test records and blobs are synthetic.

## Verification evidence

| Gate | Result |
| --- | --- |
| Pre-fix deterministic source substitution | Reproduced arbitrary-byte publication, symlink publication, and deletion of a substituted temp |
| Release `--test-substitution` | Exit 0; all three cases report `PASS SECURITY REGRESSION: temp substitution must be prevented` |
| Release historical `--reproduce-substitution` alias | Exit 0; now enforces and reports the same three blocked-attack expectations |
| Independent-process source swap | Five attempts blocked while the live temp handle was held |
| Commit callers | Provision, migration, add, update and delete each succeeded legitimately after the blocked attempt; resulting snapshots parsed with expected state |
| Crash/recovery suite | 12 actual-commit checkpoints passed across CRUD and migration, including `before-rename` and `renamed` |
| Debug full wrapper | Exit 0; both contract gates, solution build, COM harness, vault suite and management-model suite passed |
| Release full wrapper | Exit 0; hooks were recompiled in, build passed with 0 warnings/errors, and all harnesses/suites passed |
| Hooks-disabled Release | Exit 0 in the prior dedicated run; ordinary implementation links and operates without injection hooks |
| Build-wrapper regression | Exit 0; both isolated rejection fixtures returned the required exit 23 and stopped compilation |
| Whitespace check | `git diff --check` exit 0; only existing line-ending conversion warnings |

The focused Debug run is retained at `artifacts/t005-handle-focused-full.log`; full wrapper evidence
is retained at `artifacts/t005-handle-wrapper-debug.log` and
`artifacts/t005-handle-wrapper-release.log`. The final normal Release restoration was also observed
compiling `VaultStore.cpp` and `VaultStoreTests.cpp` with `AEDAE_VAULT_TEST_HOOKS` before the named
Release checks above.

SHA-256 of the verified sources:

- `src/Vault/VaultStore.cpp`: `18C724B6F124CA231C58300D18C755454D7F649D4D412AD7BF0D6295E4D81149`
- `tests/unit/VaultStoreTests.cpp`: `FD0118890EF63B01DA53D629E5B4CE245591FD655C7136980B4BFBC5F994D492`
- `scripts/build.ps1`: `A132B719C65D1EA5A837746871F1E638B0D3455A176DDD0714B22DEB2C9522DC`

## Fresh candidate bypass review

A fresh read-only reviewer traced the final `Commit` source and the substitution tests. It found no
remaining source-temp substitution or cleanup-by-re-resolved-path bypass. It caught one concrete
documentation mismatch—the ADR described a directory-relative rename although the verified code
uses an absolute canonical destination—and that mismatch was accepted and corrected before this
report. No security-critical code finding from that review remains unaddressed.

The final Claude second-pass review returned an empty findings array. It independently confirmed
that creation, validation, writes, flush, rename and failure disposition remain bound to the same
exclusive handle; that the ADR matches the implemented rename form; and that the independent-process
tests cover every `Commit` caller. There were no accepted, rejected, or deferred findings in that
pass. Its review input is retained in `T-005-final-review-packet.txt`.

## Deliberate claim limits

- Process termination tests are not power-loss, kernel-crash, or storage-controller durability
  tests. The code does not claim more than complete old-or-new snapshots under the exercised cases.
- Windows refused the on-disk foreign-owner fixture with error 1307. The production descriptor
  validator rejects a foreign owner, but that descriptor-level result is not represented as an
  on-disk integration pass.
- If Windows refuses `FileDispositionInfo`, an operation failure can leave the exact created temp
  for explicit recovery. The implementation will not delete a re-resolved pathname to conceal it.
- Network, redirected, and unusual filesystems are outside the supported local-vault evidence.
- The unapproved SQLite source directory remains untracked and is neither included nor linked.

## Remaining gate

This finding's ordered verification checks pass and the original exploit no longer reproduces,
while legitimate behavior remains intact. A fresh independent security reviewer subsequently issued
`APPROVE_WITH_CONDITIONS` for the exact hashes and claim limits recorded above. T-005 stays
`IN_REVIEW` until the human owner approves the milestone. This approval does not authorize T-006 or
any use of real credentials.
