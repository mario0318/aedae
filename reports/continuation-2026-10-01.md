# Continuation evidence and remaining work

Date: 2026-10-01
Scope: local verification and handoff reconciliation; no release or protocol authorization.

## Verified this pass

- Main checkout remains at `6c672e6`; pre-existing task edits, experiments, and reports are preserved.
- Main website: 45 focused assertions passed; build emitted 15 pages across five surfaces.
- T-024 signature-boundary lab: six tests passed. This remains synthetic model evidence only.
- T-023 loopback RP: all five test groups passed, including the CDP virtual-authenticator browser flow and rejection cases. No real Hello validation is claimed.
- Native Debug build wrapper exited 0: contract verification and negative guards, solution build, COM activation harness, VaultStore, ManagementModel and synthetic KeyProtection checks passed. The foreign-owner fixture retains its documented Win32 1307 permission limitation. Passing guards do not resolve the known unreadable-source scanner finding.
- Existing `C:\Projects\aeDae\.t020-review` checkout is clean at `280c9f0`. Its website passed 46 assertions when run from `web/`; preview preparation produced five bundles of 18 files each.
- The locally stored `origin/main` is `ef173ca`, the PR #6 preview-integration merge. Its `web/` tree is byte-identical in Git to `280c9f0`. This is local Git evidence, not a fresh remote or deployment check.
- The first preview test invocation from the checkout root failed because that version of the test expects `web/` as its working directory. The documented invocation from `web/` passed; no source change was necessary.
- Public issue https://github.com/microsoft/Windows-classic-samples/issues/431 was retrieved and shows open with no answer displayed. The web retrieval reports a two-day-old crawl; the private email thread was not checked. Neither source establishes a new platform contract.
- The protected source scanner still uses `-ErrorAction SilentlyContinue` at both scan sites. The existing human-owned remediation proposal remains applicable.

## Next work by owner

| Work | Owner | Required evidence |
| --- | --- | --- |
| T-014 scanner remediation | Human code owner | Apply `reports/T-014-human-owned-remediation-proposal-2026-09-28.md` through the protected-file PR path; locked-file refusal, exact hashes, independent review and owner approval. |
| T-018 reproducible SDK source | Architect and human code owner | Obtain an immutable source for the exact reviewed headers, or explicitly review a new contract pin; then pass hosted CI. Existing local build success cannot close this. |
| T-019 clarification | Microsoft, then Architect/Security | Contract-level signed-byte, replay/transaction-lifetime and cancellation answers; assess against G1/G2/G3 before resuming T-015. |
| T-021 temporary preview publication | Packager after explicit publishing authorization | Recheck current remote source and approval, publish the prepared five previews, then verify every live homepage/stylesheet and visual presentation. |
| Multi-scope foundation reconciliation | Coder/Packager with human owner | Keep website, vault, experiments and protected contract changes separately reviewable; do not merge the broad reconciliation branch as a website-only change. |

The 2026-09-27 plan's instruction to create a preview integration candidate has already been satisfied by the local PR #6 history. Repeating that preparation would duplicate existing work. T-020/T-021's older ledger notes are historical evidence, not proof of current live publication.

## Boundaries

No protected contract files, production source, credentials, registry, package registration, or remote content were changed. T-019/T-015 remain paused; no new task is marked DONE. The applicable AGENTS.md explicitly reserves the four ABI contract paths for a human-approved PR and forbids direct agent edits. Publishing remains a separate explicit authorization under the same command policy.
