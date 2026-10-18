# T-019 owner decision brief: continue without a Microsoft answer

Date: 2026-09-28  
Status: proposal; no protocol implementation or risk acceptance authorized

## What is known

The current sample signs the raw encoded operation request with key-blob-driven
verification parameters. It does not demonstrate a contractual signing
envelope, transaction binding, or authenticated cancellation. The public
Microsoft sample issue remains open without a reply shown as of this date;
the private email thread was not checked in this pass. See
`reports/T-019-operation-signing-envelope-finding.md` for the G1/G2/G3
evidence boundary.

The browser-mediated `localhost` RP experiment is independent. It does not
validate the system-wide plugin, resolve the operation signature, or authorize
Windows broker registration.

## Owner choices

1. **Keep the production gate (recommended):** Continue T-014/T-018,
   packaging, and release preparation, but no plugin registration, request
   decoding, Hello call, credential operation, or signing until the platform
   contract and security review are sufficient. This preserves the stated
   product security invariants.
2. **Authorize a separately isolated lab investigation:** Amend `tasks.md`
   narrowly after security review to allow synthetic request/signature tests
   against a mock platform key, with no real credential material, no Windows
   registration, no Hello invocation, and no product claim. This could prove
   fail-closed implementation mechanics, not the contract or replay safety.
   The existing blanket protocol prohibition remains binding until explicitly
   amended. It must not silently turn into a production implementation.
3. **Accept an unconfirmed production contract:** Not recommended under the
   current evidence. The sample-derived bytes leave G1 replay and G2
   cancellation unresolved, and G3 allows upstream behavior drift without a
   header change. A general approval to work on the plugin is not specific
   acceptance of those risks. This would require an explicit, documented
   owner decision, revised security/architecture/task contracts, independent
   adversarial review, and a launch gate that accurately states the residual
   limitations. Do not treat an AI review or a green local build as proof of
   platform security.

## What would close the production question

- A Microsoft contract-level answer identifying the signed bytes, key and
  algorithm rules, cancellation authentication, and transaction/replay
  lifetime guarantees; **or** a materially different, security-reviewed
  design that proves equivalent guarantees without assuming them from sample
  code.
- T-015 must bind to that decision and include fail-closed negative tests
  before T-004/T-007/T-008/T-009 resume. T-016's reviewed lifecycle design
  must then be implemented and tested under the same gate.

This brief asks for a scope/risk decision. It is not itself that decision.
