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

**Protocol lane — still PAUSED on the external dependency, but the question is now precise.** The
pinned public headers do not define the operation-signing envelope. As of 2026-09-08 a *candidate*
construction is identified from the `PasskeyManager` sample — signature over the encoded request
bytes, SHA-256, key-blob-driven padding — but sample behaviour is not a contract, and T-015 requires
an exact contract-defined byte sequence. See
`reports/T-019-operation-signing-envelope-finding.md`. T-015 remains blocked until Microsoft
confirms or rejects the construction. Experimental v2 remains not adopted (ADR-001) and appears
unnecessary. **No protocol work is authorized while this lane is paused** — no plugin registration,
request decoding, Windows Hello invocation, credential operation, signing, or v2 invocation. What
changed on 2026-09-08 is only that T-019 is no longer open-ended research: it is one clarification
request with three questions. Preparing that request is documentation progress and does not satisfy
the missing authoritative-evidence requirement.

**Local lane — OPEN.** Vault, key protection, UI, and repository governance are independent of the
envelope question and proceed now against synthetic, non-credential data.

---

## Local lane (open)

### T-017 – Contract manifest governance

- Status: DONE — live protection, human-approved bootstrap, and rejection probe verified 2026-09-11
- Owner: PACKAGER
- Priority: **merge-blocker for T-014**
- Scope: Add `CODEOWNERS` requiring human-owner approval for the pinned ABI manifest and the three
  contract scripts. Enable branch protection on those paths. Add an `AGENTS.md` clause naming them
  human-approval-only and off-limits to every agent role.
- Rationale: The contract pin was centralized into one manifest file. Without governance the entire
  T-014 approval is defeatable by a single unreviewed edit.
- Done when: Protection is active, an agent-authored edit to a protected path is demonstrably
  rejected, and `AGENTS.md` names the paths.
- **2026-09-11 completion:** The human owner authorized live GitHub protection and approved PR #1;
  it merged as `254cdf6` through a controlled admin bypass, after which admin enforcement was
  immediately restored and verified. A first post-merge probe exposed that zero blanket approvals
  let the shared owner identity satisfy CODEOWNERS. The rule was strengthened to require one
  approval and approval by someone other than the latest pusher. Disposable PR #2 then reported
  `BLOCKED` and `REVIEW_REQUIRED` with no status checks. Evidence and the single-identity operating
  constraint are in `reports/T-017-governance-evidence.md`.

### T-005 – Vault schema and CRUD

- Status: DONE — security-approved with conditions and human-approved 2026-09-11
- Owner: CODER
- Scope: Credential database schema and `IVaultStore` implementation using **synthetic records
  only**, implemented as the versioned single-file store selected by ADR-002. No real key material,
  no DPAPI integration, no plugin or protocol coupling.
- Files: `src/Vault/VaultStore.*`, tests.
- Dependencies: ADR-002.
- Done when: Unit tests cover add, find, list, update-last-used, delete, schema migration, and
  corruption handling, migration failure, unknown-schema refusal, leftover-temp discard,
  restrictive-ACL refusal, and cross-process reader/writer synchronization, against fabricated
  non-key blobs. The tests prove a reader sees only the complete old or complete new snapshot.

> **Storage-engine decision.** ADR-002 selects a versioned single-file store. The plugin-owned
> vault service is the only writer; UI requests route to it and UI reads take a consistent snapshot.
> A path-derived named mutex spans each read-modify-commit sequence. Commit is sibling temporary
> file → flush → `FlushFileBuffers` → handle-bound `FileRenameInfo` replacement; leftovers are
> discarded, never merged. Unknown schema versions and ACLs writable beyond the current user fail
> closed. SQLite is not an approved dependency.

> **2026-09-09 remediation evidence (not completion).** Replaced shared error state with
> per-call results; added bounded RAII locking keyed to opened directory identity, normalized
> existing filenames, hardlink refusal, handle-based owner/DACL validation, explicit snapshot
> and writer operations, and an actual legacy-layout-to-v3 migration. The owner-approved ACL
> exception permits SYSTEM and Administrators alongside the current user. Directory-wide
> serialization is intentionally coarser than filename-scoped locking. Direct Debug and Release
> solution builds, each configuration's vault tests and COM harness, and independently invoked
> contract verification/negative guards all returned exit 0. Tests include two processes,
> actual short-directory and short-filename aliases, abandoned/timeout locks, malformed inputs,
> foreign writable file/directory ACLs with caller access preserved, and failed-rename cleanup.
> The normal build wrapper returned before compilation and is not counted as build evidence.
> T-005 remains IN_PROGRESS: independent security approval and additional adversarial/crash
> acceptance evidence are outstanding. See `reports/T-005-claude-review-handoff.md` for exact
> boundaries, unresolved cases, and reproducible commands. No real credentials or protocol work.

