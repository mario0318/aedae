# aeDae approval gate register

Date: 2026-09-11. Scope: current repository and live Gmail evidence. This register separates
technical AI review from decisions that only the human owner, a repository administrator,
Cloudflare, or Microsoft can make. A passing build is not treated as approval.

## Decisions obtained in this review cycle

| Task | Reviewer selected | Verdict | What the verdict covers | Remaining decision |
| --- | --- | --- | --- | --- |
| T-005 vault | Independent Codex security/bypass reviewer with Windows file-identity focus | **APPROVE_WITH_CONDITIONS; HUMAN APPROVED** | Exact reviewed hashes; synthetic vault records; tested local Windows filesystem; retained-handle rename and cleanup | Closed 2026-09-11. No real credentials, protocol/plugin integration, unusual filesystem, or power-loss claim is approved. |
| T-022 lifecycle design | Claude hostile security review focused on COM, WebAuthn and concurrency | **APPROVED; HUMAN APPROVED** | F1–F12 closed; no new findings or required changes; OL-001–OL-015 accepted as the pre-implementation test plan | Closed 2026-09-11. Protocol implementation still waits on T-019/T-015. |
| T-006 key protection | Independent Codex SECURITY reviewer with Windows CNG/DPAPI focus | **APPROVED; HUMAN APPROVED** | Current host/current user; random synthetic buffers; AES-256-GCM primitive with current-user DPAPI-wrapped data key | Closed 2026-09-11. No second-user, real credential, vault/plugin, Hello, signing, hardware-backing, or same-user-compromise claim. |
| T-020 website | Specialized UX/UI and accessibility reviewer | **DENY** | Current website candidate and its T-020 acceptance criteria | Repair validation, scope separation, accessibility and design-approval failures; then repeat AI review and obtain human PR approval. |
| T-001 baseline | Codex repository/build reviewer | **CONDITIONAL / NOT DONE** | Fresh Debug/Release builds, contract gates, COM/vault/mock tests and whitespace check pass | Architect resolves the missing `spec/` directory conflict; human approves a frozen narrow diff. |
| T-002 sample mapping | Codex architecture/readiness reviewer | **DENY** | Current sample mapping against the original acceptance criteria | Restore explicit criteria and add cancellation plus concrete test-point/adopted-pattern mapping. |
| T-003 COM skeleton | Codex Windows bootstrap reviewer | **NOT REVIEWABLE AS DONE / DENY** | Current IUnknown-only bootstrap and passing positive activation harness | Architect/human chooses IUnknown-only or official-interface scope; add negative COM conformance cases; then independent COM review and human approval. |
| T-013 MSIX packaging | Codex packaging-readiness reviewer | **DENY** | Current packaging script and documentation | Implement a real, deterministic unsigned package pipeline and review it with a Windows MSIX specialist; signing/publishing remain human-only. |
| T-014 contract baseline | Codex contract/governance reviewer | **CONDITIONAL FOR THE NARROW HISTORICAL SCOPE ONLY** | Previously reviewed SDK/header integrity and bootstrap COM lifetime | Freeze and security-review the current protected-script diff, reconcile the stale formal report, and complete T-017. |
| T-017 governance | Codex governance review plus human GitHub approval | **APPROVED; HUMAN APPROVED** | PR #1 merged as `254cdf6`; CODEOWNERS syntax clean; live protected `main`; PR #2 returned `BLOCKED` and `REVIEW_REQUIRED` | Closed 2026-09-11. Single GitHub identity requires controlled human-authorized admin bypass until a second trusted reviewer is added. |

## T-005 security approval details

The independent reviewer freshly rebuilt Release, ran the full wrapper, and reran both named
substitution commands. It confirmed:

- the same exclusive `DELETE`-capable temp handle is retained through validation, writes, flush,
  `FileRenameInfo`, and failure `FileDispositionInfo`;
- there is no pathname cleanup fallback;
- the canonical absolute rename described by ADR-002 matches the tested implementation;
- the three original attacks and five independent-process attacks across every commit caller are
  blocked; and
- all 12 CRUD/migration crash cases and legitimate recovery behavior pass.

Approval is bound to these SHA-256 values:

