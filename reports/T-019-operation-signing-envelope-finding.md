# T-019 – Operation-signing envelope: finding

Status: RESOLVED (partial — see "What remains undefined")
Date: 2026-09-08
Owner: ARCHITECT
Source evidence: pinned SDK headers + Microsoft `PasskeyManager` reference sample
Supersedes the blocking premise of `reports/operation-signature-gate-design.md`

## Summary

A **candidate** envelope is now identified. It is not in the headers, and it is not documented. It
is observed behaviour of the Microsoft `PasskeyManager` sample, which constructs and verifies it
unambiguously.

**As observed, the sample verifies a signature over exactly the encoded request bytes and nothing
else** — for `WEBAUTHN_PLUGIN_OPERATION_REQUEST`, the byte range
`[pbEncodedRequest, pbEncodedRequest + cbEncodedRequest)`, the raw CTAP2 CBOR request verbatim, with
no prefix, no framing, no canonicalization step, and no other struct field mixed in.

**This does not meet T-015's acceptance bar.** T-015 requires an *exact contract-defined* byte
sequence and forbids implementation against an undefined envelope. Sample behaviour is evidence of
one implementation, not a supported semantic guarantee, and our SDK pin cannot detect a platform-side
change to it because the envelope is not a declaration. T-015 therefore stays blocked until Microsoft
confirms or rejects this construction as contractual.

What has changed is the *character* of the blockage. It was "we do not know what is signed and
cannot find out". It is now "we have a specific, testable construction and need it confirmed." That
converts an open-ended research question into a single yes/no clarification, which is what the
narrowed request in `reports/microsoft-v1-envelope-clarification-request.md` asks.

The cancellation path is not resolved even provisionally: it has no message to sign and the sample
never verifies it.

## Evidence

Two independent sources agree, and neither required inference.

**Header, `webauthnplugin.h:131`** — the field comment on `pbOpSignPubKey` states it is
"Used to sign the request in PCWEBAUTHN_PLUGIN_OPERATION_REQUEST. Refer pluginauthenticator.h."
That is a pointer to the consuming struct, not a specification of the signed bytes. The pinned
`pluginauthenticator.h:95-104` then defines the struct with `pbRequestSignature` alongside `hWnd`,
`transactionId`, `requestType` and `pbEncodedRequest`, and says nothing about which of them the
signature covers. Reading only the headers, the composition is a four-way guess. That is the
correct reason this task was opened and the correct reason nothing was implemented against a guess.

**Sample, `PasskeyManager/cpp/PluginAuthenticator/PluginAuthenticatorImpl.cpp`** — resolves the
guess. In `MakeCredential` the buffer handed to the verifier is built from the encoded request
alone:

```cpp
std::vector<BYTE> requestBuffer(
    pPluginMakeCredentialRequest->pbEncodedRequest,
    pPluginMakeCredentialRequest->pbEncodedRequest + pPluginMakeCredentialRequest->cbEncodedRequest);
```

and is passed straight to `VerifySignatureHelper(requestBuffer, pubKey…, pbRequestSignature,
cbRequestSignature)`. `GetAssertion` builds the identical buffer the same way. No other field is
appended, hashed in, or length-prefixed.

## Verification parameters

Taken from `VerifySignatureHelper` in the same file:

- Public key: `WebAuthNPluginGetOperationSigningPublicKey(clsid, …)`, imported via `NCryptImportKey`
  with `BCRYPT_PUBLIC_KEY_BLOB`. The key is also returned once at registration in
  `WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_RESPONSE.pbOpSignPubKey`.
- Digest: SHA-256, computed by the caller. `NCryptVerifySignature` receives the **hash**, not the
  message.
- Padding: the key blob's `Magic` is inspected. On `BCRYPT_RSAPUBLIC_MAGIC`, padding is
  `BCRYPT_PAD_PSS` with `BCRYPT_PSS_PADDING_INFO{ .pszAlgId = BCRYPT_SHA256_ALGORITHM, .cbSalt = 32 }`.
  Any other magic falls through with `paddingInfo = nullptr` and `dwCngFlags = 0`, i.e. the ECDSA
  path is taken implicitly and is never named. Treat the algorithm as key-blob-driven, not fixed.

The plugin must not assume RSA. The blob magic is the discriminator and must be read before
choosing padding.

## What remains undefined

Three gaps survive this finding. None is a reason to keep the lane paused; each needs a
compensating control rather than an answer from Microsoft.

**G1 — the observed signed input binds no transaction identity.** The signature appears to cover the
encoded request and only the encoded request; `transactionId`, `hWnd` and `requestType` sit outside
it. The signature alone therefore cannot establish which operation instance it was minted for. We
cannot determine from public documentation whether the platform supplies some other
operation-instance or replay binding outside the signed input — dispatch, lifetime or scoping
guarantees could exist and be undocumented. Treat replayability as an unresolved question, not a
demonstrated defect: we have not exhibited a working replay. This confirms and *widens* R1 in
`reports/lifecycle-security-review.md`: the concern was a missing freshness value, but the actual
position is that there is no platform-supplied freshness value **and** no binding to the transaction
the request arrives on.

