# Staged launch integration plan

Date: 2026-09-27  
Status: planning and local validation only; no merge, push, deployment, signing, or protocol work performed

## Purpose

Move the publicly reviewable aeDae work forward without representing the bootstrap as a functioning
WebAuthn authenticator. The release unit in scope is an unsigned, static **public-site preview**.
It is not a passkey-provider launch, application installation, package release, or custom-domain
launch.

## Frozen starting point

| Item | Revision or state | Evidence |
| --- | --- | --- |
| Reviewed website candidate | `origin/main` at `c2fb9e3` (PR #4 merge) | The linked `cx/t020-review-repair` worktree is clean. On 2026-09-27, `node test.mjs` passed 36 focused assertions and `node build.mjs` built and validated 15 pages across five surfaces. |
| Local implementation/evidence bundle | `main` at `f058d9e`, plus handoff `6c672e6` | The branch is three commits ahead and seven commits behind `origin/main`; it must not be pushed as-is. |
| Website divergence | merge-base `d71c754` | Both lines add incompatible `web/` trees. The remote tree is the reviewed, merged PR #4 result. |
| Preview deployment tooling | local `web/deploy/` only | It is absent from `origin/main` and therefore needs a fresh, narrow compatibility review against the PR #4 website before it can be used. |
| Protocol lane | T-019 remains unanswered | Request decoding, registration, Hello invocation, credential work, and signing remain prohibited. |
| Contract CI | T-018 remains red by design | The hosted `windows-2025` header hash differs from the reviewed hash. Do not repin or weaken the protected contract check. |

## Recommended integration sequence

1. Treat the PR #4 website (`origin/main`) as the canonical public-site source. Do not merge the
   earlier local website implementation into it and do not force-push either history.
2. Create a new, narrowly scoped integration candidate **from `origin/main`** containing only the
   existing `web/deploy/` files required for temporary Pages previews. Exclude the proposed
   `web/spec/07-*` redesign brief, local alternate `site.mjs`, `styles.css`, `build.mjs`, and
   `test.mjs` unless each is separately reviewed against PR #4.
3. On that candidate, run the reviewed website tests/build and a network-free `prepare.ps1` run.
   Verify each of the five bundles contains its homepage, stylesheet, all five surface routes, and
   the restrictive `_headers` file. Review that small diff independently.
4. Only after the narrow candidate is approved and merged, obtain a separate authorization to
   authenticate to Cloudflare and publish the five temporary `*.pages.dev` previews. Check all
   five URLs and their stylesheets before considering any custom domain. No DNS or domain binding
   is included here.
5. Keep the bootstrap/foundation work in a separate candidate. `f058d9e` combines reviewed source,
   reports, packaging, CI work, and protected contract-script changes, so it cannot be safely
   cherry-picked as one launch change. Any transfer must be split by approval boundary; the four
   protected contract files require their own human code-owner PR.

## Honest launch statement

Permitted once steps 1–4 are complete:

> aeDae has a static public-site preview and an unsigned developer-bootstrap artifact. The Windows
> authenticator itself is not available: it does not register with Windows, invoke Hello, create or
> use credentials, or process WebAuthn requests.

Not permitted: describe the preview as a working passkey provider, a product release, a production
deployment, or a security-reviewed authenticator.

## Remaining hard gates

- T-019: Microsoft must document the operation-signing envelope and cancellation authentication,
  or the owner must make an explicit, reviewed alternative decision. Sample behavior is not a
  substitute.
- T-018: an independent CI run must use the exact immutable reviewed SDK/header artifact. The
  current mutable hosted runner is intentionally rejected.
- T-014 and the protected contract diff: human code-owner approval is required.
- Any signing, installation, publishing, custom-domain binding, DNS, or production identity work
  requires separate human authorization.

## Commands recorded

```powershell
# In C:\Projects\aeDae\.t020-review\web (PR #4 candidate)
node test.mjs     # 36 focused assertions passed
node build.mjs    # Built and validated 15 pages across 5 surfaces
```

No network command was run while preparing this plan.
