# Proposal: Life README — Aedae as a life-readme service

Status: PROPOSAL (Planner output, not an approved task). Does not touch the authenticator,
crypto, or contract-protected files.

## One line

Everyone ships software with a README. Nobody ships their life with one. Aedae gets people to
write a Life README — a short, portable file that tells the people they love what to do, where
things are, and who to call — then keeps it current and gets it to the right people at the
right time.

## Why this, why Aedae

- Most adults have no will, and even those who do have no map of the rest: accounts, pets,
  passwords-location, medical wishes, who to call first. That gap is the actual pain for families.
- Wills are legal documents with jurisdiction-specific signing rules. A readme is not. So the
  product owns the "everything else" layer and routes the legal will to a partner.
- Aedae's existing DNA (Windows Hello, vault, "never hold what you can't protect") is the moat for
  the hard part: **controlled release** of the readme to trusted people, not storage of it.

## What "reaches out" means (and what it must not mean)

Cold-messaging strangers about their death is creepy and runs straight into anti-spam and
privacy law. Outreach comes from three legitimate directions:

1. **Cadence** — the service pings the *owner* (opt-in) on a schedule and on life events:
   birthday, new kid, marriage, divorce, new home, new job. "Your readme is 14 months old. Two
   questions, 90 seconds."
2. **Network** — a readme names people (executor, emergency contact, pet guardian). Those people
   get one invite: "Mario named you in their Life README. Here's what that means." That is the
   growth loop, and it is consent-based on both ends.
3. **Channels** — employers (benefits enrolment), banks/insurers (beneficiary updates), estate
   attorneys, hospices. They hand out Life README; Aedae never buys lists.

## The file format (open on purpose)

A Life README is Markdown with YAML front matter, versioned `life-readme/0.1`. Human-readable,
printable, diffable, works with zero Aedae servers. Openness is the trust argument: you can leave
with your data in one click because the data *is* the file.

Sections: Basics · First 24 hours · People · Health & care · Will & legal · Money map
(where, never numbers) · Digital (where the password manager is, never passwords) · Home & pets ·
Wishes · Notes to people.

Hard rule: **no secrets in the readme.** It points to where secrets live (password manager
emergency kit, safe, attorney). The prototype warns on anything that looks like a password.

## Release protocol (the security-heavy part, phase 3)

- Owner misses N check-ins across email + SMS over a configurable window (default 30 days).
- Trustees are notified and asked to confirm; release needs M-of-N trustee confirmations plus a
  waiting period during which any owner sign-in (passkey) cancels it.
- Sections can be scoped per person (pet guardian sees "Home & pets", not "Money map").
- End-to-end encrypted to trustees' keys so Aedae can't read released content. Needs a full
  `security.md` review before any code — this is exactly the kind of crypto that rule exists for.

## Ship order

| Phase | When | What | Data Aedae holds |
|---|---|---|---|
| 0 | done | Static guided builder on `aedae.life`: interview → download `life-readme.md` + yearly review `.ics` + "tell my people" email drafts | none (one disclosed count image) |
| 1 | weeks 2–4 | Opt-in check-in emails (cadence + life events), magic link back into the builder | email + schedule only |
| 2 | month 2 | Trustee invites + acknowledgement; will-partner referral (e-will provider, affiliate revenue) | contact graph |
| 3 | after security review | Encrypted vault + M-of-N release + passkey owner auth | ciphertext only |

Phase 0 is built. It has no backend, no accounts and no stored user data. It makes one self-hosted
count request: a fixed same-origin image shown after Download, so the host can count finishers. That
request carries no user data and no referrer, and it is disclosed on the about page. It can ship
the same day it's approved.

## Money

Revised after the referral red-team (see `reports/life-readme-panel-review.md`).

- Free: builder + file + calendar reminders. The free file is the marketing.
- **Primary bet: paid membership, $39/yr** (or ~$49 once). At 1,000 finished readmes, 2–3% paying
  is $780–1,170/yr, more than the best referral case, with no partner approval needed. It only
  works once it offers something real (managed check-ins first, trustee release later). Test it
  with a fake-door button before building.
- **Referral fees are upside, not funding.** Modeled at 1,000 finished readmes and ~$40 average
  commission: $80 (pessimistic) / $320 (base) / $720 (optimistic) per year. Approval needs a site
  that looks like a publication (10–20 real posts), cookies expire long before most people buy a
  will, and a funnel feel undercuts "we don't sell funerals". FreeWill has no affiliate program.
- Referral links appear only where a user says they have no will, with a plain note next to
  the link naming the partner and saying we earn a fee that varies by product (never a fixed promise). They never appear in reminders or in Joey's lines.
- B2B per-seat: parked. A solo operator can't run an enterprise sale.
- Paid fake-door test: the "Keep me reminded — $39/yr" button is a fixed link to a hosted waitlist
  form (named in the privacy line), or a static mailto if we want zero third parties. Navigation is
  not a network call from the builder, so it stays `connect-src 'none'`. Needs one allowlisted URL
  in the builder page validator.
- Fund the ~$150 ad test from a separate budget. Judge it on completion rate (≥15%) and paid
  fake-door clicks (≥2%), not referral dollars.

## Known players (check before pitching)

Everplans, Trust & Will, FreeWill, GoodTrust, Cake, Apple Legacy Contact, Google Inactive Account
Manager. They're either legal-document mills or single-platform death switches. Gap: an
**open, portable, life-wide readme with proactive upkeep**. If that gap is already filled when you
look, the release protocol is the fallback differentiator.

## Risks

- Legal wording: never call the readme a will; state laws differ. Copy reviewed before launch.
- Emotional UX: death is a conversion-killer. Frame as "if you're in hospital tomorrow," not
  "when you die." The prototype leads with the first 24 hours.
- Scope creep into the authenticator repo: keep the Life README as a separate deployable under `web/`
  or its own repo; it must not ship inside the signed authenticator package.

## Where it lives

`aedae.life/life/readme/` — source in `web/src/readme/`. It is the only route on the site allowed
to run script: one same-origin file, no inline code, `connect-src 'none'`, drafts in browser
storage only. `web/build.mjs` and `web/test.mjs` reject inline or external scripts and any
network-capable API in the builder. The file people download is `life-readme.md`
(format `life-readme/0.1`).

## Brand: Joey the cat

Joey (a real cat) is the guide. The builder already has exactly nine sections, so the hook writes
itself: "Joey has nine lives. You get one." A cat guide makes an end-of-life topic feel safe to
start, which is the main conversion problem.

Rules: Joey keeps it light but never jokes about death itself; for grief, illness or medical wishes
he goes quiet and plain. Joey also delivers the "that looks like a password" warning, because
people listen to a scolding cat more than a red box.

Domain: no new domain. The service runs on `aedae.life`, which is already configured for deploy.
`readme.cat` was dropped (`.cat` is restricted to the Catalan community) and `readme.life` is not
owned, so the product and file format are called Life README to avoid pointing people at a
domain someone else controls. Joey's avatar is a cropped, metadata-stripped photo
(`web/src/readme/joey.jpg`).