> **2026-09-09 adversarial continuation (supersedes the test gaps above).** Added 12 deterministic
> actual-commit crash cases across CRUD/migration, direct-directory and ancestor rename attempts,
> real file/temp symlink refusal, on-disk generic/inherit-only/deny-plus-allow ACL cases, and
> descriptor-level foreign-owner/null-DACL/callback/object-ACE tests. Reproduced and fixed a
> metadata-only directory-handle rename bypass (now requests FILE_LIST_DIRECTORY), and cleanup
> deleting a competing temp file after failed CREATE_NEW (now tracks successful creation).
> Repaired scripts/build.ps1's null/stale LASTEXITCODE early exit and included vault tests in it;
> isolated regressions prove both contract gates stop compilation on failure. Fresh Debug and
> Release wrapper runs returned 0 with all 12 crash cases and the temp collision regression.
> Windows refused the on-disk foreign-owner fixture with 1307; that case remains a documented
> integration limit, not a pass. Status remains IN_PROGRESS pending independent security review
> and human approval. Detailed evidence: reports/T-005-adversarial-validation.md.

> **2026-09-10 confirmed security blocker (historical; superseded by the verified candidate below).**
> The remaining source-path race was reproduced:
> substituted bytes or a symlink are committed with success, and failed-commit cleanup deletes a
> substituted temp file. --test-substitution returns 1 against the current store; ordinary green
> tests do not override this failure. A fresh read-only investigator and Claude packet review
> agree. Production code is unchanged pending owner approval to amend the explicit MoveFileEx
> requirement to handle-bound rename/cleanup. See reports/T-005-handle-commit-proposal.md.

> **2026-09-11 owner approval and implementation.** The human owner explicitly approved the
> handle-bound ADR amendment. Commit now retains the exclusive `DELETE`-capable temp handle through
> `FileRenameInfo`, with failure disposition applied to that same handle and no pathname fallback.
> Candidate bypass review and all ordered verification gates are now complete; evidence is in
> reports/T-005-handle-commit-fix.md. Status remains IN_PROGRESS pending formal independent security
> approval and human milestone approval.

> **Candidate review correction.** A fresh bypass reviewer found no surviving source-temp or
> pathname-cleanup substitution, but caught an ADR mismatch: the verified implementation uses an
> absolute canonical destination while retaining the pinned directory handle. Directory-relative
> RootDirectory returned Win32 87 and was rejected. ADR-002 now records the tested mechanism.

> **2026-09-11 verified candidate.** Normal Debug and Release wrappers pass all contract, build,
> COM, vault and mock-management gates. The focused Release substitution expectation blocks all
> three original attacks; an independent process cannot replace the live temp across provision,
> migration, add, update or delete, and each legitimate result parses with expected state. Twelve
> CRUD/migration crash cases, hooks-disabled Release, build-wrapper rejection fixtures and
> whitespace checks pass. Current hashes, commands and claim limits are in
> reports/T-005-handle-commit-fix.md. T-005 remains IN_PROGRESS pending formal independent security
> approval and human milestone approval; T-006 and real credential work remain unauthorized.
> A final Claude second-pass review returned no findings; this is corroborating review input, not
> the formal project security approval or human milestone approval.

> **Independent security disposition.** A fresh reviewer returned `APPROVE_WITH_CONDITIONS` for the
> exact hashes and synthetic local-filesystem limits in reports/T-005-handle-commit-fix.md. The
> technical SECURITY gate is satisfied. Human milestone approval remains required; T-006, DPAPI,
> real credentials, plugin/protocol integration, unusual filesystems and power-loss claims remain
> outside this approval.
>
> **Human milestone approval (2026-09-11).** The owner approved T-005 with the exact reviewed
> synthetic-data, local-Windows-filesystem and handle-bound-commit conditions above. This closes
> T-005; it does not broaden approval to real credential data or protocol/plugin integration.

### T-006 – DPAPI-backed key protection

- Status: DONE — SECURITY approved and human-approved 2026-09-11
- Owner: CODER
- Scope: `IKeyProtection` with DPAPI-backed wrapped storage, exercised with **random test buffers
  only**. No credential keys, no plugin coupling.
- Files: `src/Vault/KeyProtection.*`, tests.
- Dependencies: T-005.
- Done when: Protected data is unreadable at rest and wrong-context or tampered data fails closed.
- Review: SECURITY required.
- **2026-09-11 authorization:** Human owner approved T-005 and T-022 and explicitly authorized
  T-006 using random synthetic test buffers only. No credential keys, vault/plugin coupling,
  WebAuthn, Windows Hello, signing, registration, or production release is authorized.
