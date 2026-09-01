# Windows Personal Authenticator – Task Backlog

Last restructured: 2026-08-29 (see `reports/ADR-001-experimental-v2-apis.md`)

## Operating rules

- Every task must be small, testable, and associated with exact files or modules.
- Agents update only files within their authorized ownership boundary.
- No code task is complete without a build/test result or an explicit explanation of why the
  result cannot run.
- Security-sensitive tasks require Security review before merge.

## Status values

`TODO` | `IN_PROGRESS` | `BLOCKED` | `PAUSED` | `IN_REVIEW` | `DONE`

`BLOCKED` means waiting on another task in this backlog. `PAUSED` means waiting on something
outside this project's control.

## Lane structure

This backlog has two independent lanes. Blockage in one does not imply blockage in the other.

**Protocol lane — PAUSED on an external dependency.** The pinned public headers do not define the
operation-signing envelope, so the T-015 gate cannot be designed against stable v1. Experimental
v2 is not adopted (ADR-001). Nothing in this lane advances until T-019 returns an answer.

**Local lane — OPEN.** Vault, key protection, UI, and repository governance are independent of the
envelope question and proceed now against synthetic, non-credential data.

---

## Local lane (open)

### T-017 – Contract manifest governance

- Status: TODO
- Owner: PACKAGER
- Priority: **merge-blocker for T-014**
- Scope: Add `CODEOWNERS` requiring human-owner approval for the pinned ABI manifest and the three
  contract scripts. Enable branch protection on those paths. Add an `AGENTS.md` clause naming them
  human-approval-only and off-limits to every agent role.
- Rationale: The contract pin was centralized into one manifest file. Without governance the entire
  T-014 approval is defeatable by a single unreviewed edit.
- Done when: Protection is active, an agent-authored edit to a protected path is demonstrably
  rejected, and `AGENTS.md` names the paths.

### T-005 – Vault schema and CRUD

- Status: TODO
- Owner: CODER
- Scope: Credential database schema and `IVaultStore` implementation using **synthetic records
  only**. No real key material, no DPAPI integration, no plugin or protocol coupling.
- Files: `src/Vault/VaultStore.*`, tests.
- Dependencies: Storage-engine decision (see note below).
- Done when: Unit tests cover add, find, list, update-last-used, delete, schema migration, and
  corruption handling, against fabricated non-key blobs.

> **Storage-engine note.** `architecture.md` makes the database selection contingent on a
> dependency/security review. SQLite is a candidate, not an approved dependency. Before T-005
> starts, the human owner decides one of: (a) approve a pinned SQLite amalgamation with static
> linkage, extensions disabled, and a documented journal/durability/ACL policy; or (b) implement a
> single-file store with write-temp → fsync → atomic rename and a schema-version field. Option (b)
> satisfies the transactional-recovery requirement in `security.md` with no third-party dependency
> and no review burden, and is the recommended default for a single-user, single-process,
> low-row-count vault.

### T-006 – DPAPI-backed key protection

- Status: TODO
- Owner: CODER
- Scope: `IKeyProtection` with DPAPI-backed wrapped storage, exercised with **random test buffers
  only**. No credential keys, no plugin coupling.
- Files: `src/Vault/KeyProtection.*`, tests.
- Dependencies: T-005.
- Done when: Protected data is unreadable at rest and wrong-context or tampered data fails closed.
- Review: SECURITY required.

### T-010 – Management UI: This PC

- Status: TODO
- Owner: CODER
- Scope: First WinUI page — provider state, Windows Hello availability, key-protection status,
  local credential count. Static or mock data sources only.
- Done when: It renders from the service layer with no protocol or credential dependency.

### T-011 – Management UI: Identity Health

- Status: TODO
- Owner: CODER
- Scope: Totals, local-only credentials, duplicate candidates, and stated limits of foreign-provider
  visibility. Static or mock data only.
- Done when: Every displayed warning links to its computed rule and source data.

### T-018 – Contract verifier follow-ups

- Status: TODO
- Owner: CODER
- Scope: Close the non-blocking findings from the T-014 review.
  - **MED-02** — a stabilized (de-prefixed) v2 symbol currently surfaces as
    `Missing required WebAuthn Plugin API symbol`, i.e. the most security-significant possible
    upstream change is indistinguishable from a broken toolchain. Emit a distinct, loud diagnostic.
  - **MED-03** — no CI. Every build and test result to date is implementer-reported. Stand up a
    pipeline that reproduces build, contract verification, and tests independently.
  - **MED-04** — replace `if (-not $?)` error handling with construction that fails by design
    rather than by accident of every failure being a `throw`.
- Done when: Each finding has a test or pipeline artifact demonstrating closure.

### T-020 – aeDae public website (`web/`)

- Status: IN_REVIEW
- Owner: CODER
- Scope: Static marketing and documentation site for the five aeDae public surfaces
  (`aedae.tech`, `aedae.world`, `aedae.life`, `aedae.live`, `aedae.online`).
  Confined entirely to `web/`. No changes to `src/`, `tests/`, `reports/`,
  `scripts/`, `external/`, or authenticator code.
