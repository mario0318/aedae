# Operation lifecycle test plan stubs

These are test specifications only. They do not invoke WebAuthn, Windows Hello, vault, crypto, or experimental APIs.

Rows OL-001 through OL-007 carry the extended evidence required by `reports/lifecycle-security-review.md`; the finding each row answers is cited inline.

| ID | Scenario | Required evidence |
| --- | --- | --- |
| OL-001 | Invalid operation signature | No decode, UI, Hello, vault call, metadata change, or response payload. After N invalid requests, live-object count, server-lock count, coordinator-registry size, and peak allocation are unchanged from baseline and `DllCanUnloadNow` returns `S_OK`. Emitted log records contain no request-derived bytes or identifiers. (F1, F12) |
| OL-002 | Missing or malformed signature | Same fail-closed result and same resource assertions as OL-001. An oversized request is rejected without allocation; requests beyond `MaxUnverifiedConcurrent` are rejected before the caller-frame copy. Log records are identical to OL-001 and do not distinguish the failure cause. (F1, F12) |
| OL-003 | Authenticated cancellation races a pending UI or Hello completion | One case per boundary — UI display, Hello invocation, vault read, key unprotect, signing, metadata mutation, pre-publication claim. Each asserts the boundary is not entered or is actively aborted, that outstanding UI and Hello are cancelled rather than ignored, that no metadata row changes, and that no response is published. (F5) |
| OL-004 | Completion races cancellation | A counting, fault-injecting publisher under forced interleavings across the claim/publish window: exactly one publisher invocation and exactly one terminal result. Publisher failure after a successful claim yields terminal `Failed` with no retry and no second call. The constructed response is zeroized on the cancel-wins path. (F2, F12) |
| OL-005 | Unauthenticated or transaction-mismatched cancellation | Active operation unchanged, no side effect. Replay matrix: a captured valid cancellation replayed against a recreated coordinator with the same transaction ID, a different concurrent coordinator, a colliding transaction ID, and a different caller context — all four are no-ops, indistinguishable in result and in timing. No log record contains the supplied identifiers. (F3, F12) |
| OL-006 | Lock-state query during all nonterminal states | Conservative `PluginLocked` result without authorizing work. Returned value and call latency are identical across every nonterminal state of an active operation and reveal no operation existence, phase, or count. (F9) |
| OL-007 | DLL unload during an active operation | `DllCanUnloadNow` returns `S_FALSE` while an operation is live. Release occurs only after the operation is terminal and the in-flight-callback count reaches zero. With a pending fake Hello callback, cancel and release the coordinator, then fire the callback: no invalid memory access under ASAN or Application Verifier, and no state change. (F7) |
| OL-008 | Future SDK contract drift | The contract check fails when an approved header hash, required symbol, IID, or declaration form changes, **and** when any file under `src/`, `build/`, or `artifacts/` references an `EXPERIMENTAL_` symbol. (F8) |
| OL-009 | Bootstrap COM lifetime | The activation harness proves `DllCanUnloadNow` is `S_FALSE` while a factory or authenticator instance is retained, then `S_OK` after release. |
| OL-010 | Authenticated cancellation arrives before its operation registers | The operation is born terminal `Canceled` from the pre-cancellation tombstone; no decode, UI, Hello, vault access, or response. Tombstones expire at `TombstoneRetention` and evict oldest-first at `MaxTombstones`. (F6) |
| OL-011 | Nonterminal-state expiry | Each nonterminal state held past its `StateDeadline` becomes terminal `Failed`; no response; buffers zeroized; `DllCanUnloadNow` returns `S_OK` after release. Coordinator creation is refused beyond `MaxCoordinators`. (F11) |
| OL-012 | Retry after a consumed Hello verification | A failure after successful fake Hello followed by retry yields a new Hello invocation or a fail-closed terminal result, never a signature on the consumed verification. A verification past `VerificationLifetime`, or one belonging to another transaction or epoch, is refused. (F10) |
| OL-013 | Cancellation latency and lock discipline under a pending Hello | Cancel from a second thread returns within the stated bound while fake Hello is outstanding. Randomized interleavings of start, cancel, Hello completion, and publish across multiple coordinators show no deadlock, no lock held across an instrumented external boundary, and no double completion. (F4) |

| OL-014 | Cancellation naming a retired transaction ID | A transaction ID that reached a terminal state is refused for the remainder of the process lifetime, including after a new coordinator claims the same ID. The refusal is indistinguishable in result and cost from every other cancellation rejection. `MaxRetiredIds` evicts oldest-first. (Re-review R1) |
| OL-015 | External callback that never returns | Past `CallbackAbandonDeadline` the coordinator is deliberately leaked, a distinct diagnostic event is emitted, `DllCanUnloadNow` continues to return `S_FALSE`, and no memory reachable by the pending callback is freed. (Re-review R3) |

Cross-restart cancellation replay is **not** covered by any row here. It is an accepted residual risk pending T-019; see `reports/operation-lifecycle-design.md` → Authenticated cancellation → Binding.

## Exit criteria for future implementation

Tests must use controlled fakes for the signature verifier, UI boundary, Hello boundary, vault boundary, and response publisher. No test fixture may contain a private key or connect to a relying party.
