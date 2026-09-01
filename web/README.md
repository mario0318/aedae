# aeDae web

Static, dependency-free public-site source for the aeDae constellation. It is intentionally separate from the authenticator implementation while remaining in the same repository.

## Build

Run `node build.mjs`. This validates public claims and writes generated pages to `dist/`. Nothing in this directory deploys, changes DNS, sends email, or collects visitor data.

## Claim statuses

- `verified`: current repository fact
- `specified`: documented design requirement, not implemented
- `research`: unanswered investigation
- `horizon`: future direction
- `practice`: general, vendor-neutral guidance; not an aeDae product claim

No claim may be marked `built`. Every claim needs a repository source or an explicit practice source.
