# ADR-001 — Experimental WebAuthn Plugin v2 APIs

- Status: **ACCEPTED**
- Date: 2026-08-29
- Deciders: Human owner (R3 Labs), Architect, Security
- Supersedes: nothing
- Related: T-014, T-015, T-016, `security.md`, `reports/contract-security-review.md`,
  `reports/operation-signature-gate-design.md`

## Context

`WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_RESPONSE` exposes an operation-signing public key
(`pbOpSignPubKey`, pinned at offset 8). The pinned public headers do **not** define the byte
sequence the platform signs over when it issues an operation request.

`reports/operation-signature-gate-design.md` states this directly: *"the public header does not
define that envelope in this repository."*

The T-015 signature gate is the project's central defense. It requires verifying a signature over
an exactly specified byte sequence. An undefined envelope makes that gate undesignable, not merely
difficult. T-015 therefore cannot be completed against the stable v1 surface as currently
documented.

The experimental v2 declarations (`EXPERIMENTAL_WebAuthNPluginAddAuthenticator2`,
`EXPERIMENTAL_WebAuthNPluginPerformUserVerification2`) were considered as a route around this.

## Decision

**Experimental v2 APIs are not adopted.** They remain compile-time prohibited. No task may
reference, link, probe, or conditionally compile against an `EXPERIMENTAL_`-prefixed symbol.

**The protocol lane is paused, not cancelled.** T-015, T-016, T-004, T-007, T-008, and T-009 are
blocked on an external dependency: a documented or reliably determined operation-signing envelope.

**The local lane proceeds.** T-005, T-006, T-010, and T-011 may be implemented against synthetic,
non-credential test data. They do not depend on the envelope question.

## Rationale

1. **v2 does not solve the problem, it relocates it.** The `EXPERIMENTAL_` prefix is the platform
   vendor reserving the right to change or remove the ABI without notice. A security product whose
   signature-verification path depends on such a symbol has no supportable release.

2. **v2 adds risk to the exact invariant being rescued.** Its user-verification request accepts a
   caller-supplied buffer to sign without hashing, and it changes CLSID and transaction-ID pointer
   conventions. `reports/contract-security-review.md` records this as an open MEDIUM. Adopting v2
   to fix a signature-binding gap would introduce a second, less reviewed one.

3. **The alternative — weakening the invariant — is refused.** Proceeding with an unverified or
   best-effort operation signature would make the plugin usable as a signing and UI-prompting
   oracle by any local caller. That is the threat the gate exists to stop.

4. **Pausing is cheap.** The local lane is roughly half the remaining v1 work and is entirely
   independent of the envelope.

## Consequences

- No shippable authenticator exists until the envelope is resolved. There is no date for this and
  it is not under project control.
- Any task instructing an agent to implement v2 is invalid on its face and must be refused. An
  agent refusal on these grounds is correct behavior under `AGENTS.md`, not a fault.
- `tasks.md` is restructured into two lanes so that external blockage on one does not present as
  global blockage.
- T-019 opens the envelope question with Microsoft. It is the critical path for the protocol lane.

## Revisit triggers

This ADR is reopened, and a fresh security review is required, if any of the following occur:

1. Microsoft documents the operation-signing envelope for stable v1.
2. The v2 APIs ship without the `EXPERIMENTAL_` prefix in a pinned SDK.
3. The human owner elects to build a **non-production, non-distributed** research prototype on
   v2. Such a prototype requires its own ADR naming exact SDK and Windows build bounds, must not
   be packaged or signed for release, and must not share a source tree with product code.

Absent one of these, the pause holds. It is not revisited by schedule pressure.
