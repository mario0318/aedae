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

## Prohibitions

- Disabled by default.
- No use outside controlled development or test environments.
- No production registration, credential storage, signing, or user-verification invocation.
- No claim that v2 availability or semantics extend to stable Windows releases.

## Microsoft clarification request

Ask Microsoft to publish a normative stable-v1 operation-signature envelope, including exact bytes, serialization, algorithm, and required operation/RP/challenge/transaction bindings; or to confirm a future stable API with documented v2-style buffer-to-sign semantics.
