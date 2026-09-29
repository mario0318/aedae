# T-020 - web/ merge conflict: handoff

Date: 2026-09-17
Status: unresolved, needs a decision before `main` can be pushed

## What happened

Local `main` and `origin/main` diverged at `d71c754` (`Expand experimental evaluation plan`) and
each grew a separate, incompatible line of `web/` work:

```
* f058d9e (local main)  Land Vault/KeyProtection implementation, T-005..T-019 reports, deploy scripts
* 3bb75b9 (local main)  Add aeDae public website foundation
| *   c2fb9e3 (origin/main)  Merge pull request #4 from mario0318/cx/t020-review-repair
| |\
| | * 0a69cbd  Enforce accessible surface accents
| | * bdf37f7  Resolve website truth and design review
| | * 3ff0646  Correct current implementation status copy
| | * b7fb7fa  Rebuild auditable static website scope
| |/
| * 254cdf6  Merge pull request #1 from mario0318/cx/t017-contract-governance
| * 2459862  Add human-only contract governance
|/
* d71c754  Expand experimental evaluation plan   <- merge-base
```

Local `3bb75b9` ("Add aeDae public website foundation") built `web/` from scratch on top of the old
base. Separately, `origin/main` picked up `2459862` (T-017 contract governance, merged as
[PR #1](https://github.com/mario0318/aedae/pull/1)) and then a full website rebuild on branch
`cx/t020-review-repair` (`b7fb7fa` through `0a69cbd`), merged as
[PR #4](https://github.com/mario0318/aedae/pull/4). Neither line knows about the other's `web/`.

`git push origin main` was rejected as non-fast-forward. A `git rebase origin/main` was attempted
and produced add/add conflicts on nine files (the entire local `3bb75b9` commit conflicts with the
origin rebuild):

- `web/README.md`
- `web/build.mjs`
- `web/spec/01-information-architecture.md`
- `web/spec/02-design-system.md`
- `web/spec/03-homepage-copy.md`
- `web/spec/04-claim-registry.md`
- `web/spec/05-repo-and-deployment-plan.md`
- `web/spec/06-file-manifest.md`
- `web/src/site.mjs`
- `web/src/styles.css`

**The rebase was aborted.** Nothing was force-pushed, nothing was resolved by guessing, no content
was discarded. Local `main` is currently sitting at `f058d9e`, one commit ahead of `3bb75b9`,
unpushed and unchanged from before the rebase attempt.

## A strong clue on which side should win

There is a linked git worktree at `C:/Projects/aeDae/.t020-review`, checked out on branch
`cx/t020-review-repair` at `0a69cbd` — clean, tracking `origin/cx/t020-review-repair`, nothing
uncommitted. That is *exactly* the branch that became `origin/main`'s website rebuild
([PR #4](https://github.com/mario0318/aedae/pull/4)). This strongly suggests the origin version was
built and reviewed deliberately (T-020 in `tasks.md` is listed as `IN_REVIEW`), and local's `3bb75b9`
is an earlier or parallel attempt that the review process already superseded before PR #4 merged.

That is a strong hint, not a confirmed answer. Whoever picks this up should confirm with the human
owner (mario0318) rather than assume it.

## What still needs a human/agent decision

1. Is `origin/main`'s `web/` (from PR #4) the version that should win outright, with local's
   `3bb75b9` `web/` content discarded?
2. Or does local's `3bb75b9` contain content or fixes that never made it into the PR #4 rebuild and
   need to be merged in by hand?
3. Once that's decided: rebase (or merge) local `main` onto `origin/main`, resolve the nine
   conflicting files accordingly, then push. Do not force-push over `origin/main` — it has real
   merged PR history that must be preserved.

## What is NOT part of this problem (already done, safe, unpushed)

The rest of local `main`'s ahead-of-origin content (commit `f058d9e`) is unrelated to the `web/`
conflict and was already vetted this session:

- Vault/KeyProtection implementation (`src/Vault/*`), management UI additions
  (`src/App.UI/IdentityHealthModel.h`, `ManagementApp.cpp`, `MockThisPcService.h`), and their tests
  (`tests/unit/*`).
- New build projects (`build/AeDaeManagementApp.vcxproj`, `KeyProtectionTests.vcxproj`,
  `ManagementModelTests.vcxproj`, `VaultStoreTests.vcxproj`) and the packages lock file.
- All `reports/T-005-*` through `reports/T-019-*` review/finding artifacts, plus `ADR-002`.
- `.github/CODEOWNERS`.
- `web/deploy/*` (Cloudflare Pages deploy scripts) and `web/test.mjs` — these are new, not part of
  the `web/` conflict above since origin has no `web/deploy/` yet. Worth double-checking they still
  make sense once the `web/` conflict resolves, since they may reference paths from local's `web/`
  layout rather than PR #4's.
- Also this session: deleted `external/sqlite-amalgamation-3530400/` (forbidden per `ADR-002`) and a
  stale `cloudflarepages.zip`, and added `.t020-review/` to `.gitignore` so the linked worktree can't
  get swept into a commit by accident.

None of that needs redoing. The only open problem is reconciling `web/`.

## Also still open, unrelated

`reports/microsoft-v1-envelope-clarification-request.md` / T-019: two unanswered emails to
`fido-dev@microsoft.com` (2026-09-04, 2026-09-11), and a new issue filed today,
[microsoft/Windows-classic-samples#431](https://github.com/microsoft/Windows-classic-samples/issues/431),
also awaiting a reply. Nothing to do here but wait and check back.
