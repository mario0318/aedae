# Operation lifecycle test plan stubs

These are test specifications only. They do not invoke WebAuthn, Windows Hello, vault, crypto, or experimental APIs.

| ID | Scenario | Required evidence |
| --- | --- | --- |
| OL-001 | Invalid operation signature | No decode, UI, Hello, vault call, metadata change, or response payload. |
| OL-002 | Missing or malformed signature | Same fail-closed result as OL-001. |
| OL-003 | Authenticated cancellation races a pending UI or Hello completion | `Canceled` wins if it reaches the terminal transition first; no response is published. |
| OL-004 | Completion races cancellation | Exactly one terminal result and at most one response publication. |
| OL-005 | Unauthenticated or transaction-mismatched cancellation | Active operation is unchanged and no side effect occurs. |
| OL-006 | Lock-state query during all nonterminal states | Conservative `PluginLocked` result without authorizing work. |
| OL-007 | DLL unload during an active operation | `DllCanUnloadNow` returns `S_FALSE`; release occurs only after the operation is terminal and callbacks are joined. |
| OL-008 | Future SDK contract drift | The contract check fails when an approved header hash, required symbol, IID, or declaration form changes. |
| OL-009 | Bootstrap COM lifetime | The activation harness proves `DllCanUnloadNow` is `S_FALSE` while a factory or authenticator instance is retained, then `S_OK` after release. |

## Exit criteria for future implementation

Tests must use controlled fakes for the signature verifier, UI boundary, Hello boundary, vault boundary, and response publisher. No test fixture may contain a private key or connect to a relying party.