- **Implementation evidence:** Current-user DPAPI wraps a fresh random AES-256-GCM data key for
  each call. The versioned envelope authenticates its header, complete wrapped-key blob, purpose,
  nonce, ciphertext and tag. Random synthetic buffers round-trip; duplicate protection differs;
  wrong purpose and wrong DPAPI entropy fail; one deterministic bit flip at every byte position,
  every truncation and trailing data fail closed. Debug and Release full wrappers and focused Release `/analyze` pass
  with zero warnings/errors. See `reports/T-006-key-protection-review.md`. No cross-user fixture,
  vault persistence, credential material, Hello proof or protocol integration is claimed.
- **Independent SECURITY disposition:** `APPROVE`, with no HIGH, MEDIUM or LOW blocking findings.
  Approval is limited to the current Windows host/current user, random synthetic buffers and this
  key-protection primitive.
- **Human milestone approval (2026-09-11):** The owner approved T-006 within the exact limits above.
  This does not authorize real credentials or vault/plugin/WebAuthn/Hello/signing integration.

### T-010 – Management UI: This PC

- Status: DONE — independently approved and human-owner accepted 2026-09-13; direct specialized
  visual/accessibility inspection remains a documented limitation
- Owner: CODER
- Scope: First WinUI page — provider state, Windows Hello availability, key-protection status,
  local credential count. Static or mock data sources only.
- Done when: It renders from the service layer with no protocol or credential dependency.
- Earlier preparation: `src/App.UI/MockThisPcService.h`, the `--mock-status` branch in
  `src/App.UI/main.cpp`, `tests/unit/ManagementModelTests.cpp` and
  `build/ManagementModelTests.vcxproj` established the fail-closed service boundary before the
  dependency gate opened. See `reports/T-010-mock-status-preparation.md`.
- **Dependency review completed (2026-09-13):** The authorized project-local restore authenticated,
  locked, audited and native-build-tested the 15-package `Microsoft.WindowsAppSDK` 2.4.0 closure.
  That broad meta-package is rejected for T-010 because it unnecessarily adds AI, ML, Windows AI
  Machine Learning, Search, Widgets and DWrite package references. A narrower framework-dependent
  component set — WinUI 2.3.6, InteractiveExperiences 2.1.6 and Runtime 2.4.0 — resolves eight
  packages and passed a fresh-cache locked restore and native MSBuild evaluation/build. See
  `reports/T-010-windows-app-sdk-dependency-review.md` and its hash manifest.
- **Dependency gate approved (2026-09-13):** The owner accepted the exact eight-package closure and
  its Microsoft binary-package license/notice obligations. Direct use of the broad meta-package
  remains rejected. This authorizes the separate framework-dependent WinUI application project with
  synthetic data only; it does not authorize runtime installation, package registration, signing,
  publishing, or any plugin/vault/Hello/credential integration.
- **WinUI implementation (2026-09-13):** `build/AeDaeManagementApp.vcxproj` and
  `src/App.UI/ManagementApp.cpp` now provide a framework-dependent, unpackaged C++ WinUI window.
  It consumes `MockThisPcStatusService`, labels all values as simulated or not queried, exposes
  automation names, scrolls at constrained sizes, disables credential management, and refuses a
  non-synthetic snapshot. The reviewed lock is project-scoped at
  `build/AeDaeManagementApp.packages.lock.json`; its SHA-256 remains
  `82C45DE659CDF37F7164170B426DF75BBF952B93F77CBF592098EC117824724D`.
- **Verification:** Locked restore passed. Full Debug and Release `scripts/build.ps1` runs passed the
  contract verifier and negative guard, all solution builds with zero warnings/errors, COM
  activation, vault crash/concurrency/substitution tests, management-model tests, and synthetic
  DPAPI tests. The Release executable launched into a responsive top-level window titled
  `aeDae — This PC`. No registration, signing, publishing, protocol call, vault access, credential
  operation, Windows Hello query, or key operation occurred. See
  `reports/T-010-winui-implementation.md`.
- **Independent review follow-up (2026-09-13):** `APPROVE WITH NON-BLOCKING FINDINGS`. Confirmed
  theme-resource, accessible grouping, refusal-provenance and deterministic-test gaps were repaired.
  All colors now resolve through WinUI theme resources with page rebuild on theme change; status and
  warning cards use content-view automation peers; live refusal excludes mock provenance; expanded
  tests cover presentation refusal and invalid location metadata. Debug and Release full wrappers
  passed again with zero build warnings/errors. Direct dark/high-contrast, scale, narrow-width and
  screen-reader inspection plus owner review remain. See
  `reports/T-010-T-011-qwen-review-remediation.md`.
