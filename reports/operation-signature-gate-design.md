# Operation signature verification gate

Status: BLOCKED for stable v1, not implemented

## Stable v1 decision

T-015 applies only to the stable v1 contract. The reviewed headers and available documentation do not define the exact signed bytes, serialization, signature algorithm, or required bindings for `WEBAUTHN_PLUGIN_OPERATION_REQUEST`. Therefore no v1 request-signature verifier may be inferred, reverse engineered, or implemented. The HIGH request-authentication finding remains open.

Experimental v2 APIs are out of scope for T-015. They are handled only by T-017 and remain prohibited from production use pending a separate security review and explicit human go/no-go decision.

## Rule

Every incoming plugin operation and cancellation request is untrusted until its platform operation signature has been verified against the current plugin operation-signing public key. Verification is a mandatory gate, not diagnostic telemetry.

The allowed ordering is:

1. Copy only bounded raw request fields needed to establish the operation identity and signature input.
2. Obtain the platform operation-signing public key through the locked official API.
3. Verify the request signature over the exact contract-defined byte sequence.
4. Check the operation has not been canceled.
5. Decode the WebAuthn payload and continue to request validation.

Before step 3 succeeds, the plugin must not decode CBOR, show UI, invoke Windows Hello, read or modify vault or credential metadata, select credentials, write logs containing request data, or create an operation response.

## Failure and cancellation behavior

- Missing, malformed, unsupported, or invalid signatures produce a generic failure with no response payload and no side effect.
- A cancellation request is subject to equivalent authentication and must match the active transaction before it can change state.
- Signature-key retrieval or signature verification failure is terminal and fail-closed. There is no cached-key, unsigned, or unprefixed-API fallback.
- The experimental v2 declarations are prohibited. Their caller-provided buffer-to-sign is not eligible for use under this design.

## Implementation boundary

A future `IOperationSignatureVerifier` must accept only raw contract request data and return a non-secret verdict. It must not own request decoding, UI, vault access, signing, or logging. The verifier implementation and its exact signing envelope require separate architecture approval because the public header does not define that envelope in this repository.

## Required tests

See `tests/integration/OperationLifecycleTestPlan.md`. The implementation must prove a failed signature prevents decode, UI, Hello, vault access, metadata changes, and response emission.
