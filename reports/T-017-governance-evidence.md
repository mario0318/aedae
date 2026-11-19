# T-017 contract-governance evidence

Date: 2026-09-11  
Repository: `mario0318/aedae`  
Default branch: `main`

## Protected paths

`.github/CODEOWNERS` assigns `@mario0318` to the pinned ABI manifest and all three contract
enforcement scripts:

- `reports/webauthnplugin-abi-manifest.json`
- `scripts/ContractCheck.psm1`
- `scripts/verify-webauthnplugin-contract.ps1`
- `scripts/test-webauthnplugin-contract-guard.ps1`

It also owns `.github/CODEOWNERS` and `AGENTS.md` so an unreviewed change cannot remove the policy.
`AGENTS.md` prohibits every agent role from editing the four contract files and limits agents to
inspection, execution, findings, and proposals outside those paths.

## Live GitHub enforcement

Before T-017, `GET /repos/mario0318/aedae/branches/main/protection` returned HTTP 404 `Branch not
protected`. The owner authorized live branch protection. The applied rule requires pull requests,
dismisses stale approvals, requires code-owner review for owned paths, includes administrators,
requires conversation resolution, and disables force pushes and deletion.

The post-update API read returned:

- `enforce_admins: true`
- `dismiss_stale_reviews: true`
- `require_code_owner_reviews: true`
- `required_approving_review_count: 1`
- `require_last_push_approval: true`
- `required_conversation_resolution: true`
- `allow_force_pushes: false`
- `allow_deletions: false`

The isolated bootstrap is [pull request #1](https://github.com/mario0318/aedae/pull/1), commit
`24598623424a0c56d9ebcd3417357fba61a2901c`, against base
`d71c7542a04fd5038c78dce67f1432fb564a6383`. GitHub reports exactly two changed files:
`.github/CODEOWNERS` and `AGENTS.md`. The CODEOWNERS error endpoint returned `{"errors":[]}` for
the PR branch. No CI checks are configured on that branch; the project-local contract verification
and contract negative-guard scripts both passed before the commit.

The owner reviewed and approved PR #1. Because the sole GitHub identity could not self-approve, the
merge used a controlled temporary admin-enforcement bypass. Only admin enforcement was relaxed;
the PR and code-owner configuration remained present. PR #1 merged as
`254cdf6e8aafc2838e35a3324f17cc1924a9829b` at 2026-09-12 01:28:59 UTC. Admin enforcement was
restored in a cleanup block and a fresh API read confirmed the complete rule before testing.

## Identity constraint

The repository currently has one write-capable identity, `mario0318`, which is both repository owner
and code owner. The first post-merge probe revealed that code-owner review with zero blanket
approvals was insufficient: an owner-authored change to protected `AGENTS.md` reported `CLEAN`.
That result was rejected, and the live rule was strengthened to require one approving review and
approval by someone other than the latest pusher.

With the strengthened rule, disposable [PR #2](https://github.com/mario0318/aedae/pull/2) changed
only protected `AGENTS.md`, commit `a0ffb1ba9acc2e724af747009353483677e164fb`, and GitHub reported:

- `mergeable: MERGEABLE`
- `mergeStateStatus: BLOCKED`
- `reviewDecision: REVIEW_REQUIRED`
- no status checks

This is the required rejection evidence. The probe was closed without merge and its remote/local
branch and temporary worktree were deleted. No contract file was changed.

Until a second write-capable human reviewer is added, every legitimate protected-path PR will also
be blocked. The safe operating procedure is explicit human review and authorization followed by a
controlled temporary admin bypass, immediate restoration, and a fresh protection-state read. Adding
a distinct trusted human collaborator later would allow ordinary GitHub approval without that
bypass and is the preferred long-term arrangement.

T-017 is complete within this documented single-identity constraint.
