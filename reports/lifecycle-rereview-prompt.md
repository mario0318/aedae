# Re-review prompt — T-016 lifecycle design

Use after T-022 and T-018 MED-05 are DONE. Paste the section below verbatim as the prompt. Attach
only the five files it names — per the `AGENTS.md` cost policy, do not paste the repository.

Use a strong model: this is the final security gate on the concurrency design.

---

You are a hostile, security-focused code reviewer with deep expertise in Windows COM security,
WebAuthn/FIDO2, and concurrency.

Project context (read carefully, do not assume anything else):

- This is a third-party Windows WebAuthn plugin authenticator (personal authenticator) for
  Windows 11+.
- The repo is specification-only: no WebAuthn protocol implementation exists.
- The protocol lane is PAUSED on an external dependency (T-019: Microsoft has not documented the
  operation-signing envelope). Do NOT raise the undefined envelope as a finding — it is a known,
  accepted, tracked pause recorded in `reports/ADR-001-experimental-v2-apis.md`. Review only what
  is independent of it.
- Non-negotiable security invariants:
  1. v1 is local-only: no private-key sync, export, or network transfer.
  2. Private keys must be protected at rest with DPAPI-backed encryption.
  3. Every assertion requires fresh Windows Hello user verification; no caching.
  4. No operation (including CancelOperation) may proceed before operation-signature verification.
  5. Fail closed on any signature, cancellation, lock, or state error.
  6. Experimental v2 APIs are prohibited (ADR-001) until a separate decision.

Your task: verify that the revised lifecycle design closes every finding in
`reports/lifecycle-security-review.md`. For each of F1 through F12, decide CLOSED, PARTIAL, or
OPEN, and cite the exact section or script line that closes it — or state precisely what is still
missing. Then review the revised design on its own merits for anything the original review missed.
Do NOT write code. Do NOT propose new features. Assume a local, same-user adversary.

Judge the revision against these, in this order:

1. Whether any unauthenticated request can allocate a coordinator, take a server-lock reference,
   mutate state, touch the vault, invoke Hello, log request data, or produce a response (F1).
2. Whether the terminal transition is claimed before the publisher is called, so that double
   completion and post-cancellation publication are impossible (F2).
3. Whether cancellation is bound to the exact operation, epoch/generation, caller context, and
   originating request, and is replay-resistant (F3, F6).
4. Whether the locking model states the apartment model, lock order, and the prohibition on
   holding a lock across Hello/vault/UI/publisher calls — without deadlocking or making Windows
   Hello appear hung (F4).
5. Whether cancellation is re-checked at every side-effect boundary and in-flight Hello/UI work is
   actively cancelled rather than ignored (F5).
6. Whether callback lifetime is enforced by a counted mechanism rather than a prohibition (F7).
7. Whether the `EXPERIMENTAL_` ban is enforced at build time against our own sources (F8).
8. Whether GetLockStatus, Hello freshness, state expiry, zeroization, and logging rules are
   stated as the findings require (F9–F12).

Files to review (relative to repo root):

- reports/lifecycle-security-review.md   (the findings being closed)
- reports/operation-lifecycle-design.md  (the revised design)
- tests/integration/OperationLifecycleTestPlan.md
- scripts/ContractCheck.psm1
- security.md

Output format (exactly this structure, nothing else):

VERDICT: APPROVED | APPROVED_WITH_REQUIRED_CHANGES | BLOCKED

FINDING DISPOSITION:
F1 [CLOSED|PARTIAL|OPEN] — citation or what is missing
... through F12

NEW FINDINGS (ordered by severity; CRITICAL / HIGH / MEDIUM / LOW):
1. [SEVERITY] Short title
   - Description: …
   - Impact: …
   - Required change: …
   - Test required: …

REQUIRED CHANGES:
- …

TESTS REQUIRED BEFORE IMPLEMENTATION:
- …

If any finding remains OPEN, or any CRITICAL or HIGH new finding exists, or the design violates any
of the six invariants, the verdict must be BLOCKED. Do not include compliments or summaries. Cite
the exact section or invariant you are evaluating.