- **Independent rereview (2026-09-13):** `APPROVE`; no findings remain in the remediated source.
  The reviewer confirmed all four prior findings closed, the theme-change recursion guard effective,
  the lock/dependency closure unchanged, the security boundary intact, and the remaining external
  inspection limits accurately reported. See `reports/T-010-T-011-qwen-rereview.md`.
- **Human milestone approval (2026-09-13):** The owner accepted the final T-010/T-011 synthetic UI
  milestone with the direct dark/high-contrast, 200% scale, narrow-width and screen-reader checks
  explicitly retained as unperformed limitations. This approval does not authorize package
  registration, signing, publishing, protocol work, Windows Hello, vault, credential or key access.

### T-011 – Management UI: Identity Health

- Status: DONE — independently approved and human-owner accepted 2026-09-13; direct specialized
  visual/accessibility inspection remains a documented limitation
- Owner: CODER
- Scope: Totals, local-only credentials, duplicate candidates, and stated limits of foreign-provider
  visibility. Static or mock data only.
- Done when: Every displayed warning links to its computed rule and source data.
- Current preparation: `src/App.UI/IdentityHealthModel.h` computes totals and local-only,
  single-known-authenticator and same-RP/account duplicate-candidate warnings from synthetic metadata.
  Every warning carries a stable rule ID plus its source credential references; invalid, ambiguous or
  non-synthetic input produces no summary. Tests cover known zero, totals, all three rules, provenance,
  visibility limits and refusal paths. `src/App.UI/ManagementApp.cpp` projects those totals and every
  rule/source link into the synthetic-only management window, preserves the incomplete-visibility
  disclaimer, and fails closed for a refused result. Post-projection Debug and Release full wrappers
  passed with zero build warnings/errors, and the Release window launched responsively. See
  `reports/T-011-identity-health-model.md`. This makes no claim about live or complete
  foreign-provider visibility.
- **Independent review follow-up (2026-09-13):** Warning cards now expose one combined accessible
  name through a `ContentControl` automation peer. The presentation guard and expanded tests prove
  live/invalid results cannot be projected, every local-only fixture record is covered, and empty
  provider/device/location inputs fail closed. Full Debug and Release wrappers passed after repair.
- **Independent rereview (2026-09-13):** `APPROVE`; no findings remain. Direct visual/UIA inspection
  was retained for human disposition as the only remaining T-011 evidence gap.
- **Human milestone approval (2026-09-13):** The owner accepted T-011 together with T-010 under the
  documented inspection limitation and unchanged synthetic-only boundary.

### T-018 – Contract verifier follow-ups

- Status: IN_PROGRESS — CI prepared; hosted SDK drift blocks a green independent run
- Owner: CODER
- Scope: Close the non-blocking findings from the T-014 review.
  - **MED-02** — a stabilized (de-prefixed) v2 symbol currently surfaces as
    `Missing required WebAuthn Plugin API symbol`, i.e. the most security-significant possible
    upstream change is indistinguishable from a broken toolchain. Emit a distinct, loud diagnostic.
  - **MED-03** — no CI. Every build and test result to date is implementer-reported. Stand up a
    pipeline that reproduces build, contract verification, and tests independently.
  - **MED-04** — replace `if (-not $?)` error handling with construction that fails by design
    rather than by accident of every failure being a `throw`.
  - **MED-05** — nothing scans `src/`, `build/`, or `artifacts/` for a reference to an
    `EXPERIMENTAL_` symbol, so ADR-001's compile-time prohibition has no build-time check. Add the
    scan to `scripts/ContractCheck.psm1` and a negative fixture case to
    `scripts/test-webauthnplugin-contract-guard.ps1`. Source: `reports/lifecycle-security-review.md` F8.
    **DONE 2026-09-02.** `Invoke-AeDaeSourceCheck` scans `src/` recursively plus `*.map`,
    `*.vcxproj`, `*.props`, `*.def` under `build/` and `artifacts/`. Demonstrated: a probe file
    written to `src/Common/` fails `verify-webauthnplugin-contract.ps1` with
    `Prohibited EXPERIMENTAL_ API reference (ADR-001)`, and both scripts pass on the clean tree.
    MED-02, MED-03, and MED-04 remain open.
