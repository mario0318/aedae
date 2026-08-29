# Draft: WebAuthn Plugin v1 operation-signature clarification request

Subject: Normative verification contract for `WEBAUTHN_PLUGIN_OPERATION_REQUEST`

The Windows SDK documents `pbOpSignPubKey` in `WEBAUTHN_PLUGIN_ADD_AUTHENTICATOR_RESPONSE` as an operation-signing public key and provides `WebAuthNPluginGetOperationSigningPublicKey`. However, the pinned SDK contract does not specify the normative verification envelope for v1 operation requests.

Please provide the exact signed bytes, serialization/canonicalization rules, signature algorithm and key format, and required bindings for RP ID, challenge, operation type, transaction identity, caller context, and cancellation. If v1 verification is intentionally not available for third-party implementations, please confirm whether a future stable API will provide documented semantics comparable to the experimental caller-buffer signing model.

Until that clarification is available, we will not infer, reverse engineer, or implement v1 request verification.
