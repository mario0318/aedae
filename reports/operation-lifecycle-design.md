# Transaction-safe operation lifecycle

Status: design-only, not implemented. Revised 2026-09-02 under T-022 to close
`reports/lifecycle-security-review.md` F1–F7 and F9–F12, then revised again the same day against
the re-review findings R1–R4 recorded in that file. F8 is closed by T-018 MED-05 in
`scripts/ContractCheck.psm1`, not here.

One residual risk is open by design and is not closed by this document: replay of a captured
cancellation across a process restart. See `Authenticated cancellation → Binding`.

This document defines ownership, synchronization, cancellation authentication, and lifetime. It
does not define the operation-signing envelope; that remains blocked on T-019 and T-015. Where a
rule below requires a value to be covered by the platform signature, it states a constraint that
the resolved envelope must satisfy — it does not assert or invent envelope bytes.

## Ownership

One `OperationCoordinator` owns one platform transaction. It is the only component allowed to
transition state, publish a response, or release the operation's lifetime reference.

A coordinator is created only after the operation-signature gate in
`reports/operation-signature-gate-design.md` has succeeded. Until that point the request has no
coordinator, no state-machine entry, no server-lock reference, no registry entry, no handle, and
no heap allocation that outlives the calling frame. Verification operates on a caller-frame copy
of the raw request bounded at `MaxRequestBytes`; a request exceeding it is rejected without
allocation. At most `MaxUnverifiedConcurrent` verifications may be in flight process-wide;
requests beyond that are rejected before the copy. Both bounds are configuration constants fixed
at build time, not runtime-tunable.

COM object lifetime and server-lock accounting retain the DLL while a coordinator is nonterminal.
Because no coordinator exists before verification, an unauthenticated caller cannot influence the
unload count. (F1)

## States

```text
(no coordinator)            request verified on the caller frame only
  -> SignatureVerified      first coordinator state; registry entry created
  -> Decoded                payload decoded and structurally valid
  -> AwaitingUserAction     optional non-secret UI decision
  -> AwaitingUserVerification  fresh Windows Hello is outstanding
  -> Executing              future authorized operation only
  -> ResponseReady          response built, terminal outcome not yet claimed
  -> Publishing             terminal outcome claimed by exactly one thread
  -> Completed              publisher returned success

Any nonterminal state EXCEPT Publishing -> Canceled | Failed
```

`Canceled`, `Failed`, and `Completed` are terminal. `Publishing` is a claimed state: it is
reachable only by winning the atomic terminal claim, its only successors are `Completed` and
`Failed`, and it is explicitly excluded from the blanket cancellation transition above. A
cancellation observed while the state is `Publishing` is a no-op. Reading the blanket rule as
permitting `Publishing -> Canceled` would reintroduce the F2 race and is prohibited.

### Terminal claim and publication (F2)

Publication is ordered by a claim, not by arrival:

1. A thread that intends to publish first executes the atomic claim, moving `ResponseReady` to
   `Publishing`. The claim is a single compare-and-set on the state word.
2. Only the winner calls the response publisher, exactly once.
3. Every loser — including a concurrent cancellation — observes the claimed state, calls no
   publisher, and returns the observed terminal result.
4. If the publisher fails after a successful claim, the operation becomes terminal `Failed`. There
   is no retry, no re-entry to a nonterminal state, and no second publisher call.
5. A cancellation that arrives after the claim does not change the result or the state.

No response may be published from any state other than `Publishing`. A terminal outcome other than
`Completed` zeroizes the constructed response before releasing it.

## Synchronization (F4)

The plugin COM class is registered as a multithreaded (MTA / `Both`) object. Platform calls,
including `CancelOperation`, may arrive concurrently on any thread. The design must remain correct
under single-threaded-apartment reentrancy, so no code path may assume a call cannot re-enter while
an outer call is in progress.

Two locks exist:

| Lock | Guards |
| --- | --- |
| `RegistryLock` | the coordinator registry: transaction-ID/epoch index, tombstone table, epoch counter, concurrent-coordinator count |
| `CoordinatorLock` | one coordinator's mutable fields: current state, cancellation flag, in-flight-callback count, verification-consumed flag, buffers |

Rules:

- Acquisition order is `RegistryLock` then `CoordinatorLock`. Never the reverse. No other lock may
  be introduced between them.
- Neither lock may be held across any call to UI, Windows Hello, the vault, cryptographic
  operations, or the response publisher. Every such call is issued with all locks released.
- The state word is an atomic; the terminal claim and the cancellation flag are read and written
  without holding `CoordinatorLock`, so that `CancelOperation` never blocks on a thread that is
  waiting on Hello.
- `CancelOperation` acquires `RegistryLock` only to resolve the target coordinator, then releases
  it before signalling. Its total lock hold time is bounded and independent of any external call.
