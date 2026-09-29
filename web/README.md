# aeDae web

Static, dependency-free public-site source for the aeDae constellation. It is intentionally separate
from the authenticator implementation while remaining in the same repository.

## Build and test

Run `node test.mjs`, then `node build.mjs`. The tests exercise claim-validation rejection paths and
the build validates structured claims, page structure, internal links, and prohibited runtime
features before writing 15 generated pages to `dist/`.

The optional `deploy/` directory prepares five local Cloudflare Pages preview bundles. Run
`powershell -NoProfile -ExecutionPolicy Bypass -File deploy/prepare.ps1` for a network-free check.
It does not authenticate, upload, bind a domain, change DNS, send email, load third-party assets,
or collect visitor data. Publishing temporary `*.pages.dev` previews is a separately authorized
operator action using `deploy/deploy.ps1` after local preparation and review.

## Claim statuses

- `verified`: current repository fact
- `specified`: documented design requirement, not implemented
- `research`: unanswered investigation
- `horizon`: future direction
- `practice`: general, vendor-neutral guidance; not an aeDae product claim

No claim may be marked `built`. Every claim needs a repository source or an explicit practice source.
