# Security findings

## T-023 — loopback WebAuthn relying-party experiment

Reviewed: 2026-09-28  
Disposition: APPROVED for the isolated local experiment only; not approved for deployment,
production identity, or the Windows plugin protocol lane.

### Scope and review evidence

- Owner approval covers a narrow browser-mediated WebAuthn experiment on `localhost`.
  The original plugin product and T-019 operation-signing envelope remain paused.
- The pinned dependencies are `@simplewebauthn/server` 14.0.3,
  `@simplewebauthn/browser` 14.0.0, and test-only `playwright-core` 1.63.0
  with an npm lockfile. No custom WebAuthn parser, signer, or private-key store
  was added.
- The peer scope review returned no findings; the implementation review returned
  no critical or high findings. Its four lower-severity observations were
  independently checked against source and resolved as below.
- `npm test`: 5 groups passed, 0 failed. CDP virtual-authenticator tests cover
  registration, authentication, UI button flow, cross-session registration,
  replay, tampered origin/challenge/RP data, wrong credential, and cancel.
  Server tests cover exact host/origin, session cookie, one-use challenges,
  content type, and body bound.
- `npm audit --omit=dev --audit-level=moderate`: zero reported advisories.
- `scripts/build.ps1 -Configuration Debug`: contract verification and guard
  negative tests, native solution build (0 warnings/errors), COM harness,
  VaultStore, ManagementModel, and KeyProtection tests passed. The existing
  foreign-owner fixture reported its known Win32 1307 permission limitation
  while the overall VaultStore suite passed.

### Findings disposition

1. **Medium — cookie omits `Secure`: accepted as documentation.** HTTP
   `localhost` is intentional for this nondeployable demo. The source and README
   now warn not to reuse the cookie policy for an HTTPS deployment. No release
   approval is inferred.
2. **Low — active-session cap denies new sessions: accepted as intentional.**
   The 128-session bound fails closed rather than evicting an active session;
   the behavior is documented.
3. **Info — generic catch conceals internal errors: rejected as a security
   change.** Responses remain generic; the demo does not log verifier exceptions
   that could include challenge or credential details. Debugging may require
   local instrumentation, not production-style request logging.
4. **Info — browser-path skip wording: strengthened.** The browser test now
   fails if Chrome/Edge CDP virtual WebAuthn is unavailable; it cannot silently
   pass through a skip in CI.

### Remaining boundaries

- No real Windows Hello prompt was invoked. A successful virtual authenticator
  is not evidence that Windows Hello works on this machine or in the native
  plugin path. The owner may run the manual demo separately if desired.
- Public credential records and session state are process-memory-only. This
  deliberately lacks persistence, account bootstrap/recovery, rate limiting,
  production cookie policy, and deployment architecture; it is not a service
  security approval.
- An RP controls only its own origin. It cannot serve as a selectable provider
  for third-party websites or close T-019/T-015.

## T-014 — current protected contract-checker candidate

Reviewed: 2026-09-28 against `origin/main` at `ef173ca`  
Disposition: APPROVE_WITH_REQUIRED_CHANGES for this exact candidate; do not
merge or mark T-014 done. This review does not authorize an agent to edit the
four human-only contract paths. The exact packet and full hashes are in
`reports/T-014-current-review-packet-2026-09-28.md`.

### Required finding

**Medium — source scan can fail open on file-read errors.**
`scripts/ContractCheck.psm1` calls `Select-String` with
`-ErrorAction SilentlyContinue` for both `EXPERIMENTAL_` and SQLite tokens.
If an enumerated file cannot be read, the scanner treats the suppressed error
as no match and can report a clean contract. A read-only probe against a
nonexistent literal path returned `False` under this setting despite
`$ErrorActionPreference='Stop'`. The build's successful current tests do not
exercise this condition.

Required human-authored protected-path remediation: make each scan failure
terminating (for example, `-ErrorAction Stop`), add a negative fixture with an
unreadable/locked selected source file, and prove both the verifier and the
build wrapper fail closed. Re-freeze all four protected SHA-256 values, rerun
the Debug/Release contract and build gates, and obtain a new independent
review plus human code-owner PR approval. Do not copy the older 2026-09-11
hashes; the three script hashes have changed even though the diff shape is
similar.

### Other review observations

- **Low — SQLite token coverage:** the four literal tokens identify the known
  amalgamation names but are not a general ban on custom `sqlite3_*.h`
  headers or other source naming. Decide whether ADR-002 intends a narrow
  known-distribution check or a broader dependency ban before approving the
  protected scanner; document the choice and add a fixture if broadened.
- **Info — artifact selection:** `build/` and `artifacts/` are scanned only
  for `.map`, `.vcxproj`, `.props`, and `.def`. Confirm that generated or copied
  compilable source cannot bypass the scanner; the `src/` tree is scanned
  recursively.
- **Info — negative-fixture coverage:** the guard tests prohibited tokens in
  `src/`, but not selected `build/` or `artifacts/` paths. Add such a fixture
  during the human-owned amendment if the artifact scan remains a security
  control.

The SDK-version, header-hash, stable IID, and method-order checks were not
weakened by this diff. This is a narrow contract-integrity review only; it
does not resolve T-019's signing envelope, replay, or cancellation questions.

## T-024 — synthetic operation-signature boundary lab

Reviewed: 2026-09-29. Disposition: APPROVED for the isolated lab scope only;
no production verifier, plugin registration, or T-015 design approval.

The code uses only Node built-in cryptography with an ephemeral process-local
keypair and synthetic bytes. No Windows/WebAuthn/Hello/plugin API, vault,
credential, network path, or protected ABI file is touched. Five original
tests established that the model rejects changed encoded bytes but accepts
changed transaction/context fields; a sixth test prevents fixture overrides
from silently replacing the signed bytes. `npm test` passed 6/6, and the
native Debug build including contract guards and local tests passed. The
foreign-owner ACL fixture retained its known Win32 1307 permission limitation.

Independent peer review of the constrained change packet found no scope
violation. Its low-severity override observation was accepted and fixed with
a rejecting guard and test. Its medium suggestion to say an attacker could
redirect/retype a signed blob was rejected as a production claim: this lab
does not prove attacker control of broker fields or broker replay reachability.
The README instead reports only what the synthetic signature covers. The
review's information-level observations agreed that G1/G2/G3 and the
production gate remain open.

Residuals: the sample-observed input is not a normative platform contract;
broker lifetime/freshness and cancellation authentication remain unknown.
This lab is evidence for deciding what additional guarantees are necessary,
not evidence that the system-wide provider is safe to register or launch.