- `src/Vault/VaultStore.cpp`: `18C724B6F124CA231C58300D18C755454D7F649D4D412AD7BF0D6295E4D81149`
- `tests/unit/VaultStoreTests.cpp`: `FD0118890EF63B01DA53D629E5B4CE245591FD655C7136980B4BFBC5F994D492`
- `scripts/build.ps1`: `A132B719C65D1EA5A837746871F1E638B0D3455A176DDD0714B22DEB2C9522DC`

Conditions retained: foreign-owner integration remains descriptor-only because Windows rejected
fixture setup with error 1307; process-kill tests do not prove power-loss durability; unusual and
redirected filesystems are untested; failed handle disposition may leave the exact created temp for
explicit recovery. Human milestone approval remains separate.

Review packet and evidence: `T-005-final-review-packet.txt`, `T-005-handle-commit-fix.md`,
`T-005-claude-review-handoff.md`, current source/tests, and the `t005-handle-*` artifact logs.

## T-022 lifecycle approval details

The fresh hostile re-review returned **APPROVED**. It classified F1 through F12 as CLOSED and found
no new issue. In particular it accepted signature-first coordinator allocation, claim-then-publish,
caller/request-bound cancellation, lock ordering, per-boundary cancellation checks, tombstones,
counted callback lifetime, the build-time experimental-API prohibition, coarse lock status, fresh
per-transaction verification, state/callback deadlines, and unconditional zeroization/logging rules.

The cross-process-restart replay risk remains explicitly routed to T-019; this is not a hidden
approval of an unknown envelope. Evidence: `operation-lifecycle-design.md`,
`lifecycle-security-review.md`, `lifecycle-rereview-prompt.md`,
`../tests/integration/OperationLifecycleTestPlan.md`, `../scripts/ContractCheck.psm1`, and
`../security.md`.

## T-020 denial and required repair

The site builds 15 pages and currently contains no runtime JavaScript, forms, storage, analytics,
external fonts/assets, or user-data collection. Those passing checks do not meet the approval gate.

Approval is denied because:

1. `web/build.mjs` can accept an unsourced claim by pairing it with a later claim's source. The
   required unsourced-claim negative gate therefore fails its purpose.
2. The original T-020 commit also contains protocol reports and a broad `tasks.md` rewrite, violating
   the narrow website-only acceptance rule.
3. The current tree mixes T-020 with T-021 deployment files and a partial redesign governed only by
   unapproved `web/spec/07-house-of-human-capability-implementation-brief.md`.
4. Ledger status text has measured contrast below 4.5:1; mobile nav/wordmark targets are below 44px;
   and 10 of 15 non-home routes have no `h1`.
5. The current monospace editorial direction conflicts with the approved grotesque system in
   `web/spec/02-design-system.md` and does not fully implement the proposed replacement.

Required before reconsideration: validate claims at the structured data boundary with repeatable
negative tests; reconstruct a narrow T-020 review unit excluding T-021; obtain human approval for
spec 07 or revert to spec 02; fix contrast, touch targets and heading hierarchy; run responsive
screenshot QA and the full 15-page/no-JS/no-network/link/structure suite; then repeat UX and general
code review. Human PR approval remains mandatory.

## Bootstrap, contract and packaging dispositions

### T-001 — conditional

Fresh Debug and Release wrappers pass with zero build warnings/errors, and the contract gates, COM
harness, vault suite, mock-management suite and whitespace check pass. However `architecture.md`
requires a `spec/` directory while the specifications live at the repository root and `AGENTS.md`
grants roles paths under the missing directory. The source-of-truth conflict requires an Architect
and human decision. The unapproved SQLite directory should not be part of a baseline candidate.

### T-002 — denied

The current mapping records activation, registration, excluded create/assert behavior, metadata,
UI and SDK provenance, but omits cancellation and explicit test points required by the original
acceptance criteria. The ledger should restore the exact criteria before a new architecture review.

### T-003 — denied pending a scope decision

The current class implements `IUnknown`, while the original task expected a loadable
`IPluginAuthenticator` with safe nonfunctional stubs. The positive activation harness passes but
does not cover wrong CLSID/IID, null outputs, aggregation, or unmatched unlock. The owner/Architect
must choose the bootstrap scope before an independent Windows COM reviewer can approve it.

