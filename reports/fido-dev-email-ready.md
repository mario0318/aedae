Sent on 2026-09-11 at 17:22 ET to `fido-dev@microsoft.com`, in the same Gmail thread as the
2026-09-04 original request. Gmail message ID: `1a092905b447e002`. The exact sent subject and body
are retained below as the communication record. T-019 now awaits Microsoft's response.

To:      fido-dev@microsoft.com
Subject: Clarification requested: stable v1 WebAuthn plugin request-signature contract, cancellation authentication, and replay/transaction binding

---

Hello,

We are evaluating the stable Windows WebAuthn plugin APIs for a third-party plugin authenticator. We
need to authenticate operation and cancellation requests before any protocol side effect, including
CBOR decoding, user interaction, Windows Hello invocation, credential selection, vault access, or
response generation.

The public headers expose an operation-signing public key and request signature fields, but we have
not found normative documentation for the signed-message contract, cancellation signature semantics,
or replay binding.

The official PasskeyManager sample appears to verify a request signature over
SHA-256(pbEncodedRequest[0..cbEncodedRequest)) and appears to select verification algorithm/padding
based on the public-key blob magic. We understand that sample behavior is not necessarily a
supported contract.

Could Microsoft clarify the following for stable v1?

1. Is the sample-observed operation-request construction a contractual, supported v1 requirement for
   third-party plugins?

   Specifically, is the signed message exactly pbEncodedRequest[0..cbEncodedRequest), hashed with
   SHA-256, and verified using an algorithm and padding selected based on the operation-signing
   public-key blob magic?

   If yes, please identify the authoritative documentation and version policy, key-blob formats,
   supported algorithms and padding rules, malformed-input behavior, key-rotation behavior, and
   compatibility guarantees. If no, please identify the supported verification contract, or confirm
   that stable v1 does not provide one for third-party plugin implementations.

2. What authenticates WEBAUTHN_PLUGIN_CANCEL_OPERATION_REQUEST?

   The structure carries cbRequestSignature / pbRequestSignature but no pbEncodedRequest equivalent.
   Please document whether this signature is populated, the exact bytes it covers, its algorithm and
   key requirements, and how it binds cancellation to the intended authenticator and active
   operation/transaction.

   If cancellation is intentionally an unauthenticated signal, please confirm that explicitly and
   describe the required plugin behavior for treating cancellation as untrusted.

3. Is the absence of transaction binding from the operation-request signed material intentional?

   Our reading of the sample is that transactionId, hWnd, and requestType are outside the observed
   signed input. If the observed signed input excludes transactionId, hWnd, and requestType, we
   cannot determine from public documentation whether the platform supplies any other
   operation-instance or replay binding outside the signed input. Please document the replay and
   transaction-binding model:

   - Is a plugin expected to provide replay prevention itself?
   - What platform guarantee bounds the lifetime and uniqueness of a transaction ID?
   - Is a durable retired-transaction-ID set expected, supported, or sufficient across
     plugin-process restart or crash?
   - Is there a documented platform nonce, counter, timestamp, or operation identity that
     third-party plugins can validate?
   - Are operation requests scoped to a particular plugin instance, process, Windows session, or
     authenticator registration in a documented way?

If the stable-v1 API intentionally does not provide a documented envelope and transaction-bound
anti-replay primitive, please confirm that stable v1 is not intended to support this level of
pre-side-effect request authentication for third-party plugin implementations.

Thank you.
