# ADR-002 — Single-file vault store

- Status: **ACCEPTED**
- Date: 2026-09-04
- Decider: Human owner (R3 Labs)
- Amended: 2026-09-11 by explicit human-owner approval
- Related: T-005, `architecture.md`, `functional.md`, `security.md`

## Context

The local credential vault is expected to hold tens to low hundreds of records. Private-key
secrecy is provided by per-record DPAPI wrapping before key material reaches persistent storage;
the storage engine therefore primarily protects metadata integrity and recovery.

SQLite would add an approximately 11 MB third-party dependency and require a dependency and
security review. That review burden is not justified for this small local store. The prior
single-process assumption is also invalid: the plugin operates in the Windows broker process and
the management UI enumerates and requests revocation from a separate process.

## Decision

T-005 uses a versioned, single-file store. SQLite is not an approved dependency and must not be
included or linked.

1. **One writer.** The plugin-owned vault service is the sole writer. The UI never opens the vault
   for writing; its delete and disable requests go through the management service to that owner.
   UI reads use a consistent snapshot.
2. **Path-scoped synchronization.** A named mutex derived from the canonical vault path is held
   across each read-modify-commit sequence. Reads acquire the same mutex before opening a snapshot.
3. **Handle-bound rename commits.** A writer creates a sibling temporary file with exclusive
   sharing and `DELETE` access, retains that exact handle through validation and writing, flushes
   it, calls `FlushFileBuffers`, then calls `SetFileInformationByHandle(FileRenameInfo)` with
   replace-existing semantics and the canonical absolute destination resolved from the already-open
   vault directory. The directory handle remains open without delete sharing throughout, preventing
   its directory entry from being renamed while the canonical destination is used. A directory-relative
   `RootDirectory` form was tested but returned `ERROR_INVALID_PARAMETER` (87) on the supported host,
   so it is not the selected mechanism. Failure cleanup
   marks that same handle with `FileDispositionInfo`; it never deletes a re-resolved pathname. This
   mechanism supersedes the original `MoveFileEx` wording after deterministic tests proved source
   substitution and substituted-path deletion between handle close and pathname use. A leftover
   temporary file is discarded on open and is never merged. A crash therefore leaves either the
   old complete store or the new complete store, never a partially parsed intermediate.
4. **Versioned parsing.** The header contains a schema version that is checked before parsing.
   An unknown, malformed, truncated, or inconsistent version fails closed.
5. **Access control.** The user-scoped vault directory receives restrictive ACLs at creation. On
   every open, the service verifies that neither the directory nor the vault file is writable by
   principals other than the current user and refuses to proceed when that check fails.

## Consequences

- T-005 owns migrations, recovery, corruption testing, and cross-process synchronization rather
  than delegating them to a database engine.
- The store remains restricted to synthetic, non-key records until separately authorized work
  permits more; this ADR authorizes no plugin, protocol, Windows Hello, DPAPI, or signing work.
- The unapproved `external/sqlite-amalgamation-3530400/` directory is **still present and untracked**
  in the worktree: removal was blocked by the environment and the safeguard was not bypassed. It is
  forbidden by this ADR and enforced at build time by the SQLite clause in
  `Invoke-AeDaeSourceCheck` (`scripts/ContractCheck.psm1`), which fails the build on any reference
  to it from `src/`, `build/`, or `artifacts/`. Deleting the directory remains the preferred end
  state; until then the build-time guard, not its absence, is what holds the decision.

## Revisit trigger

Revisit only if the single-file acceptance tests prove the recovery or synchronization requirements
cannot be met, or if the human owner explicitly authorizes a reviewed third-party storage engine.
