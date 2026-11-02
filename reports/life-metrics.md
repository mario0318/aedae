# Life README: the gate metrics

Nothing else moves until **completion rate** exists. The fake door must be live before the
~$150 ad test, so finishing and willingness to pay are measured on the same traffic.

## Gate (one number)

**Completion rate = browser-deduped (approximate) completion-image requests ÷ bot-filtered builder views. Gate: ≥ 15%.**

- Numerator: requests for `/life/readme/done.svg`. The image is shown only after **Download**.
  It has a fixed URL (no cache-busting, which the build bans anyway), so it is counted about once
  per **browser profile**. A cleared cache, a private window or a second browser on the same machine
  counts again. Call it **browser-deduped download events (approximate)**: not raw download clicks,
  and not a precise unique-user count.
- Denominator: requests for `/life/readme/` with bots removed. Exclude Cloudflare
  verified-bot traffic and obvious crawler or prefetch user agents. A rough filter is enough; never
  run the gate on raw hits.
- Known bias, accepted: ad blockers, privacy browsers and caches suppress the image, so the rate
  is undercounted. That makes the 15% gate conservative. Don't try to defeat blockers.

## Secondary reads (not gates)

| Read | Numerator | Denominator | Note |
|---|---|---|---|
| Fake-door click-through | **arrivals** at `/life/readme/membership/` | completion-image requests | The page is reachable only from the button after download, so arrivals ≈ clicks. The owner's own visits can't be filtered on Cloudflare's free plan: keep them few and note them. |
| Waitlist sign-ups | Tally form submissions | fake-door arrivals | A separate number, not a substitute for click-through. |

About 2% click-through on a tool audience would be good. **Don't kill the product on the
fake-door number** if completions clear 15%.

## What the visitor is told

The about page (colophon) says that after Download the page shows one image from this site, that
our host counts how often it and the membership page are fetched, and that nothing about the
person or their draft goes with it. If Tally is linked, the membership page names it. The image
sends no referrer.

## Order of operations

1. **Tally form**: one email field, "Membership isn't open yet — tell me if it opens". Put its
   share link in `web/src/readme/config.json` → `waitlistUrl`. The build rejects anything that
   isn't a `https://tally.so/r/...` link. If empty, the page shows "sign-up list coming soon" and no
   link, so there is never a dead link.
2. **Check the deploy script before running it** (PowerShell, on Windows):
   from the repo root, `.\web\deploy\deploy.ps1 -DryRun`. It also runs `prepare.ps1`, so a syntax
   error in either script shows up here. It must print `--project-name aedae-life --branch preview`
   with both paths absolute and in double quotes, then "DRY RUN complete". Afterwards `git status`
   must be clean. Use `[System.Management.Automation.Language.Parser]::ParseFile` only if it fails
   in a way you can't pin down (it lists every error with line numbers).
   Before the first real run: in the Pages dashboard, confirm `aedae-life`'s production branch is
   `main`, and save `curl -I` of the production URL.
3. **Preview deploy, life site only**: `deploy.ps1` now defaults to `-Surface life` and always
   deploys to a non-production branch (`preview`), so a bound custom domain is never touched. From a
   Linux session with Cloudflare secrets, the equivalent is:
   `npx wrangler@4 pages deploy web/preview/pages/life --project-name aedae-life --branch preview`
   (after building and preparing the bundle).
4. **Verify**: `node web/deploy/verify-preview.mjs https://preview.aedae-life.pages.dev`. It checks
   exactly one CSP and the required directives, scans the served files, confirms the Tally link
   returns 200, checks `done.svg` is served, then runs the 20 browser checks on the real origin.
   "Exactly one CSP" is a hard fail: Cloudflare joins duplicate headers with a comma, and browsers
   enforce the intersection of both policies, which silently breaks resources. Afterwards, `curl -I`
   the production URL again: it must be unchanged.
5. **Confirm per-path counts**: in Cloudflare analytics, check that `/life/readme/done.svg` and
   `/life/readme/membership/` appear as separate paths on your plan. If not, use the GraphQL
   analytics API or Logpush and note which here. **No denominator means no gate, so find out
   before spending.**
6. Then the 10-person watch, then the $150.

## Headline

The live line is "Joey has nine lives. Yours deserves a plan." Fallback, if the panel still reads
"nine lives" as a mortality reference: "Nine sections. One cat. Your people, covered."