G1 cannot be closed by an answer that merely assigns responsibility. If Microsoft replies that
plugins must defend replay themselves, that establishes *whose job it is* but does not establish
that the job is doable. A plugin-local retired-transaction-ID set only rejects replays it can still
remember, so it does not cover replay after plugin or broker process restart, after crash or state
loss, into a fresh plugin instance with no durable record, into a distinct platform-created
transaction, or across any platform context the signed material omits. Making the set durable
requires persisting transaction identifiers, which is itself a design decision with its own
exposure.

G1 therefore closes only if Microsoft additionally documents a lifecycle guarantee that bounds
replay — uniqueness and lifetime semantics for `transactionId`, or a platform nonce/counter/operation
identity a plugin can validate, or documented scoping of requests to a plugin instance, process,
session or registration. Absent that, G1 is a standing limitation requiring explicit human risk
acceptance. It is not a finding an architect may mark closed.

**G2 — cancellation has no envelope and no reference verification.**
`WEBAUTHN_PLUGIN_CANCEL_OPERATION_REQUEST` carries `transactionId` and `pbRequestSignature` and no
encoded request, so there is no message for the signature to cover. The sample's `CancelOperation`
never reads `pbRequestSignature` at all — it compares the transaction ID against the stored one and
proceeds. Consequently the T-015 rule "a cancellation request is subject to equivalent
authentication" is currently **unimplementable as written**: there is no defined byte sequence to
verify. T-015 must either specify a candidate envelope and fail closed when it does not verify, or
downgrade cancellation to an explicitly unauthenticated, non-destructive signal. That is an
architecture decision, not a blocked one.

**G3 — the source is a sample, not a normative contract.** The construction is unambiguous but it is
observed behaviour of reference code, not a documented guarantee. Microsoft Learn does not specify
the envelope. It may change without a header change, which our contract guard would not catch,
because the guard pins declarations and the envelope is not a declaration. A verification failure at
runtime must therefore be treated as a possible upstream contract drift and reported loudly, not
silently as a hostile request.

## The sample is not a security reference

Recorded so that no later task mistakes it for one. The sample's ordering is the inverse of the
T-015 gate and must not be copied:

- `WebAuthNDecodeMakeCredentialRequest` runs at line 567, **before** verification at 588.
- In `GetAssertion`, verification runs later still — after credential selection and after the
  username has been read out of the selected credential.
- The verdict is stored (`m_pluginOperationStatus.requestSignatureVerificationStatus`) and never
  consulted. `PerformUserVerification` — which raises Windows Hello UI — runs unconditionally
  immediately afterwards.
- If `GetRequestSigningPubKey()` returns empty, `requestSignResult` stays `E_FAIL` and the operation
  continues regardless. Fail-open.

The sample demonstrates *how to compute* the verification. It does not demonstrate *gating on it*.
`reports/operation-signature-gate-design.md` remains the authority on ordering, and its ordering is
correct and unchanged by this finding.

## Unblocked: a stable v1 user-verification path exists (T-007)

Incidental to the envelope question and directly resolves what T-007 was paused on.

Stable v1 `WebAuthNPluginPerformUserVerification` returns a response buffer that is **itself a
signature over the same `requestBuffer`**, verifiable with the key from
`WebAuthNPluginGetUserVerificationPublicKey` using the identical helper
(`PluginAuthenticatorImpl.cpp:298-309`). So the stable API already yields a UV proof cryptographically
bound to the request being authorised.

This is the capability the experimental v2 `pbBufferToSign` field was assumed to be needed for. It is
not needed. ADR-001's prohibition on `EXPERIMENTAL_` APIs costs us nothing here, and T-007 has a
usable stable path.

Note the sample stores this verdict without gating on it either, and skips UV entirely when its
mock vault is already unlocked. aeDae must gate.

## Consequences for the backlog

- T-019 — the research is done and the question is now precisely posed, but the task does not reach
  DONE until the clarification is answered. Its exit condition asks for an envelope "documented well
  enough for T-015 to specify verification over an exact byte sequence"; a sample observation is not
  that.
- T-015 — **stays blocked.** Its completion condition requires a security reviewer to approve a gate
  a future implementation can bind to an official envelope with no fallback behaviour. A candidate
  construction does not meet that bar. When the answer arrives, T-015 must also rule on G2 and carry
  G1 per the constraint above.
- T-007 — **stays paused.** A stable UV path appears to exist on the API surface, which narrows why
  the task is blocked, but the UV response envelope rests on the same sample observation as the
  operation envelope and is equally unconfirmed. No Windows Hello invocation is authorized while the
  protocol lane is paused. When it does proceed, the implementation must verify and fail closed
  rather than assume the construction.
- T-016 — unchanged. Its dependency on T-015 stands.
- `reports/microsoft-v1-envelope-clarification-request.md` — the blocking artifact. All three
  questions must be answered at a contractual level before T-015 moves. Filing is a human-owner
  action.
