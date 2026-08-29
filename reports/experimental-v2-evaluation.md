# Experimental WebAuthn v2 evaluation

Status: planning only. No implementation, registration, Windows Hello call, credential operation, or production use is authorized.

## Purpose

Evaluate whether the explicitly experimental v2 plugin APIs could support a non-production prototype with a documented caller-buffer signing envelope. This work does not unblock stable v1 and does not change T-015.

## Required evidence before any prototype

- Minimum supported Windows build and every required experimental enablement binary, verified from Microsoft-controlled sources.
- Exact v2 declarations, ABI, buffer lifetime rules, and availability behavior pinned to a reviewed SDK contract.
- A proposed canonical envelope binding RP ID, operation type, transaction GUID, challenge, caller context, version, and algorithm identifier.
- Fail-closed verification, cancellation binding, replay resistance, downgrade behavior, and no fallback to v1 or unprefixed APIs.
- A separate security review and explicit human approval.

## Pinned SDK observations

The locked SDK `10.0.26100.0` declares `EXPERIMENTAL_WebAuthNPluginAddAuthenticator2` and `EXPERIMENTAL_WebAuthNPluginPerformUserVerification2`. The v2 registration options use `const CLSID* pClsid` and add `pwszUserVerificationKeyName`. The v2 user-verification request uses `const GUID* pGuidTransactionId` and a caller-owned `cbBufferToSign` / `pbBufferToSign` pair. These declarations establish no supported availability, enablement, signing algorithm, canonical serialization, or production suitability.

## Proposed envelope, for evaluation only

Any future prototype proposal must use a versioned canonical binary envelope that binds: envelope version, algorithm identifier, operation type, transaction GUID, RP ID, canonical challenge bytes and length, caller-context identifier, and an explicit anti-replay nonce or generation. Fields must be length-delimited, unambiguous, and rejected on duplicate, unknown-required, truncated, noncanonical, or unsupported values. This is a design hypothesis only, not an implementation authorization.

## Threats and future negative tests

- Truncated, reordered, partially bound, or ambiguously encoded fields must fail before UI, vault, or response work.
- A transaction GUID, RP ID, operation-type, challenge, caller-context, algorithm, or version mismatch must fail closed.
- Replay across transactions, RPs, operations, or provider generations must fail.
- Any unavailable v2 capability, enablement failure, or downgrade attempt must fail closed with no v1, unsigned, or unprefixed fallback.
- Cancellation must authenticate and bind to the same transaction and envelope generation.

## Prohibitions

- Disabled by default.
- No use outside controlled development or test environments.
- No production registration, credential storage, signing, or user-verification invocation.
- No claim that v2 availability or semantics extend to stable Windows releases.

## Microsoft clarification request

Ask Microsoft to publish a normative stable-v1 operation-signature envelope, including exact bytes, serialization, algorithm, and required operation/RP/challenge/transaction bindings; or to confirm a future stable API with documented v2-style buffer-to-sign semantics.
