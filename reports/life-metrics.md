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

Everything below runs in GitHub Actions (see `web/README.md`, "Running checks and deploys from any
session"), so it doesn't depend on the owner's PC being online.

1. **One-time secrets** in GitHub: `CLOUDFLARE_API_TOKEN`, `CLOUDFLARE_ACCOUNT_ID`,
   `TALLY_API_KEY`. Optionally, a required reviewer on the `life-preview` environment.
2. **Waitlist form:** run the workflow with `create_waitlist_form`. Put the printed
   `https://tally.so/r/...` link in `web/src/readme/config.json` → `waitlistUrl` and push. The build
   rejects anything that isn't a Tally share link. If empty, the page shows "sign-up list coming
   soon" and no link, so there is never a dead link.
3. **Checks:** every push runs Linux tests and browser checks, plus the Windows PowerShell 5.1 dry
   run. It was verified manually once on the owner's ThinkPad (2026-11-03), and that run caught the
   CRLF `_headers` bug.
4. **Preview deploy + verify:** run the workflow with `deploy_preview`. Life site only, `preview`
   branch only. It fails if production content changes, then runs `verify-preview.mjs` against
   `https://preview.aedae-life.pages.dev`. "Exactly one CSP" is a hard fail: Cloudflare joins
   duplicate headers with a comma, and browsers enforce the intersection.
5. **Confirm per-path counts:** in Cloudflare analytics, check that `/life/readme/done.svg` and
   `/life/readme/membership/` appear as separate paths on your plan. If not, use the GraphQL
   analytics API or Logpush and note which here. **No denominator means no gate.**
6. Then the 10-person watch, then the $150.

## Headline

The live line is "Joey has nine lives. Yours deserves a plan." Fallback, if the panel still reads
"nine lives" as a mortality reference: "Nine sections. One cat. Your people, covered."