- Files: `web/` — `build.mjs`, `README.md`, `spec/`, `src/`. `web/dist/` and
  `web/preview/` are generated output, gitignored, and not part of the diff.
- Prohibited: No DNS, hosting, registrar, email, analytics, CDN, or deployment
  configuration. No network calls at build or runtime. The build emits static
  HTML/CSS with no JavaScript on any page. No modification to files outside
  `web/`.
- Dependencies: None. Independent of the protocol lane and the local lane's
  vault/UI work.
- Done when: `node build.mjs`, run from `web/`, succeeds and reports 15 pages
  across 5 surfaces; the validation gates demonstrably reject an unsourced
  claim, a reference to `aedae_handoff_v1_3_final.md`, and any
  `data-status="built"` claim, then pass cleanly once each is removed;
  `git diff --check` is clean; apart from this task entry, the diff touches
  only `web/`; and the human owner reviews and approves the pull request.
- Review: Human-owner approval is required before merge under the `AGENTS.md`
  merge gate. No SECURITY review is required for this phase: it collects no
  user data and contains no client-side scripts or network calls. Reopen this
  line if a later phase adds any of those.

---

## Protocol lane (paused — external dependency)

### T-019 – Resolve the operation-signing envelope

- Status: TODO
- Owner: ARCHITECT
- Priority: **critical path for the entire protocol lane**
- Scope: Determine the exact byte sequence the platform signs for plugin operation requests. Open
  an issue against `microsoft/webauthn`, and in parallel record what the Microsoft Passkey Manager
  sample actually constructs and verifies.
- Done when: Either the envelope is documented well enough for T-015 to specify verification over
  an exact byte sequence, or a written finding records that it is not obtainable from public
  sources.

### T-015 – Operation signature-verification gate design

- Status: PAUSED
- Owner: ARCHITECT
- Paused on: T-019. Cannot be completed — the signing envelope is undefined in the pinned headers.
- Scope: Fail-closed ordering for authenticating platform operation and cancellation requests
  before all protocol side effects.
- Output: `reports/operation-signature-gate-design.md` (ordering rules already drafted and sound;
  the envelope specification is the missing piece).

### T-016 – Transaction lifecycle and lock-state design

- Status: PAUSED
- Owner: ARCHITECT
- Paused on: T-015.
- Scope: Operation state machine, cancellation race semantics, conservative lock-state semantics,
  COM lifetime ownership, unload constraints.
- Output: `reports/operation-lifecycle-design.md` and test-plan stubs.

### T-004 – Plugin registration manager

- Status: PAUSED
- Owner: CODER
- Paused on: T-015, T-016.

### T-007 – Windows Hello user verification wrapper

- Status: PAUSED
- Owner: CODER
- Paused on: T-019 confirming a usable stable verification path.
- Review: SECURITY required.

### T-008 – Credential creation path

- Status: PAUSED
- Owner: CODER
- Paused on: T-015, T-016. Also requires T-005 and T-006.
- Review: SECURITY required.

### T-009 – Assertion path

- Status: PAUSED
- Owner: CODER
- Paused on: T-015, T-016. Also requires T-006, T-007, T-008.
- Review: SECURITY required.

---

## Completed or in review

### T-001 – Initialize repository and build baseline

- Status: IN_REVIEW (awaiting human owner sign-off)

### T-002 – Import and map Microsoft sample patterns

- Status: IN_REVIEW
- **Resolved header record:** `reports/sample-mapping.md` and
  `reports/webauthnplugin-contract.md` both record that the locked local SDK exposes
  `webauthnplugin.h`; the latter pins its path and SHA-256. Any future mismatch is a contract
  verification failure, not a reason to infer or recreate declarations.

### T-003 – COM plugin skeleton

- Status: IN_REVIEW

### T-013 – MSIX package and release checks

- Status: IN_REVIEW (bootstrap scope only)

### T-014 – Security review of the WebAuthn Plugin contract

- Status: IN_REVIEW — **approved with follow-ups**, blocked from `DONE` only by T-017.
- Approved scope: compiler-bound SDK/header contract integrity plus bootstrap COM lifetime.
- Explicitly **not** covered: signature-envelope semantics, request authentication, transaction
  state machines, cancellation races, registration, credential handling, signing, Hello, or v2.
- Note: the two HIGH findings belong to T-015 and T-016. Their open status is not a reason to hold
  T-014 open; the original completion criteria were circular and have been narrowed accordingly.

### T-012 – Final security review gate

- Status: PAUSED
- Paused on: the protocol lane.

---

## Prohibition (binding on all lanes)

No task may implement WebAuthn request decoding, cryptographic signing over real credential
material, Windows Hello invocation, plugin registration with Windows, credential metadata
mutation, or any `EXPERIMENTAL_`-prefixed API, until the protocol lane resumes and its tasks pass
security review.

Schema work, storage-engine work, key-protection work against random buffers, and UI work using
synthetic non-credential records are **permitted** and do not constitute credential storage.

Per ADR-001, a task instructing an agent to implement an `EXPERIMENTAL_` API is invalid. Refusing
it is correct behavior under `AGENTS.md`.