- Because no lock spans an external call, and the two locks are always taken in one order, no cycle
  exists. Windows Hello cannot appear hung on account of a cancellation, and a cancellation cannot
  block behind Hello.

## Authenticated cancellation (F3, F5, F6)

### Binding

A cancellation request is untrusted until it passes the operation-signature gate. After the gate,
it takes effect only if all of the following hold:

- the transaction ID names a **live** coordinator in the registry;
- that transaction ID has not been **retired** (below);
- the **caller context** recorded when the operation started matches;
- the originating request identity recorded at verification matches.

**The epoch is internal and is never read from a request.** It is a process-wide monotonically
increasing counter allocated under `RegistryLock` at coordinator creation and never reused. The
platform allocates the transaction ID and cannot know or sign a value this plugin invents, so
requiring a cancellation to carry a matching epoch would be unsatisfiable. The epoch exists to make
registry identity unambiguous across a transaction ID that is created, retired, and seen again —
not to authenticate a caller.

**Retirement.** When a coordinator reaches a terminal state, its transaction ID and epoch are
recorded in a retired-ID set, capped at `MaxRetiredIds` with oldest-first eviction. A cancellation
naming a retired transaction ID is refused, even if a later coordinator claims the same ID: within
one process lifetime a transaction ID is answerable exactly once. This is what defeats replay of a
captured cancellation against a subsequent operation, and unlike an epoch-in-the-envelope scheme it
is achievable with no platform cooperation.

**Envelope requirement on T-015.** The resolved envelope must bind at minimum the operation type
and the transaction ID, so that a captured cancellation cannot be reinterpreted as another
operation or aimed at another transaction. Freshness *across process lifetimes* — replay of a
cancellation captured before a restart — cannot be defeated locally, because the retired-ID set
does not survive the process. Defeating it requires a platform-supplied nonce, counter, or
timestamp in the envelope. That is an open requirement on T-019 and stands as a **residual risk**
until answered; the local retired-ID rule bounds it to the first cancellation after a restart.

The registry enforces uniqueness on transaction ID plus epoch. A cancellation that matches no live
coordinator, or that fails any check above, is a silent no-op: no state change, no side effect, and
no log containing the supplied identifiers. Rejection must not branch on how far the match failed,
must emit one indistinguishable result for every failure kind, and must not vary in cost beyond the
coarse resolution of a single registry lookup. Cryptographic constant time is not required; a
distinguishable early-exit path is prohibited.

### Cancellation before registration

A verified cancellation whose transaction is not yet registered records a **tombstone** in the
registry under `RegistryLock`, keyed by transaction ID and caller context, retained for
`TombstoneRetention`. Coordinator creation consults and consumes any matching tombstone atomically
within the same `RegistryLock` acquisition that allocates the epoch; a coordinator created against
a live tombstone is born terminal `Canceled` and performs no decode, UI, Hello, vault access, or
publication. Tombstones are evicted on consumption or on retention expiry, whichever is first, and
the tombstone table is capped at `MaxTombstones` with oldest-first eviction.

### Effect and enforcement

A successful cancellation sets the cancellation flag and moves the operation to `Canceled` if the
terminal outcome has not already been claimed. It must actively cancel outstanding work, not
merely disregard it: any displayed UI is dismissed and any outstanding Windows Hello request is
cancelled through the platform's own cancellation facility. Ignoring an outstanding request is not
an acceptable implementation of cancellation.

A single cancellation token is created with the coordinator and passed into every UI, Hello, vault,
and signing boundary. The cancellation flag is re-checked immediately before each of:

- displaying UI;
- invoking Windows Hello;
- reading the vault;
- unprotecting key material;
- signing;
- mutating credential metadata;
- claiming the terminal outcome for publication.

A check that observes cancellation aborts the boundary before it is entered. Credential metadata is
mutated only after the terminal outcome has been claimed, so no cancellation can leave a partial
mutation and no rollback is required.

**Check and arm are one step.** For a boundary that issues an external call, the cancellation check
and the in-flight-callback increment occur within a single `CoordinatorLock` acquisition; the lock
is released and the call is then issued. A cancellation arriving in the residual window between
release and issue cannot be prevented from letting the call go out — that window is inherent, not
removable — so it is bounded instead: the issuing thread re-reads the cancellation flag immediately
after the call is accepted by the platform and, if set, invokes the platform's cancellation
facility for that call. The design does not claim a cancellation can retract a call already
accepted; it claims the call is cancelled promptly and produces no result the operation acts on.
No response, metadata mutation, or signature may follow from a call whose result arrives after the
cancellation flag was set.

## User verification freshness (F10)

