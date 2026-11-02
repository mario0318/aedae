# Life README: five-reviewer panel (2026-11-02)

Five independent Claude reviewers, each with one role, none seeing the others: skeptical investor,
security reviewer (AGENTS.md role), product/brand designer, legal/compliance (not legal advice),
and Planner (AGENTS.md role). No GPT/Gemini/DeepSeek were available in this session.
Competitor facts below were web-checked by the investor reviewer; legal points were NOT
web-verified and must be re-checked before launch.

## Where they agree

1. **The real risk is people not finishing, not upkeep.** Nothing today can tell you whether
   anyone completes the builder. Measure completion before building phase 1 email infrastructure.
2. **"Joey has nine lives. You get one." breaks the brand's own rule** (no death jokes). Investor
   and designer both flagged it. Also cut "feed me by 6pm" (hospital scene), "I'll nap" (wishes
   section) and "Joey approves" (gamifies an estate file).
3. **Joey is an asset for consumers, a liability for B2B.** Keep him for people, keep employer and
   attorney material plain. Decide now how the brand handles Joey's own death someday.
4. **Never email strangers.** Phase 1/2 invites should be a link the owner sends themselves.
5. **No backend for phase 1.** Use a hosted newsletter with double opt-in, tracking off, and
   linked out from the site. Aedae holds zero emails.

## Where they disagree

- **Name.** Investor: "Life README" is a good name. Designer: it's developer jargon for normal
  people; suggests **"Just in Case"** (with Joey). Tie-break: test both in the ad experiment.

## Must-fix before going live (phase 0)

| # | Fix | From | Size |
|---|---|---|---|
| 1 | Drafts default to session-only; "remember on this device" is opt-in | Security (Medium) | S |
| 2 | Validators are bypassable; ban `location`/`.href=`/meta refresh, allowlist links, add bypass payloads as tests. A `location.href` redirect could leak drafts, and CSP won't stop it | Security (Medium) | S |
| 3 | Copy: drop the death jokes above, make Joey's "people" line honest (nothing is sent), "Joey can't catch everything" on the secret warning, add "not an advance directive" in Health | Design, Legal, Security | S |
| 4 | Verify on a preview deploy that `/life/readme/` gets exactly one CSP with `script-src 'self'` | Security, Planner | S |
| 5 | Human sign-off on the `_headers` change (required by `web/spec/05`) | Security | — |

## Should-fix (UX)

- Hero: one sentence of value + time ("about 20 minutes, stop anytime"), privacy as a badge, a
  Start button. Hide the nine tabs behind "Step 1 of 9".
- A "3 essentials" short path: First 24 hours, People, Digital → download.
- "Skip for now" on every section. "Saved on this device as you type" notice.
- Brighter Joey photo with his face clearly visible.
- New "tell my people" email, subject "Something I did for you, nothing to worry about".

## Business

- Pricing: drop $4/mo. Yearly ($29–39) or one-time (~$49). Will-partner referral fees are the most
  realistic early revenue (needs a clear "we earn a fee" disclosure next to the link).
- Gap (web-checked): Everplans was bought by Precoa and Cake by Foundation Partners, both in the
  funeral business. Trust & Will raised a Series C (2026) and sells via banks. Pitch: **"We don't
  sell funerals. You keep the file."**
- Next week: sit with 10 people who just lost or hospitalised a parent and watch them use it.
  Then ~$150 of ads to new parents/carers. Kill line: under 15% finish, or under 2% click a
  fake "Keep me reminded — $39/yr" button.

## Governance gaps (stop-and-report per AGENTS.md)

- `security.md` covers only the authenticator. A web/service security spec is needed before
  phase 1 stores anything.
- A dead-man switch releases on silence (fails open). That conflicts with `security.md`
  invariant 5 (fail closed). It has to be resolved explicitly before phase 3.
- Phase 3 trustee encryption is new crypto, so it needs human and strong-model review first.
- AGENTS.md says Planner edits `spec/tasks.md`, but the backlog is root `tasks.md`.

## Before phase 1 (legal, not verified)

1. A lawyer reviews the copy (a few hundred dollars).
2. Privacy policy and terms, plus a data-processing agreement with the email provider.
3. Double opt-in with logged consent, one-click unsubscribe, and a postal address in every email.
4. Invites sent by the owner, never by Aedae.
5. Breach plan (72h under GDPR). Health wishes are special-category data under GDPR.
   HIPAA likely doesn't apply unless a B2B health or benefits partner supplies data.

## Planner's path to live

L-001 local build check → L-003 security review → **L-004 preview deploy (your approval)** →
L-005 header/behaviour check → **L-006 bind aedae.life (your approval)**. About a day with you
available. Phase 1 (newsletter link-out, cadence calendar files, a `/life/readme/done/` page so
the host can count completions without analytics) adds about 3–5 days, mostly waiting on your
account and DNS steps.

## Sixth opinion (external AI, relayed by owner)

It agreed independently on: phase 0 as the trust wedge, deferring release behind security review,
"no secrets, only pointers", and no cold lists. Its new or sharper points:
- Referral revenue is carrying the business until phase 3. Are the affiliate economics real?
  (Nobody has checked actual payouts yet.)
- The password check can false-positive on addresses as well as miss passphrases. Never market
  it as a guarantee.
- The contact graph of third parties needs a lawful basis, retention and deletion spelled out
  (matches the legal reviewer).
