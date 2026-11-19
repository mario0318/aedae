# T-016 Lifecycle and cancellation design security review

Reviewed: 2026-09-02
Verdict: BLOCKED
Scope: `reports/operation-lifecycle-design.md`, `reports/operation-signature-gate-design.md` (cancellation clauses only), `scripts/ContractCheck.psm1`, `tests/integration/OperationLifecycleTestPlan.md`
Closed by: T-022 (design revision, F1–F7 and F9–F12) and T-018 MED-05 (F8)

This review is independent of the T-019 envelope question. Every finding below concerns the state machine, locking, cancellation binding, and lifetime rules, none of which depend on the signed byte sequence. The design revision can be completed in the local lane while the protocol lane is paused.

The state machine, atomic terminal transition, fail-closed gate, and v2 ban are the right shape. What is missing is the mechanism under each claim. Every finding is closed by a document or script edit; none requires new architecture. Implementers must address each finding by ID, apply each `Fix:` line literally, add nothing beyond it, and keep the design document under 200 lines.

## Findings

### F1 - CRITICAL - state and lifetime allocated before signature verification
`operation-lifecycle-design.md` §States places `Received` before `SignatureVerified`, and §Ownership requires the DLL to be retained while any coordinator is nonterminal. An unsigned request therefore creates a coordinator, mutates state, and pins `DllCanUnloadNow` to `S_FALSE`. Conflicts with `operation-signature-gate-design.md` §Rule ("untrusted until verified").
Fix: no coordinator, state entry, server-lock reference, handle, or heap object outliving the call frame before verification succeeds. Verification runs on a caller-frame copy with a stated byte bound and a stated cap on concurrent unverified requests. Make `SignatureVerified` the first coordinator state; `Received` becomes a non-owning pre-coordinator stage or is deleted.
Test: OL-001/OL-002 assert live-object count, server-lock count, registry size, and peak allocation unchanged after N unsigned requests; `DllCanUnloadNow` returns `S_OK`.

### F2 - CRITICAL - terminal transition and response publication unordered
`Completed` is defined as "response published exactly once" but nothing says the terminal transition must be won before calling the publisher. Cancel can win after the response has left; two threads can publish.
Fix: claim-then-publish. A thread atomically claims the terminal state first; only the winner calls the publisher exactly once; losers return the observed result without calling it. Define publish-failure-after-claim as terminal `Failed`, no retry, no re-entry. Zeroize the constructed response on every non-`Completed` outcome.
Test: OL-004 with a counting, fault-injecting publisher under forced interleavings across the claim/publish window: exactly one invocation, one terminal result, buffer zeroized on cancel-wins.

### F3 - CRITICAL - cancellation bound only to transaction ID
§Cancellation requires only signature plus transaction-ID match. No epoch/generation, no caller-context binding, no binding to the originating request, no replay resistance, no coordinator registry, no ID-collision policy.
Fix: cancel must match signature, exact transaction ID, a per-coordinator monotonically increasing epoch that is part of the signed input and never reused across coordinators or process lifetimes, and the caller context that started the operation. Define the coordinator registry, its lock, and uniqueness enforcement on transaction ID plus epoch. A mismatched cancel is a silent no-op: no state change, no timing difference, no log containing supplied identifiers.
Test: OL-005 replay matrix: captured valid cancel replayed against a recreated coordinator with the same ID, a different concurrent coordinator, a colliding ID, and a different caller - all no-ops, indistinguishable in result and timing.

### F4 - HIGH - no locking or threading model
No lock, lock order, apartment model, reentrancy rule, or statement about what may be held across UI, Hello, vault, or publisher calls. "Atomic terminal transition" names no primitive.
Fix: add a `Synchronization` section declaring the COM threading/apartment model; exactly one coordinator lock and one registry lock with a stated acquisition order; the fields each guards and which are atomics; a hard prohibition on holding either lock across any UI, Hello, vault, crypto, or publisher call; STA reentrancy safety for `CancelOperation`; a short no-deadlock argument.
Test: OL-013 (new): cancel from a second thread while fake Hello is outstanding returns within a stated bound; interleaving stress asserts no deadlock and no lock held across an instrumented external boundary.