A Windows Hello verification result is bound to one transaction ID and epoch, is consumed exactly
once, and expires after `VerificationLifetime`. It is discarded on any error, on cancellation, and
on any re-entry to an earlier state. It is never cached, never shared between coordinators, and
never carried across a retry. `Executing` is not reachable on a consumed, expired, or foreign
verification; an operation that needs verification again must obtain a new one. This preserves the
`security.md` §2.2 invariant that every assertion requires fresh verification.

## Lock state (F9)

`GetLockStatus` is conservative and returns `PluginLocked` unless the vault service reports ready
and no prior unrecovered error has placed the provider in a locked state. During the current
bootstrap it is always locked.

Its result is a function only of coarse provider state. It must not reflect the existence, phase,
count, or resource needs of any operation, and its latency must not vary with operation state, so
that an unauthenticated caller cannot use it to observe or time an in-flight operation. Lock status
is informational and never authorizes an operation.

## Timeouts and limits (F11)

Every nonterminal state carries a maximum lifetime. On expiry the operation becomes terminal
`Failed`: all references are released, all buffers are zeroized, and nothing is published. Expiry
is fail-closed and is never reported as success.

| Constant | Applies to |
| --- | --- |
| `MaxRequestBytes` | pre-verification caller-frame copy |
| `MaxUnverifiedConcurrent` | in-flight signature verifications |
| `MaxCoordinators` | live nonterminal coordinators |
| `StateDeadline` (per state) | each nonterminal state |
| `VerificationLifetime` | a Hello verification result |
| `TombstoneRetention`, `MaxTombstones` | pre-registration cancellations |
| `MaxRetiredIds` | retired transaction-ID set |
| `CallbackAbandonDeadline` | an external callback that never returns |

No coordinator may remain nonterminal indefinitely, so no operation can pin `DllCanUnloadNow`
against unload merely by stalling in a state.

A state deadline makes an operation terminal but does not by itself make its memory reclaimable:
destruction still waits on the in-flight-callback count. If a callback has not returned by
`CallbackAbandonDeadline`, the coordinator is **deliberately leaked** — its memory is never freed,
its module reference is never released, and `DllCanUnloadNow` continues to return `S_FALSE` — and a
distinct diagnostic event is emitted. Retaining an unloadable module is the fail-closed choice; a
use-after-free is not. This is a known, accepted liveness cost of F7 and must not be traded away
for unload availability.

## Logging and zeroization (F12)

Decoded payloads, key material, and constructed responses are zeroized on every terminal path —
`Completed`, `Canceled`, and `Failed` alike — before the owning memory is released. Zeroization is
unconditional; it is not contingent on which path reached the terminal state.

Pre-verification and verification-failure paths emit a generic event ID only. They record no
attacker-supplied bytes, no transaction identifiers, no caller-supplied signature material, and no
detail distinguishing one failure cause from another. Cancellation no-ops log nothing containing
the supplied identifiers. Logging obligations elsewhere are unchanged from `security.md` §5.

## Destruction and unload (F7)

The DLL maintains module-level live-object and server-lock counts; `DllCanUnloadNow` returns
`S_FALSE` while either is nonzero.

Every outstanding external callback holds a strong reference to its coordinator for the callback's
full lifetime. An in-flight-callback count is incremented under `CoordinatorLock` before the
external call is issued and decremented when the callback completes or is abandoned by the
platform. Coordinator destruction is permitted only after a terminal transition **and** after the
in-flight-callback count reaches zero, subject to the `CallbackAbandonDeadline` leak rule in
`Timeouts and limits`. A late callback that arrives after cancellation therefore observes a live
coordinator and a terminal state; it changes nothing and touches no freed memory. No buffer
reachable by a pending callback may be released before that count reaches zero.

## Out of scope

This design authorizes no implementation of WebAuthn decoding, signature verification, Windows
Hello, credential storage, key handling, metadata mutation, `MakeCredential`, or `GetAssertion`.
It does not define the operation-signing envelope. Implementation remains gated on T-015 and T-019.

## Appendix — finding to section

| Finding | Closed by |
| --- | --- |
| F1 pre-verification allocation | Ownership; States (first state is `SignatureVerified`) |
| F2 unordered publication | States → Terminal claim and publication |
| F3 cancellation binding | Authenticated cancellation → Binding (retired-ID rule; cross-restart replay is a stated residual risk on T-019) |
| F4 no locking model | Synchronization |
| F5 unchecked side-effect boundaries | Authenticated cancellation → Effect and enforcement |
| F6 cancel before registration | Authenticated cancellation → Cancellation before registration |
| F7 callback lifetime | Destruction and unload |
| F8 `EXPERIMENTAL_` source scan | Not here — `scripts/ContractCheck.psm1` under T-018 MED-05 |
| F9 `GetLockStatus` leaks state | Lock state |
| F10 verification freshness | User verification freshness |
| F11 no state expiry | Timeouts and limits |
| F12 zeroization and logging | Logging and zeroization |
