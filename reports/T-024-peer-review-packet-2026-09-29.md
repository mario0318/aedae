TASK: Review the isolated synthetic operation-signature boundary lab and its task exception for security, accuracy, and scope. The owner asked us to progress the system-wide plugin without evading project standards. This is not a production verifier or approval to bypass T-019/T-015.
MODE: first-pass
FILES_TOUCHED:
  - tasks.md (only the T-024 additions; the worktree already contains unrelated earlier changes)
  - experiments/operation-signature-boundary/package.json
  - experiments/operation-signature-boundary/model.mjs
  - experiments/operation-signature-boundary/model.test.mjs
  - experiments/operation-signature-boundary/README.md
DIFF / PROPOSED ACTIONS:
  - tasks.md creates an expressly lab-only T-024 exception: generated ephemeral synthetic keys and requests, offline, no platform-key import, Windows/plugin/Hello APIs, WebAuthn decoding, real credentials, registration, production changes, or verifier implementation. T-019/T-015 and the all-lanes prohibition remain binding. Security review required before DONE.
  - package.json: private ESM package, test command `node --test`, no dependencies.
  - model.mjs exports createSyntheticRequest(encodedRequest, overrides) returning a copy of encoded bytes and synthetic transactionId, requestType, and windowContext fields. createEphemeralSigner() generates an RSA-2048 key pair; signRequest signs SHA-256 over request.encodedRequest using PSS salt length 32; verifyRequest verifies only request.encodedRequest using the generated public key. Keys remain process-local and are never exported.
  - model.test.mjs: five tests prove valid request accepted; changed encoded byte rejected; changed transactionId accepted; changed requestType/windowContext accepted; another ephemeral public key rejects the signature.
  - README explicitly states that the signed input is observed in Microsoft's PasskeyManager sample, not a confirmed contract or demonstrated broker replay. G1 requires documented broker freshness/binding or a new authenticated per-operation identity; G2 cancellation input undefined; G3 sample-vs-contract drift remains. Nothing unblocks plugin work.
TESTS:
  npm test: 5 pass, 0 fail, exit 0.
  git diff --check: exit 0.
CONSTRAINTS:
  security.md requires RP integrity, fresh Hello before assertion, fail-closed malformed requests, and no real secret logging.
  architecture.md requires signature verification before decode/UI/Hello/vault and authenticated transaction-bound cancellation.
  tasks.md has a binding no-protocol-work prohibition while T-019/T-015 paused, with narrowly separated experiments.
  Protected ABI files, AGENTS.md, and CODEOWNERS may not be edited by an agent.
  Do not interpret the lab as proof of a real Windows exploit or as satisfying any production acceptance criterion.
  Only review the packet. Return structured findings; do not edit or use tools.
