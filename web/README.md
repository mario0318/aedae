# aeDae web

Static, dependency-free public-site source for the aeDae constellation. It is intentionally separate from the authenticator implementation while remaining in the same repository.

## Build

Run `node test.mjs`, then `node build.mjs`. The focused suite proves the negative claim, static-resource, accessibility-token, and contrast gates; the build validates all rendered routes and writes generated pages to `dist/`. Nothing in this directory deploys, changes DNS, sends email, or collects visitor data.

The optional `deploy/` folder contains five Cloudflare Pages configurations and a credential-free deployment script. Run `web/deploy/prepare.ps1` for a local, network-free check of the five self-contained preview bundles. After authenticating Wrangler on your own machine, run `web/deploy/deploy.ps1` from the repository root to publish temporary `*.pages.dev` URLs. The script does not bind custom domains or change DNS.

## Claim statuses

- `verified`: current repository fact
- `specified`: documented design requirement, not implemented
- `research`: unanswered investigation
- `horizon`: future direction
- `practice`: general, vendor-neutral guidance; not an aeDae product claim

No claim may be marked `built`. Every claim needs a repository source or an explicit practice source.
