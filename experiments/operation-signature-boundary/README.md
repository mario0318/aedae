# Synthetic operation-signature boundary lab (T-024)

Run `npm test` with Node.js. The tests create an ephemeral RSA-PSS/SHA-256
keypair in memory and never call Windows WebAuthn, the plugin, Windows Hello,
the vault, or a network service. No key or request is written to disk.

The model signs and verifies only `encodedRequest`. That is the input observed
in Microsoft's PasskeyManager sample, **not a confirmed Windows contract**.
Changing those bytes fails verification. Transplanting the same signed bytes
to a different synthetic transaction ID, request type, or window context
still verifies because none of those fields enters the signature. This is a
boundary demonstration, not proof that a replay can reach a real broker or
that Windows fails to provide some other binding.

The lab cannot resolve G1 without a documented broker lifetime/freshness
guarantee or an authenticated per-operation identity. It does not model G2:
the cancellation signing message is undefined, so a truthful positive
cancellation-verification test cannot be written. G3 remains because the
input is sample-observed rather than normative. Nothing here unblocks T-015
or authorizes plugin registration, request decoding, Hello, real keys, or
production signing.
