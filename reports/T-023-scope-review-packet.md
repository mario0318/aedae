TASK: User approved a separate local-only Windows Hello relying-party prototype while keeping the Windows plugin protocol lane paused.
MODE: first-pass
FILES_TOUCHED:
  - tasks.md
  - reports/ADR-003-windows-hello-rp-track-proposal.md
DIFF / PROPOSED ACTIONS:
  - tasks.md adds a T-023 exception to its all-lanes prohibition, limited to user-gesture browser WebAuthn registration/authentication on a loopback origin and RP verification by a pinned library.
  - T-023 explicitly forbids native/plugin Hello calls, key access/storage, plugin registration/decoding, product-vault credential metadata mutation, production identities, signing code, and experimental APIs.
  - The isolated demo will use experiments/hello-rp/, in-memory synthetic accounts and public credentials, exact local origin/RP ID, one-use challenges, and virtual-authenticator-only automated tests.
  - ADR-003 records that this is not a system-wide authenticator or a resolution of the Microsoft signing envelope.
TESTS:
  - None yet; this packet is for scope/gate review before implementation.
CONSTRAINTS:
  - security.md, functional.md, architecture.md, tasks.md precedence. Report any conflict; do not invent a resolution.
  - The plugin protocol lane remains paused on T-019; four human-only ABI contract files cannot be edited.
  - Review only. Do not execute tools, mutate files, or attempt a credential ceremony.
  - Assess whether the exception has any pathway to weaken the plugin security gate, and what mandatory checks the demo needs.
