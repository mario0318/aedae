# Human-owner disposition — Tasks 1–4

Date: 2026-09-14
Status: Official owner response

The following decisions are the human owner's official disposition of the current Task 1–4
evidence and review findings.

## Decisions

### Task 1 — Documentation and bootstrap baseline

- T-001 is accepted as `DONE` once its acceptance criteria are recorded explicitly.
- T-002 is accepted conditionally on human architecture sign-off. It remains documentation-only
  and does not authorize protocol implementation.
- T-003 is accepted only for the narrow `IUnknown` bootstrap COM activation and lifetime scope.
  It must not be described as official plugin registration or `IPluginAuthenticator` support.
- Official plugin registration and `IPluginAuthenticator` implementation are separate future
  protocol-lane work and remain blocked by the unresolved signing and cancellation contract.

### Task 2 — Website

- Do not merge the current mixed working tree.
- Preserve unrelated changes; do not use broad `git restore` or `git checkout` operations.
- Produce an isolated candidate containing only authorized `web/` changes, rerun the website
  assertions and build, and review that narrow diff before approval.

### Task 3 — Packaging

- T-013 is accepted as `DONE`.
- Existing evidence for reproducibility, MakeAppx unpacking, manifest semantics, signature
  absence, and PE hardening is sufficient.
- The PE-hardening evidence is recorded in `reports/T-013-packaging-blocker.md`.
- No further packaging repair is authorized by this disposition.

### Task 4 — CI and SDK strategy

- Do not weaken the SDK/header hash check.
- Do not repin the manifest to match mutable `windows-2025` runner output.
- Use an immutable, exact reviewed SDK artifact or offline/self-hosted installation for CI.
- T-018 remains `IN_PROGRESS` until an independent CI run succeeds against that immutable SDK.

## Protocol-lane boundary

The operation-signing envelope and cancellation-authentication questions must not be resolved by
inference from Microsoft sample behavior. Protocol implementation, registration, request decoding,
Windows Hello invocation, credential operations, signing, and experimental API use remain paused
until the required authoritative contract and approvals exist.

## Scope of this disposition

This record authorizes the above review and documentation decisions only. It does not authorize
source-code changes, protected contract-script changes, credential access, signing, registration,
deployment, or destructive cleanup of unrelated working-tree changes.
