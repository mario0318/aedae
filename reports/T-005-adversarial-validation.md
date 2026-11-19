# T-005 adversarial validation follow-up

Date: 2026-09-09. Synthetic-only foundation. Independent security approval remains outstanding.

**Superseded security state (2026-09-11):** This report records the earlier remediation and the
pre-fix residual pathname race. The owner-approved handle-bound repair and its current evidence are
in `T-005-handle-commit-fix.md`. The old source hash and residual-race language below are retained
as historical evidence, not a description of the current implementation.

## Scope

This continuation owns the vault implementation and its directly necessary native tests,
the VaultStoreTests project configuration, scripts/build.ps1 as the build-wrapper repair,
tests/unit/Test-BuildWrapper.ps1 and its two fixture scripts, and T-005 evidence documentation.
No protected contract script, ABI manifest, plugin source, lifecycle file, or website was edited.
No dependencies, real credentials, key protection, protocol integration, or release work were added.

## Reproduced defects and repairs

### Directory replacement during commit

The original directory open requested READ_CONTROL and FILE_READ_ATTRIBUTES without delete
sharing. That metadata-only handle did not prevent a rename once the temp-file handle was closed.
The new crash harness failed at `before-rename` with `FAIL active vault directory cannot be renamed`.
The directory open now also requests FILE_LIST_DIRECTORY. The regression attempts to rename both
the direct vault directory and an ancestor while the writer is paused at each commit checkpoint.
The fixture restores an ancestor if its attempted rename succeeds; it never targets user directories.

### Cleanup deleted a file that this operation never created

The old cleanup destructor deleted the temp path whenever the commit had not completed, including
when CREATE_NEW failed. A test inserts a sentinel file after stale-temp discard and before
CREATE_NEW. Running the focused test with the original destructor returned exit 1 and
`FAIL failed exclusive create does not delete another creator's file`.
Cleanup now tracks successful creation and only removes a temp it created. The regression requires
an error result, byte-identical main file, and an unchanged sentinel; explicit recovery can then
discard the synthetic leftover. This does not claim to close every same-user pathname-swap race
after the temp handle is closed; that remains an independent review concern.

### Build wrapper returned success before compilation

In a fresh PowerShell process, the successful contract script did not initialize LASTEXITCODE.
The wrapper compared that null value with zero and exited before building. It could also inherit
a stale native exit code. Each gate now runs in its own PowerShell process. The wrapper preserves
gate/build/harness failures and executes the vault test executable after the COM harness.
Two isolated fixture layouts prove that rejection from either contract gate propagates exit 23
and prevents compilation. A separate run seeds LASTEXITCODE=47 and exercises the actual wrapper.
The three protected contract scripts were not changed to achieve this.

## Coverage and precise limits

| Boundary | Test evidence |
| --- | --- |
| Process crashes | Actual commit path paused at temp-created, partial-write, written, flushed, before-rename, and renamed; 6 cases each for CRUD and migration |
| Atomic snapshots | After killing the child, the main file equals the expected old or new byte sequence; post-crash parse succeeds under an abandoned mutex |
| Recovery | Expected leftover state checked; recovery removes temp without merging; migration/CRUD restart succeeds |
| Path replacement | Direct directory and ancestor rename attempts at all 12 crash points |
| ACL grants on disk | Foreign generic-write and inherit-only-write refusal, deny-plus-allow conservative refusal, legitimate foreign read-only acceptance |
| Descriptor policy | Actual production validator rejects foreign owner, null DACL, callback ACE, and object-specific ACE; baseline current owner accepted |
| File reparse points | Real symbolic link refused, target unchanged; symbolic-link temp refused without deleting target |
| Ownership integration | Attempt to install a foreign owner was refused by Windows with 1307; NOT counted as an on-disk owner-rejection pass |
| Temp collision | Failed CREATE_NEW preserves another creator's sentinel and the original vault |
| Build gates | Verification rejection and negative-guard rejection both stop compilation with exit 23 |
| Hook isolation | Test project defines AEDAE_VAULT_TEST_HOOKS; separate hooks-disabled compile/run verifies the ordinary implementation links and operates without injection support |