### T-013 — denied

`scripts/package.ps1` selects the first recursively found MakeAppx—10.0.19041 on this host despite
claiming 10.0.26100.7175 or newer—never invokes MakeAppx, and unconditionally throws. It does not
produce the unsigned developer package claimed by `packaging/README.md`. Assets, full-trust
extension validation, signing authorization, reproducibility, output hashes, and explicit Release
hardening are absent. Signing, publishing and production keys remain outside AI authority.

### T-014 historical narrow approval remains insufficient; T-017 is now closed

T-014's old review covers compiler-bound SDK/header integrity and bootstrap COM lifetime only. The
three protected contract scripts now have uncommitted post-review changes, including source scans,
and require a frozen security diff review. The formal contract report also contradicts the newer
task ledger and must be reconciled. The earlier T-017 denial is superseded: PR #1 installed
CODEOWNERS and the human-only AGENTS rule, live `main` protection is enabled, and disposable PR #2
proved an agent-authored protected-path change is `BLOCKED` and `REVIEW_REQUIRED`. T-014 still needs
its own frozen current-diff security review; closing T-017 does not approve those script changes.

A formal Codex Security diff scan was attempted with scope restricted to `scripts/`; the scanner
refused because working-tree scans require whole-repository scope. Running it against this broadly
dirty tree would mix unrelated approval units and could not legitimately approve T-014. No scan was
started or replaced. Freeze the protected scripts into an exact narrow revision before retrying the
formal scan.

## External and human-only gates

### T-019 Microsoft contract clarification

Live Gmail verification found the original email in Sent on 2026-09-04 with subject
`Normative verification contract for WEBAUTHN_PLUGIN_OPERATION_REQUEST`. It asks for the normative
v1 bytes, serialization, algorithms, bindings, replay freshness and v2 status. No reply from
`fido-dev@microsoft.com` was present as of 2026-09-11.

The sharper confirm-or-reject request in `fido-dev-email-ready.md` was sent in the same Gmail thread
on 2026-09-11 at 17:22 ET (message ID `1a092905b447e002`). It asks the precise cancellation-signature
question and tests the sample-observed construction. T-019 now waits only for Microsoft; no AI can
provide Microsoft's answer. After an answer, an Architect must resolve G2, the human owner must
accept G1 if Microsoft supplies no lifecycle guarantee, and Security must approve T-015.

### T-020, T-021, T-001 and milestone merges

- T-020 requires a human-reviewed narrow PR after the denial is repaired.
- T-021 temporary previews require human-owned Cloudflare authentication and live visual evidence.
  Any domain binding requires separate explicit human authorization.
- T-001 explicitly awaits human baseline sign-off after its architecture conflict is resolved.
- Every milestone merge inherits the human-owner approval requirement in `AGENTS.md`.

## Future reviews that are not ready yet

| Task | Future reviewer | Why it cannot be approved now |
| --- | --- | --- |
| T-006 DPAPI key protection | SECURITY, then human | `DONE`: SECURITY and human approved within random-synthetic-buffer scope. No real credential, vault/plugin, Hello, signing, registration, or release scope. |
| T-007 Hello wrapper | SECURITY, then human | Paused on T-019/T-015; no authorized implementation. |
| T-008 credential creation | SECURITY, then human | Paused on T-015/T-016 and depends on T-005/T-006. |
| T-009 assertion path | SECURITY, then human | Paused on T-015/T-016 and T-006/T-007/T-008. |
| T-012 final gate | SECURITY and human | Entire protocol lane is paused. |
| T-010 WinUI page | Dependency/build reviewer before implementation; UX reviewer after | Current work is mock console preparation, not a completed WinUI page; no pinned WinUI dependency/build plan exists. |
| T-011 identity health | Architecture/UX, then human | No implementation exists. |

## Approval integrity rule

The worktree contains unrelated modified and untracked files across vault, contract, lifecycle, UI,
website and deployment scopes. No reviewer should approve the entire working tree as one candidate.
Each approval must bind to a narrow task diff or exact hashes, and every post-approval change to a
protected or security-sensitive file invalidates the corresponding technical approval until a new
diff review.
