# aeDae web

Static, dependency-free public-site source for the aeDae constellation. It is intentionally separate
from the authenticator implementation while remaining in the same repository.

## Build and test

Run `node test.mjs`, then `node build.mjs`. If Playwright is available, `node test-browser.mjs` runs browser acceptance checks against `dist/` (it skips when Playwright is missing). The tests exercise claim-validation rejection paths and
the build validates structured claims, page structure, internal links, and prohibited runtime
features before writing 15 generated pages to `dist/`, plus the Life README builder at `/life/readme/` (the only route allowed to run script; see `src/readme/`).

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

## Running checks and deploys from any session

Nothing here needs the owner's PC. GitHub Actions (`.github/workflows/life-readme.yml`) runs:

- **Every push touching `web/`:** Linux tests, build and the Playwright browser checks, plus a
  Windows job on Windows PowerShell 5.1 (CRLF checkout) that runs the tests and
  `deploy.ps1 -DryRun`, and asserts the exact deploy line, LF `_headers` and a clean tree.
- **Manual run, `deploy_preview`:** after both checks pass, deploys the life site with `deploy.ps1`
  to the Pages `preview` branch only, confirms production content is unchanged, then runs
  `verify-preview.mjs` (headers, files, browser checks) against `https://preview.aedae-life.pages.dev`.
- **Manual run, `create_waitlist_form`:** creates (or finds) the Tally waitlist form and prints its
  link in the run summary. Put that link in `src/readme/config.json` → `waitlistUrl`.

From a cloud session: push to trigger the checks. Start a manual run with the GitHub MCP
`actions_run_trigger` tool (workflow `life-readme.yml`, inputs as above), then read results with
`actions_list` / `get_job_logs`.

One-time owner setup: repo Settings → Secrets and variables → Actions → add
`CLOUDFLARE_API_TOKEN` (permission "Cloudflare Pages: Edit"), `CLOUDFLARE_ACCOUNT_ID`, and
`TALLY_API_KEY`. Optional but recommended: Settings → Environments → `life-preview` → add yourself as
required reviewer, so every deploy waits for a human click.