- Done when: Each finding has a test or pipeline artifact demonstrating closure.
- **2026-09-11 continuation:** PR #3 adds a pinned, least-privilege Windows CI matrix, repairs the
  unprotected build wrapper's ambient `LASTEXITCODE` handling, and adds two rejection fixtures.
  Isolated local Debug/Release contract, build and COM gates pass with zero warnings/errors. The
  current-head hosted jobs fail closed before compilation because GitHub's mutable `windows-2025`
  SDK exposes `webauthnplugin.h` hash `91E7218...`, not reviewed hash `8B8897A...`, despite using the
  same `10.0.26100.0` directory. Do not weaken or silently repin the manifest. PR #3 includes
  `reports/T-018-protected-amendment-proposal.md` for human-authored MED-02/MED-05 changes and the
  three acceptable SDK-source decisions. T-018 is not done while CI is red and the protected
  amendments remain unapproved.

### T-022 – Lifecycle design revision (closes lifecycle review F1–F7, F9–F12)

- Status: DONE — security-approved and human-approved 2026-09-11
- Owner: ARCHITECT
- Priority: high — pays down T-016's design debt while the protocol lane waits on T-019.
- Scope: Revise `reports/operation-lifecycle-design.md` so that every finding in
  `reports/lifecycle-security-review.md` except F8 is closed. Apply each finding's `Fix:` line
  literally; add nothing beyond it. Add sections `Synchronization`, `Authenticated cancellation`,
  `Timeouts and limits`, and `Logging and zeroization`. End with an appendix table
  `Finding -> Section`. Keep the file under 300 lines and design-only: no code, no envelope bytes,
  no Hello, vault, or crypto detail. Update `tests/integration/OperationLifecycleTestPlan.md` rows
  OL-001 through OL-007 with the extended evidence each finding's `Test:` line names; rows OL-010
  through OL-013 are already present.
- Files: `reports/operation-lifecycle-design.md`, `tests/integration/OperationLifecycleTestPlan.md`.
- Dependencies: none. This task does not depend on T-015 or T-019: none of the findings concern the
  signed byte sequence.
- Model guidance: a mid-tier model is sufficient. Supply only the two files above plus
  `reports/lifecycle-security-review.md` and `reports/operation-signature-gate-design.md`.
- Done when: every F-ID except F8 appears in the appendix table pointing at a real section; the
  design still forbids implementation in its `Out of scope` section; a Security re-review of
  `reports/lifecycle-security-review.md` records each finding closed.
- Review: SECURITY required.
- **Status note (2026-09-02):** first revision written and re-reviewed. The re-review returned
  APPROVED_WITH_REQUIRED_CHANGES with four findings (R1 HIGH, R2–R4 MEDIUM); all four were applied.
  The document is 280 lines — the original 200-line guidance was raised to 300 because R1's
  correction and the R3 leak policy needed the space, not because of prose bloat. Awaiting human
  owner sign-off. One residual risk is deliberately left open and routed to T-019.
- **Fresh approval check (2026-09-11):** hostile Claude re-review returned `APPROVED`; F1–F12 are
  CLOSED, with no new findings, required changes or additional pre-implementation tests. This
  confirms the technical design gate only. Human sign-off and the T-019/T-015 implementation pause
  remain.
- **Human design approval (2026-09-11):** the owner approved T-022 and accepted its documented
  residual risk. This closes the design revision only; lifecycle implementation remains paused on
  T-019/T-015.

### T-020 – aeDae public website (`web/`)

- Status: IN_REVIEW — denial repaired in isolated candidate; repeat independent review and human approval required
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
- **2026-09-11 review disposition:** DENIED. The unsourced-claim negative gate can accept an
  unsourced claim; the candidate mixes T-020, T-021 and an unapproved partial redesign; the original
  commit is not narrowly scoped; and contrast, touch-target and heading-hierarchy defects remain.
  Exact repairs and passing/non-passing evidence are in reports/approval-gate-register-2026-09-11.md.
- **2026-09-13 repair candidate:** `cx/t020-narrow-review` was reconstructed from the pre-website
  parent as a web-only two-commit review unit ending at `ffd98a3`. Structured claim validation now
  rejects the adjacent-source false negative, all 15 routes require exactly one `h1`, status and
  accent contrast gates cover light and dark modes, and navigation targets require at least 44 px.
  The focused suite passes 45 assertions; the build reports 15 pages across 5 surfaces; and
  `git diff --check 335d28c..ffd98a3` is clean. Playwright checks at 1440, 1024 and 390 px covered
  tech, world and online; the 390 px live measurement reported no horizontal overflow, 45–46 px
  nav targets, one `h1`, and zero console errors. The candidate uses approved spec 02 and excludes
  T-021 deployment files and proposed spec 07. This is implementation evidence, not the required
  repeat independent UX/general review or human merge approval.

### T-021 – Cloudflare Pages temporary preview deployment

