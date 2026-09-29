# T-005: handle-bound commit amendment

Date: 2026-09-10. Status: APPROVED by explicit human-owner response on 2026-09-11.
ADR-002 has been amended; implementation verification and candidate bypass review are complete.

## Validated finding

The original CREATE_NEW temp handle closes before Commit calls MoveFileExW. A noncooperating
same-user actor can rename that source and replace its pathname. The mutex only coordinates
participating callers. Cleanup likewise calls DeleteFileW using a pathname after the handle closes.

The deterministic test invokes a separate file-open/rename operation at the actual before-rename
checkpoint in the store. It uses the same process, not an independently scheduled attacker process;
it proves the Windows handle/pathname gap without relying on timing. All inputs are synthetic.

`VaultStoreTests.exe --reproduce-substitution` confirmed three cases with exit 0:

1. AddCredential reports success while publishing substituted arbitrary bytes.
2. AddCredential reports success while publishing a substituted symbolic link; later reads refuse it.
3. Failed replacement preserves the main file but cleanup deletes the actor's replacement temp.

Exit 0 here means the vulnerability reproduced, NOT that the implementation is safe.
The separate `--test-substitution` security expectation is intended to fail until the race is fixed.
The ordinary suite does not include this expected-failing diagnostic. Its green result cannot
override the confirmed security failure.

## Independent review reconciliation

The fresh read-only investigator independently traced the shared Commit path through provisioning,
migration and CRUD, and identified the equivalent cleanup gap. Its findings agree with the source.
Claude's packet-only review returned HIGH source substitution, MEDIUM cleanup substitution, and an
ADR amendment requirement. Both defect findings are accepted based on the runtime reproduction.
No findings were rejected. Full-patch approval is deferred: there is no production candidate yet.

Review artifacts: T-005-race-review-packet.txt and T-005-race-peer-review.txt. The latter is review
input only. Its broad wording about eliminating a TOCTOU window must be read as limited to the
original temp object's identity, not every same-user metadata or pathname attack.

## Required design decision

ADR-002 section 3 and the T-005 storage-engine note explicitly mandate MoveFileEx.
Keeping a handle with delete sharing to permit pathname-based rename still permits substitution.
Closing and rechecking the path merely moves the check/use gap. Randomizing names or strengthening
the cooperative mutex does not bind the source object to the later pathname operation.

Approve the following narrow replacement for the mechanism, retaining the single-file design:

- Open the temp with GENERIC_WRITE, READ_CONTROL and DELETE; keep exclusive sharing.
- Retain that exact handle throughout validation, write, FlushFileBuffers and rename.
- Use SetFileInformationByHandle(FileRenameInfo), replacing the canonical absolute destination
  derived from the pinned directory. The initially proposed directory-relative RootDirectory form
  returned Win32 87 on the supported host and was rejected during legitimate-behavior testing.
- On failure, use FileDispositionInfo on the same handle before closing it. No pathname fallback.
- Keep format, migration semantics, snapshot API, mutex, ACL checks and synthetic-only scope.
- If handle cleanup fails, report the operation failure and leave a recoverable temp; never delete
  a separately resolved pathname in an attempt to hide the cleanup failure.

The proposed API is documented, but filesystem compatibility and durability still require tests.
Do not assume equivalence with MOVEFILE_WRITE_THROUGH or infer power-loss guarantees.

## Validation required after approval

The following validation was subsequently performed; current results are in
`T-005-handle-commit-fix.md`.

1. Before/after original trigger and alternate file/symlink substitutions; independent-process
   attempts as additional coverage, including forced-failure cleanup and every Commit caller.
2. Legitimate provisioning/CRUD/migration, locked-destination failure, and old/new snapshot bytes.
3. Existing 12 crash cases with the source handle still live at before-rename.
4. Debug, Release, hooks-disabled compilation/tests and both contract gates.
5. Fresh read-only candidate bypass/regression review, then verification of any accepted fixes.

## 2026-09-11 disposition

The owner approved this mechanism change and the candidate has now been implemented and verified.
The pre-fix SHA and failing/reproducing commands above remain historical evidence; they do not
describe the current source. Release `--test-substitution` now exits 0 after blocking all three
attacks, independent-process swap attempts are blocked for every `Commit` caller, and a fresh
candidate bypass review found no surviving source-temp or pathname-cleanup substitution. See
`T-005-handle-commit-fix.md` for current hashes, commands, results, limits, and the remaining
independent-security-approval gate. T-005 and T-006 are not marked complete or authorized.

References: [SetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle),
[FILE_RENAME_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_rename_info).