### F5 - HIGH - cancellation not re-checked at side-effect boundaries; in-flight work may be "ignored"
One cancel check exists (gate design step 4). "Cancels or ignores later Hello completion" permits Hello UI and in-flight vault mutation to complete after `Canceled`.
Fix: one cancellation token created with the coordinator and passed into every UI, Hello, vault, and signing boundary; mandatory re-check immediately before UI display, Hello invocation, vault read, key unprotect, signing, metadata mutation, and response publication. Replace "or ignores" with active cancellation of Hello and UI. Metadata mutation happens only after the terminal `Completed` claim.
Test: OL-003 split into one case per boundary; each asserts the boundary is not entered or is aborted, no metadata change, no response.

### F6 - HIGH - cancel before registration is silently lost
Cancel must "match the active transaction"; a verified cancel arriving before the operation registers has nothing to match and is dropped, then the operation proceeds to Hello and signing.
Fix: authenticated pre-cancellation tombstone recorded in the registry under the registry lock with a stated retention bound; coordinator creation consults and consumes it atomically and enters `Canceled`. State the eviction policy.
Test: OL-010 (new): cancel delivered before its operation; operation is born terminal `Canceled`; no decode, UI, Hello, vault, or response.

### F7 - HIGH - callback lifetime is a prohibition without a mechanism
"Must not free buffers reachable by a Windows callback" names no owner, count, or join method. An "ignored" late Hello callback after cancel-triggered teardown is a use-after-free.
Fix: every outstanding external callback holds a strong reference to the coordinator; an in-flight-callback count is incremented under the coordinator lock before the external call is issued and decremented on completion; destruction is gated on the count reaching zero; the terminal transition is safe to observe from a callback.
Test: OL-007 extended: cancel and release with a pending fake Hello callback, then fire it; no invalid access under ASAN or Application Verifier; no state change.

### F8 - MEDIUM - v2 ban is enforced on the headers, not on our sources
`scripts/ContractCheck.psm1` pins all three header hashes, checks the IID and method order, and rejects unprefixed v2 declarations in the SDK header. Nothing scans `src/` or link output for a reference to an `EXPERIMENTAL_` symbol, so ADR-001's "compile-time prohibited" has no build-time check. OL-008 is otherwise satisfied.
Fix: in `Invoke-AeDaeHeaderCheck` or a sibling function, scan every file under `src/` and any `.map` or `.obj` listing under `build/` and `artifacts/` for the substring `EXPERIMENTAL_` and throw if found. Add a negative case to `test-webauthnplugin-contract-guard.ps1` using a temporary fixture file.
Test: OL-008 gains a third assertion: a `src/` file containing `EXPERIMENTAL_` fails the build.

### F9 - MEDIUM - `GetLockStatus` leaks live operation state
§Lock state makes the result depend on whether a nonterminal operation exists and whether the vault is ready, and the call is unauthenticated. This is the timing oracle needed to aim the races in F3, F5, F6.
Fix: result is a function only of coarse provider state (bootstrap/locked, vault-ready); it must not reflect the existence, phase, or resource needs of any operation; latency must not vary with operation state.
Test: OL-006 extended: value and timing identical across all nonterminal states of an active operation.

### F10 - MEDIUM - Hello verification result freshness not stated
`AwaitingUserVerification -> Executing` says nothing about single-use, expiry, or reuse on retry. `security.md` §2.2 requires fresh verification per assertion.
Fix: a verification result is bound to one transaction ID and epoch, consumed exactly once, time-bounded, discarded on any error or state re-entry, never cached or shared across coordinators. No transition back into `Executing` on a consumed verification.
Test: OL-012 (new): failure after successful fake Hello then retry yields a new Hello invocation or fail-closed terminal, never a signature on the consumed verification.

### F11 - MEDIUM - no expiry on nonterminal states
A coordinator stuck in `AwaitingUserVerification` pins the DLL for the process lifetime; with F1 this is reachable unauthenticated.
Fix: stated maximum lifetime per nonterminal state; expiry is fail-closed terminal `Failed`, releases all references, publishes nothing; stated cap on concurrent coordinators.
Test: OL-011 (new): hold each nonterminal state past its bound; assert `Failed`, no response, `DllCanUnloadNow` returns `S_OK` after release.