- Status: IN_PROGRESS
- Owner: PACKAGER
- Scope: Prepare five independent Cloudflare Pages preview deployments for the
  `web/` surfaces using `web/deploy/`. Publish only temporary `*.pages.dev`
  URLs for `aedae-tech`, `aedae-world`, `aedae-life`, `aedae-live`, and
  `aedae-online`.
- Files: `web/build.mjs`, `web/README.md`, `web/spec/05-repo-and-deployment-plan.md`,
  and `web/deploy/` only.
- Prohibited: No custom-domain binding, DNS, registrar, email, analytics,
  CDN, or production-domain changes. No credentials in source or commands.
- Dependencies: T-020 build output; human-owned Cloudflare authentication on
  the operator's machine.
- Done when: Each Pages project deploys successfully, each temporary URL
  returns the expected homepage and stylesheet, all five surfaces are visually
  checked, and no DNS or custom-domain operation has occurred.
- Review: Human owner approval is required before any domain-binding step.
- **2026-09-13 local packaging repair:** `web/deploy/prepare.ps1` now produces five self-contained
  preview bundles, each with its selected homepage at `/`, the shared stylesheet, security headers,
  and all five root-relative surface routes. Two consecutive local preparations succeeded; each
  bundle contained 18 files and all required routes. A locally served tech bundle returned HTTP 200
  for `/`, all five surface roots and `/assets/styles.css`; Playwright followed its world navigation
  with zero console errors. No Wrangler authentication, Pages project creation, upload, DNS, custom
  domain or other network operation was performed. T-021 remains IN_PROGRESS pending separately
  authorized live preview deployment and visual verification of the resulting `*.pages.dev` URLs.

---

## Protocol lane (paused — external dependency, now precisely posed)

### T-019 – Resolve the operation-signing envelope

- Status: IN_PROGRESS — research complete, awaiting upstream answer
- Owner: ARCHITECT
- Investigated 2026-09-08. Output: `reports/T-019-operation-signing-envelope-finding.md`.
- Scope: Determine the exact byte sequence the platform signs for plugin operation requests. Open
  an issue against `microsoft/webauthn`, and in parallel record what the Microsoft Passkey Manager
  sample actually constructs and verifies.
- Candidate finding (**not contractual**): the sample verifies over exactly
  `[pbEncodedRequest, pbEncodedRequest + cbEncodedRequest)` — the raw CTAP2 CBOR request, no framing,
  no other struct field. Digest SHA-256 supplied as a hash to `NCryptVerifySignature`; padding
  selected from the key blob's `Magic` (`BCRYPT_PAD_PSS`, SHA-256, salt 32 for RSA). Observed in the
  `PasskeyManager` sample; absent from the pinned headers and from Microsoft documentation.
- **Neither "Done when" branch is met.** The envelope is not documented well enough for T-015 to
  bind to, and it is not established as unobtainable either — it is unconfirmed. The task closes when
  Microsoft confirms or rejects the construction.
- Open gaps, all requiring upstream answers rather than local decisions:
  - **G1** the envelope binds no transaction identity — `transactionId`, `hWnd` and `requestType`
    are all outside the signature, so a signed request is replayable into any transaction. This
    widens R1: there is not merely no freshness value, there is no transaction binding at all.
    G1 does **not** close on an answer that merely assigns replay defence to the plugin. A
    retired-transaction-ID set only rejects replays it still remembers, so it fails across process
    restart, crash, and fresh instances. Closing G1 additionally requires documented uniqueness and
    lifetime semantics for `transactionId`, or a platform freshness value, or documented scoping of
    requests to an instance/process/session/registration. Otherwise it stands as a limitation
    requiring explicit human risk acceptance.
  - **G2** the cancellation struct carries a signature but no message to sign, and the reference
    sample never reads it. T-015's "equivalent authentication" rule is unimplementable as written
    and must be re-decided.
  - **G3** the construction is sample-derived, not normative. It can drift without a header change,
    which the contract guard cannot catch. Verification failure must be reported loudly as possible
    upstream drift.