Crash hooks are compiled only when AEDAE_VAULT_TEST_HOOKS is defined by the native test project.
They are not a public API and are not enabled in the plugin or UI. The hook callback is inert during
normal tests. Child processes signal a named event at the actual source checkpoint and are killed
by their parent; child cleanup also terminates an owned helper if a test aborts. Timeouts fail tests.
Process termination is NOT an OS crash or power-loss durability test. No result here proves arbitrary
same-user malware resistance, authenticated IPC writer ownership, or safe use of real credentials.

## Reproduction

Run from C:/Projects/aeDae:

```powershell
pwsh -NoProfile -File tests/unit/Test-BuildWrapper.ps1
pwsh -NoProfile -File scripts/build.ps1 -Configuration Debug
pwsh -NoProfile -File scripts/build.ps1 -Configuration Release
pwsh -NoProfile -Command '$global:LASTEXITCODE = 47; & ./scripts/build.ps1 -Configuration Debug; exit $LASTEXITCODE'
./artifacts/Debug/VaultStoreTests.exe --collision-only
```

For hooks-disabled coverage, invoke the discovered MSBuild against AeDae.sln with
`/p:Configuration=Release /p:Platform=x64 /p:VaultTestHooks=false`, then run
artifacts/Release/VaultStoreTests.exe. Rebuild normally afterward to restore the crash-enabled suite.

Logs are local generated artifacts, not security approvals. Synthetic temporary fixtures are retained; test-created
hardlinks and symlinks are removed during their tests. No unrelated files were deleted.

## Final verification results

| Run | Result / log |
| --- | --- |
| Debug wrapper: both gates, build, COM harness, vault suite | Exit 0; artifacts/t005-debug-final.log; 12 crash cases plus collision assertion |
| Release wrapper: both gates, build, COM harness, vault suite | Exit 0; artifacts/t005-release-final.log; 12 crash cases plus collision assertion |
| Hooks-disabled Release build and suite | Exit 0; artifacts/t005-nohooks-final.log; no crash callbacks compiled/run |
| Normal Release restored and suite rerun | Exit 0; artifacts/t005-restored-release.log; all 12 crash cases restored |
| Wrapper with inherited LASTEXITCODE=47 | Exit 0; artifacts/t005-stale-final.log; compilation and both harnesses actually ran |
| Build-gate rejection fixtures | Exit 0 from regression script; both nested wrapper cases returned expected exit 23 |
| Focused collision regression | Exit 1 with old destructor, exit 0 with creation-ownership guard |
| Whitespace validation | git diff --check returned 0; existing line-ending warnings only |

SHA-256 of tested source (not an approval or release signature):

- src/Vault/VaultStore.cpp: B3FC67E72C0393C7A095E43A21A78AEC0EBD44DAB8A660293BEECA9F2192C65F
- tests/unit/VaultStoreTests.cpp: EC5D4DDD960526B55264278693E61C8B9482E4F738F1AA050DC96A6212DFCA8F
- scripts/build.ps1: 3275A9A458283231201BF991B148EEB568D1657B2D4117105B4F9D604DA02105

## Review gate

T-005 remains IN_PROGRESS until independent review resolves the residual path/cleanup concerns,
assesses the documented platform test limit, and approves the sensitive changes. Human milestone
approval is still required. Do not start T-006 or enable real credential storage based on these tests.

The residual source-temp and cleanup-path concern referenced above is fixed in the 2026-09-11
candidate and has passed a fresh bypass review. The foreign-owner integration and power-loss claim
limits remain, and formal independent security approval plus human milestone approval are still
required.

API context: [CreateFile access and sharing](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea),
[symbolic-link creation](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-createsymboliclinka).
