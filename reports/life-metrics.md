# Life README: the gate metrics

Nothing else moves until **completion rate** exists. Run the fake door before the ~$150 ad test,
so finishing and willingness to pay are measured on the same traffic.

All counts come from the host's own request counts (Cloudflare). There is no analytics script,
no cookie, and nothing the person typed is sent. The colophon discloses this.

| Metric | Numerator | Denominator | Gate |
|---|---|---|---|
| Completion rate | requests for `/life/readme/done.svg` (shown only after **Download**) | requests for `/life/readme/` | **≥ 15%** |
| Fake-door click rate | requests for `/life/readme/membership/` | requests for `/life/readme/done.svg` | **≥ 2%** |
| Waitlist sign-ups (bonus) | Tally form submissions | — | — |

"Finished" means the file was downloaded, not that section 9 was reached. The browser caches
`done.svg`, so repeat downloads from one browser count roughly once. That's acceptable, since it
approximates unique finishers.

## Before the ad spend

1. Create the Tally form ("Membership isn't open yet — tell me if it opens": email only). Put its
   share link (`https://tally.so/r/...`) in `web/src/readme/config.json` → `waitlistUrl`. The build
   rejects anything that isn't a Tally share link.
2. Deploy the preview and run `node web/deploy/verify-preview.mjs https://<preview-host>`.
   It checks for exactly one CSP and the required directives, scans the served files for inline
   attributes and `location`/`action=`, then runs the 19 browser checks against the real origin.
3. Confirm Cloudflare shows per-path request counts on this plan. If not, use the analytics API
   or Logpush, and record which one here.

## Headline

The live line is "Joey has nine lives. Yours deserves a plan." Fallback, if the panel still reads
"nine lives" as a mortality reference: "Nine sections. One cat. Your people, covered."
