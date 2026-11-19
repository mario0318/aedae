# T-005 handoff prompt (for Codex)

Paste the block below. It is self-contained; do not paste the repository.

---

You are the CODER role on a security-sensitive Windows project at `C:\Projects\aeDae`. Read
`AGENTS.md` first — it is binding, and it limits you to the files named in your task.

## Task: T-005 — Vault schema and CRUD

Implement `IVaultStore` as a versioned single-file store, plus its tests. This is foundation work
against **synthetic, non-credential data only**.

## Hard prohibitions (violating any of these fails the task)

- **No DPAPI, no Windows Hello, no plugin coupling, no WebAuthn, no signing, no real key material.**
  Records hold fabricated non-key blobs. T-006 adds key protection later; do not anticipate it.
- **No third-party dependencies.** SQLite is explicitly forbidden by `reports/ADR-002-single-file-vault-store.md`
  and the build fails on any reference to `sqlite3.h`, `sqlite3.c`, `sqlite3ext.h`, or
  `sqlite-amalgamation`. The unapproved amalgamation sits untracked under `external/` — ignore it.
- **No test framework.** GoogleTest, Catch2, doctest, and friends are unreviewed dependencies and
  are prohibited. Write a self-contained native test executable that returns a nonzero exit code on
  failure, following the existing `tests/integration/ComActivationHarness.cpp` pattern.
- **No `EXPERIMENTAL_` symbol references** anywhere (ADR-001).

## Files you own

- `src/Vault/VaultStore.h`, `src/Vault/VaultStore.cpp`
- `src/Vault/IVaultStore.h` (interface, if you split it out)
- `tests/unit/VaultStoreTests.cpp` (new)
- `build/VaultStoreTests.vcxproj` (new; add to `AeDae.sln`, x64, `WindowsTargetPlatformVersion`
  must match the other projects exactly or the contract check fails)

## Files you must NOT touch

Other sessions hold these. Do not edit, and do not "fix" them in passing:

- `reports/operation-lifecycle-design.md`, `reports/lifecycle-security-review.md`,
  `tests/integration/OperationLifecycleTestPlan.md`
- `scripts/ContractCheck.psm1`, `scripts/verify-webauthnplugin-contract.ps1`,
  `scripts/test-webauthnplugin-contract-guard.ps1`, `reports/webauthnplugin-abi-manifest.json`
- anything under `web/`
- `src/PluginAuthenticator/*` — the plugin does not call the vault yet

You may append a status note to your own task entry in `tasks.md` and nothing else in that file.

## Interface (from `architecture.md` §4)

```
IVaultStore
  AddCredential(record)
  GetCredentialById(rpId, credentialId)
  FindCredentialsForRp(rpId)
  ListCredentials()
  UpdateLastUsed(credentialId)
  DeleteCredential(credentialId)
```

`CredentialRecord` fields, from `architecture.md` §5: id, rpId, accountLabel,
userHandleEncryptedOrMinimized, credentialId, publicKeyCose, protectedPrivateKey, createdAt,
lastUsedAt, userVerificationPolicy, originSource, backupStatus, schemaVersion. In T-005
`protectedPrivateKey` holds a fabricated opaque blob, never a key.

## Storage rules — these are the decision, not suggestions

From `reports/ADR-002-single-file-vault-store.md`. Read it before starting.

1. **One writer.** The vault service is the sole writer. Readers never open for write.
2. **Path-scoped named mutex** derived from the canonical vault path, held across each
   read-modify-commit sequence and across each snapshot read. Two processes must serialize.
3. **Rename commits.** Sibling temp file → flush → `FlushFileBuffers` → `MoveFileEx` with
   `MOVEFILE_REPLACE_EXISTING`. A leftover temp is discarded on open, **never merged**. A crash
   leaves either the complete old store or the complete new store.
4. **Versioned parsing.** Schema version in the header, checked before parsing. Unknown, malformed,
   truncated, or internally inconsistent → fail closed, return an error, do not guess or repair.
5. **ACL check on every open.** See the correction below — implement the corrected rule, not the
   ADR's literal wording.

### ACL rule correction (apply this; the ADR text is too strict)

ADR-002 §5 says refuse if the file is writable by "principals other than the current user." Taken
literally that fails on every normal Windows install: the default DACL under `%LOCALAPPDATA%`
grants `SYSTEM` and the local `Administrators` group full control, and both can bypass any ACL you
set anyway. Implement instead:

> Enumerate the DACL of the vault directory and file, including inherited ACEs. Allow write access
> for exactly three principals: the current user, `SYSTEM` (`S-1-5-18`), and the local
> `Administrators` group (`S-1-5-32-544`). Refuse to open, fail closed, and return a distinct error
> if any other principal holds write, append, delete, or change-permissions access.

State this correction in a comment at the check, citing ADR-002 §5.

## Acceptance criteria (from `tasks.md` T-005)

Tests must cover, each as a distinct case:

- add, find-by-id, find-by-RP, list, update-last-used, delete
- schema migration across at least one version step
- migration failure → fails closed, original file intact
- unknown schema version → refused, not repaired
- corrupt/truncated file → refused, not repaired
- leftover temp file on open → discarded, never merged
- ACL check refuses a directory writable by an unexpected principal, and accepts a default
  user-profile ACL
- cross-process reader/writer: a reader observes only the complete old snapshot or the complete
  new one, never a torn intermediate. Drive this with two processes or two threads plus an
  injected delay between temp-write and rename; assert no partial read is ever observable.

## Build and verify

```
pwsh -File scripts\build.ps1 -Configuration Debug
```

This runs the contract verifier and its guard before compiling; both must pass. Then run your test
executable and report its exit code. Per `AGENTS.md`, no code task is complete without a real
build/test result — if something cannot run, say so explicitly rather than asserting success.

## Report back

A compact change report: files changed, commands run, actual output, tests passing/failing, and any
place where you deviated from this prompt and why. Cap yourself at three autonomous repair cycles;
if still broken, stop and write a short blocker note instead of continuing.
