# Transaction-safe operation lifecycle

Status: design-only, not implemented

## Ownership

One `OperationCoordinator` owns one platform transaction. It is the only component allowed to transition state, publish a response, or release the operation's lifetime reference. COM object lifetime and server-lock accounting must retain the DLL while a coordinator is nonterminal.

## States

```text
Idle
  -> Received                 raw bounded request copied
  -> SignatureVerified        platform signature accepted
  -> Decoded                  payload decoded and structurally valid
  -> AwaitingUserAction       optional non-secret UI decision
  -> AwaitingUserVerification fresh Windows Hello is outstanding
  -> Executing                future authorized operation only
  -> ResponseReady            response built but not yet published
  -> Completed                response published exactly once

Any nonterminal state -> Canceled | Failed
```

`Canceled`, `Failed`, and `Completed` are terminal. The coordinator owns an atomic terminal-state transition. The first successful terminal transition wins; all later work observes that result, clears transient buffers where applicable, and emits no additional response.

## Cancellation

Cancellation may arrive concurrently with every nonterminal state. It must first pass the operation-signature gate and transaction-ID match. A successful cancellation moves the operation to `Canceled`, prevents UI continuation, cancels or ignores later Hello completion, prevents vault or metadata mutation, and prevents response publication. A completed operation remains completed; a late cancellation changes neither result nor state.

## Lock state

`GetLockStatus` is conservative: return `PluginLocked` unless the future vault service reports ready, the coordinator has no active nonterminal operation that requires an unavailable protected resource, and no prior unrecovered error has placed the provider in a locked state. During the current bootstrap, it must be treated as locked. Lock status is informational only and never authorizes an operation.

## Destruction and unload

The DLL must maintain module-level live-object and server-lock counts. `DllCanUnloadNow` returns `S_FALSE` while either count is nonzero. Coordinator destruction is permitted only after a terminal transition and completion of any joined worker or callback; it must not free buffers still reachable by a Windows callback.

## Out of scope

This design authorizes no implementation of WebAuthn decoding, signature verification, Windows Hello, credential storage, key handling, metadata mutation, `MakeCredential`, or `GetAssertion`.