- Communication status verified in Gmail: the original clarification was sent on 2026-09-04, and
  the sharper confirm-or-reject follow-up—including the precise G2 cancellation and G1 replay/
  transaction-binding questions—was sent in the same thread on 2026-09-11 at 17:22 ET. Gmail
  message ID: `1a092905b447e002`. No Microsoft reply was present before the follow-up. The only
  remaining T-019 dependency is Microsoft's upstream response.
  - Send-ready text: `reports/fido-dev-email-ready.md` (subject + body, paste into mail).
  - Full context and evidence boundary: `reports/microsoft-v1-envelope-clarification-request.md`.
  - Channel, verified 2026-09-08: email to `fido-dev@microsoft.com` was treated as the only sanctioned
    route. The `microsoft/webauthn` README names that address for API-adoption and clarification
    questions, and the repository has GitHub **issues disabled** (`has_issues=false`; `gh issue list`
    refuses). The GitHub-issue route on that repo is withdrawn, not deferred.
  - Superseded 2026-09-17: with no reply after both emails, opened
    [microsoft/Windows-classic-samples#431](https://github.com/microsoft/Windows-classic-samples/issues/431)
    against the `PasskeyManager` sample directly (the same three questions, condensed). That repo has
    issues enabled where `microsoft/webauthn` does not, so it is a second channel, not a substitute for
    the email thread — the email stays open pending reply.
  - Not to be filed as a vulnerability report. This is a contract, documentation and supportability
    question; no exploitable defect has been demonstrated.

### T-015 – Operation signature-verification gate design

- Status: PAUSED
- Owner: ARCHITECT
- Paused on: T-019. A candidate envelope exists but is sample-observed. This task's completion
  condition requires a gate bound to an official envelope with no fallback behaviour, which an
  unconfirmed construction cannot satisfy.
- Scope: Fail-closed ordering for authenticating platform operation and cancellation requests
  before all protocol side effects.
- Required once T-019 returns an answer: specify verification over the confirmed byte sequence; rule
  on G2 (either specify the cancellation envelope and fail closed, or downgrade cancellation to an
  explicitly unauthenticated non-destructive signal); record G1's disposition, which is a human risk
  acceptance unless Microsoft documents a lifecycle guarantee; require a loud distinct diagnostic
  for G3 drift.
- Output: `reports/operation-signature-gate-design.md`. Its ordering rules, failure behaviour and
  implementation boundary remain drafted and sound; only the envelope specification is missing.
- Note: the `PasskeyManager` sample is **not** a security reference for this task. It decodes before
  verifying, raises Hello UI regardless of the verdict, never consults the stored result, and
  fails open when the key is unavailable. See the finding's "The sample is not a security
  reference" section.

### T-016 – Transaction lifecycle and lock-state design

- Status: PAUSED
- Owner: ARCHITECT
- Paused on: T-015 (implementation). The design itself was reviewed 2026-09-02 and returned
  BLOCKED — see `reports/lifecycle-security-review.md`. Its findings are closed by T-022 and
  T-018 MED-05 in the local lane, neither of which waits on T-019.
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
- Paused on: T-019/T-015. No Windows Hello invocation is authorized while the protocol lane is
  paused. The 2026-09-08 investigation narrowed *why* this is paused but does not release it: the
  evidence for the UV path is sample observation on the same footing as the operation envelope.
- A stable verification path appears to exist on the API surface. Stable v1
  `WebAuthNPluginPerformUserVerification` returns a response that the sample verifies as a signature
  over the encoded request buffer, using `WebAuthNPluginGetUserVerificationPublicKey`. The
  experimental v2 `pbBufferToSign` field therefore appears unnecessary, so ADR-001 costs nothing
  here. Source: `reports/T-019-operation-signing-envelope-finding.md`.
- Same epistemic caveat as T-019: the UV response envelope is sample-observed, not documented. Do
  not treat it as contractual. The implementation must verify and fail closed, so an envelope
  mismatch surfaces as a refused operation rather than a silent bypass.
- Requirement beyond the sample: the UV response signature must be verified and **gated on**. The
  sample stores the verdict without acting on it and skips UV entirely when its mock vault is
  already unlocked. Neither behaviour is acceptable here.
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
- Owner: ARCHITECT
- Scope: Document which Passkey Manager sample components are adopted, adapted, or intentionally
  excluded. Mapping work is documentation only and authorizes no protocol implementation.
- Output: `reports/sample-mapping.md`.
- Done when: The architecture identifies plugin activation, registration, authenticator operations,
  cancellation, concrete test points, and the SDK/API-version gate; every pattern is classified as
  adopted, adapted, or excluded; and planned tests are not presented as executable evidence.
- **Resolved header record:** `reports/sample-mapping.md` and
  `reports/webauthnplugin-contract.md` both record that the locked local SDK exposes
  `webauthnplugin.h`; the latter pins its path and SHA-256. Any future mismatch is a contract
  verification failure, not a reason to infer or recreate declarations.
- **2026-09-13 denial repair:** The original acceptance criteria are restored above. The mapping now
  covers all six required areas, explicitly excludes the sample's fail-open signature ordering,
  transaction-ID-only cancellation, mock credential state, UV bypass and operation bodies, and
  distinguishes current executable/build-only evidence from OL-001 through OL-015 future test
  specifications. T-002 remains IN_REVIEW pending a fresh architecture/readiness review and human
  disposition; no protocol lane task is unpaused.

### T-003 – COM plugin skeleton

- Status: IN_REVIEW

### T-013 – MSIX package and release checks

- Status: DONE — deterministic unsigned bootstrap candidate verified, Claude-approved, and
  human-approved 2026-09-13
- Owner: PACKAGER
- Scope: Produce and validate a deterministic, unsigned x64 developer MSIX for the bootstrap
  status application only. Signing, installation, publishing, production identity/assets,
  plugin registration, protocol operations, credentials, and Windows Hello remain prohibited.
- Done when: two complete Release build/package runs produce byte-identical packages; MakeAppx
  successfully unpacks both exact outputs; required entries, developer-asset dimensions, manifest
  semantics, Release hardening, and absence of `AppxSignature.p7x` are verified; negative version
  and output-path tests fail closed; an independent packaging review and the human owner approve
  the candidate.
- **2026-09-13 repair evidence:** repeated full Release gates and MakeAppx packing passed, and all
  payload hashes matched, but MakeAppx's current-time ZIP metadata broke reproducibility. A .NET
  archive rewrite made hashes match while MakeAppx unpack rejected the result with `0x80511007`
  because its required data-descriptor layout was lost. The safer in-place patch discovered that
  MakeAppx emits ZIP64 sentinels and failed closed rather than guessing. The autonomous three-cycle
  repair limit is now reached. `reports/T-013-packaging-blocker.md` records the exact evidence and
  bounded ZIP64-aware next repair. At that point no generated package was accepted as a candidate.
- **2026-09-13 authorized resolution:** the ZIP64-aware in-place timestamp normalizer now preserves
  MakeAppx's data descriptors. Two complete Release build/package runs produced byte-identical
  18,509-byte packages with SHA-256
  `192871d6839a243343ab1e56e70c52d0dc94e2901f16603b03dcf0006d318fb6`. MakeAppx
  10.0.26100.7705 successfully unpacked both exact outputs; all seven entry hashes, normalized
  timestamps, asset dimensions, manifest semantics, sidecars, signature absence and PE hardening
  checks passed. Invalid versions and external output paths failed closed. Evidence and reviewer
  questions are in `reports/T-013-packaging-review.md`. At that point T-013 remained `IN_REVIEW`
  pending independent packaging review and human approval.
- **2026-09-13 independent review:** Claude's first pass found two medium defects: non-atomic
  timestamp finalization and an EOCD false-positive gap. Both were fixed with same-directory
  temporary output plus `File.Replace` recovery backup and an exact EOF/comment-length check.
  Adversarial probes confirmed a forced temporary-write failure preserves the original hash and a
  fake EOCD signature inside a valid comment is ignored. Two fresh full Release/package runs still
  produced SHA-256 `192871d6839a243343ab1e56e70c52d0dc94e2901f16603b03dcf0006d318fb6`,
  and MakeAppx unpacked both. Claude's second pass returned `[]` and closed both medium findings,
  with no critical, high, or new medium issue. At that point only final human approval remained.
- **2026-09-13 human approval:** the owner approved the final post-review candidate. This closes
  T-013's unsigned developer-bootstrap scope only. Signing, installation, publishing, production
  identity/assets, plugin registration, protocol operations, credentials, and Windows Hello remain
  separately gated and are not authorized by this approval.

### T-014 – Security review of the WebAuthn Plugin contract

- Status: IN_REVIEW — historical narrow review only; current protected-script diff is not yet
  approved. T-017 is closed.
- Approved scope: compiler-bound SDK/header contract integrity plus bootstrap COM lifetime.
- Explicitly **not** covered: signature-envelope semantics, request authentication, transaction
  state machines, cancellation races, registration, credential handling, signing, Hello, or v2.
- Note: the two HIGH findings belong to T-015 and T-016. Their open status is not a reason to hold
  T-014 open; the original completion criteria were circular and have been narrowed accordingly.
  T-017's governance/rejection requirement is now satisfied. Before T-014 can be approved, freeze
  and independently review the exact current protected-script diff and reconcile the stale formal
  report. Then obtain human approval for the limited contract-integrity/bootstrap-lifetime scope.
- **Frozen review input (2026-09-11):** `reports/T-014-protected-diff-review-packet.md` records the
  exact SHA-256 of all four protected files, the 30-insertion/two-deletion diff, local Debug/Release
  evidence, hosted-CI evidence boundary and five required adversarial review questions. It is a
  packet, not an approval; the protected candidate must not be changed or merged by an agent.

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