### F12 - MEDIUM - conditional zeroization and no logging rules
"Clears transient buffers where applicable" is undefined on the one path holding a built assertion. The lifecycle design has no logging rules; the gate design restricts only pre-verification logs.
Fix: unconditional zeroization of decoded payloads, key material, and constructed responses on every terminal path. Add a `Logging and zeroization` section: generic event IDs on all pre-verification and failure paths; no attacker-supplied bytes, no transaction identifiers, no detail distinguishing failure causes.
Test: buffer scan on cancel-after-`ResponseReady`; log-content assertion for OL-001/002/005 that records contain no request-derived bytes or identifiers and are identical across failure causes.

## Re-review 2026-09-02 (after T-022)

Verdict: **APPROVED_WITH_REQUIRED_CHANGES**, changes applied same day.

Disposition: F1 CLOSED (Ownership; `SignatureVerified` is the first coordinator state).
F2 CLOSED after R2. F3 CLOSED after R1, with a stated residual risk. F4 CLOSED (Synchronization).
F5 CLOSED after R4. F6 CLOSED (tombstones). F7 CLOSED after R3. F8 CLOSED in
`scripts/ContractCheck.psm1` — demonstrated failing on a probe file under `src/` and passing on the
clean tree. F9 CLOSED (Lock state). F10 CLOSED (User verification freshness). F11 CLOSED (Timeouts
and limits). F12 CLOSED (Logging and zeroization).

### R1 - HIGH - the epoch requirement was unsatisfiable
The first revision required a cancellation to carry a matching epoch and required the signing
envelope to cover it. The epoch is allocated by this plugin; the platform issues the transaction ID
and cannot know or sign a value the plugin invents. The rule could never be implemented, and an
implementer would have satisfied it by dropping it — leaving cancellation bound to the transaction
ID alone, which is F3 unfixed.
Resolution: the epoch is internal only and is never read from a request. Replay across operations
is defeated locally by a retired transaction-ID set: within a process lifetime a transaction ID is
answerable exactly once. The envelope requirement is reduced to operation type plus transaction ID,
which a platform can actually sign. Replay across a process restart is now recorded as an explicit
residual risk against T-019 rather than papered over.

### R2 - MEDIUM - `Publishing` was reachable by the blanket cancellation rule
`Any nonterminal state -> Canceled | Failed` was written above the note that `Publishing` may only
proceed to `Completed` or `Failed`. Read literally, the table permitted `Publishing -> Canceled`,
reintroducing the exact F2 race the claim was added to prevent.
Resolution: the blanket rule now excludes `Publishing` explicitly and the prohibition is stated.

### R3 - MEDIUM - no abandonment policy for a callback that never returns
Destruction waits on the in-flight-callback count. A callback the platform never completes leaves
the count nonzero forever, so `DllCanUnloadNow` never returns `S_OK` — reintroducing the indefinite
unload pin that F11 exists to prevent, via a path F11's state deadlines do not reach.
Resolution: `CallbackAbandonDeadline` is added. On expiry the coordinator is deliberately leaked and
a distinct diagnostic is emitted; the module stays unloadable. Retaining an unloadable module is
the fail-closed choice and a use-after-free is not, so this liveness cost is accepted and named
rather than traded away.

### R4 - MEDIUM - the window between the boundary check and the external call was unspecified
The per-boundary cancellation re-check said nothing about the gap between releasing the lock and
issuing the call, so a Hello prompt could still appear after cancellation with no rule covering it.
Resolution: the check and the in-flight-callback increment are one locked step; the residual window
is named as inherent, bounded by re-reading the flag once the platform accepts the call and
invoking the platform's cancellation facility; and no response, metadata mutation, or signature may
follow from a call whose result arrives after the flag was set.

## Exit criteria

T-016 may leave PAUSED for the design portion when T-022 and T-018 MED-05 are DONE and a re-review confirms each F-ID is addressed by a named section or script line, with the test plan carrying OL-010 through OL-013. Implementation of the lifecycle remains gated on T-015 and T-019 as before.
