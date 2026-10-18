# ADR-003 proposal: Windows Hello relying-party track

Status: LOCAL EXPERIMENT VERIFIED — not a production or plugin approval
Date: 2026-09-28

## Decision needed

The current product contract (`functional.md` and `architecture.md`) defines aeDae as a
system-wide Windows WebAuthn **plugin authenticator**. A conventional WebAuthn relying
party (RP) is the **site or app requesting** a passkey. It can use Windows Hello as an
available platform authenticator, but cannot make aeDae a selectable provider for
unrelated sites or enumerate/manage Windows Hello's private keys. This route cannot
silently replace the plugin product or satisfy T-004, T-007, T-008, or T-009.

The current `tasks.md` prohibition applies to **all lanes** and expressly forbids
Windows Hello invocation while the protocol lane is paused. A real RP ceremony is
therefore not authorized by the existing task contract, even if browser-mediated.

The owner chose option 1 on 2026-09-28. These remain the distinct product choices:

1. **Parallel proof of concept (recommended initially):** Keep the plugin product
   paused and add a separately labeled, local-only RP experiment to validate a
   usable Windows Hello journey on an aeDae-controlled origin. This does not count
   as a passkey-provider launch.
2. **Product pivot:** Amend `functional.md`, `architecture.md`, `security.md` as
   needed, the task ledger, public claims, and release criteria to make aeDae an
   RP-facing product instead of a system-wide authenticator. This is a new product
   decision, not a workaround that preserves the original user stories.

## Proposed parallel experiment boundary

This authorization does not itself instruct an automatic real credential ceremony.
The corresponding `tasks.md` exception covers only a standard browser-mediated `navigator.credentials.create`
and `navigator.credentials.get` on a loopback origin. The exception must not
unpause the plugin/protocol lane or authorize direct Windows Hello plugin calls.

- Own files under `experiments/hello-rp/` and focused tests only; do not modify
  `src/PluginAuthenticator/`, `src/Vault/`, `web/`, release packaging, or any of the
  four human-only ABI contract files.
- Use a reviewed, pinned WebAuthn RP verification library. Do not implement
  attestation parsing, COSE signature verification, cryptographic primitives,
  credential private-key storage, or a substitute user-verification prompt.
- Bind only to loopback. Use a single exact local origin and RP ID, random
  one-use server-side challenges, and explicit origin/RP ID verification.
- Use synthetic test accounts and ephemeral server-side **public** credential
  records only. No production account, telemetry, cloud service, external secret,
  persistent passkey vault, or aeDae-controlled private key.
- Registration and sign-in require an explicit user gesture in the demo UI.
  Automated tests use a virtual authenticator and cannot be reported as real
  Windows Hello validation. Any real Hello prompt is owner-initiated.
- Do not present this as system-wide passkey provider functionality or evidence
  that the Microsoft operation-signing envelope is resolved.

## Proposed acceptance evidence

1. The RP rejects a replayed challenge, wrong origin, wrong RP ID, wrong
   credential, canceled ceremony, and cross-session response.
2. An automated browser test with a virtual authenticator registers then
   authenticates on the loopback origin without custom signing code.
3. The owner separately confirms or declines a manual Windows Hello test;
   record its result distinctly from the virtual-authenticator test.
4. A security reviewer checks dependency pinning, challenge/session handling,
   parser/verification boundaries, local exposure, and demo-only claims.
5. The plugin protocol lane remains PAUSED; T-019 and the Microsoft contract
   question remain open unless independently resolved.

## Integration and launch consequence

An RP can protect only aeDae-controlled sign-in surfaces. It does not give
users a provider they can select at arbitrary third-party websites and cannot
manage those sites' Windows Hello credentials. A useful owner-controlled
application could launch through this route, but a product pivot requires
new functional requirements, hosting/account/privacy decisions, threat model,
and release review. No shipping or deployment is approved by this proposal.

## Current disposition

The owner approved a separate local-only RP prototype while the plugin lane stays
paused. The binding task ledger carries the narrow exception. Automated virtual
authenticator tests and local-scope security review passed on 2026-09-28; see
`reports/security-findings.md`. No real Windows Hello test, deployment, or
system-wide provider integration has been performed.
