# Windows Personal Authenticator – Security Spec

## 1. Threat model

The v1 design protects against disk theft or offline access to vault files, same-user malware attempting key extraction or misuse, and application bugs that could create invalid WebAuthn behavior or relying-party confusion.

Out of scope for v1:

- Kernel-level compromise
- Firmware compromise
- An attacker who controls the device and can satisfy the user’s Windows Hello factor

## 2. Non-negotiable invariants

1. **No plaintext private keys at rest.** Private key material must never be stored in raw form on disk.
2. **Fresh user verification before signing.** Every `GetAssertion` must require Windows Hello verification unless the platform’s documented policy explicitly permits another approved path.
3. **Relying-party integrity.** A credential for one RP ID must never be used for another RP ID.
4. **No secret logging.** Logs must never contain private keys, vault encryption keys, raw credential blobs, challenge data, or unnecessary personal identifiers.
5. **Fail closed.** Corrupt vault records, failed user verification, malformed requests, and unexpected crypto states must deny the requested operation.
6. **No bypass or extraction.** The app does not attempt to bypass Windows policy, extract third-party credentials, or clone hardware security-key secrets.

## 3. Key protection

- Generate credential keys using Windows-supported or Microsoft-supported WebAuthn cryptographic implementations.
- Encrypt each private key with a unique random data-encryption key.
- Wrap that data-encryption key with DPAPI under the current user context.
- Prefer TPM-backed or hardware-bound Windows protection when available and practical.
- Keep decrypted key material in memory only for the shortest possible signing operation.
- Clear sensitive buffers immediately after use where language/runtime facilities permit.

## 4. Windows Hello rules

- User verification is delegated to Windows Hello using documented WebAuthn plugin facilities.
- The app never implements biometric capture, biometric matching, a PIN database, or a substitute credential-verification system.
- Canceled, failed, timed-out, unavailable, or ambiguous verification outcomes produce no assertion.

## 5. Storage and logging

- Vault files live in a user-scoped application directory with ACLs appropriate to the current user.
- Database schema migrations must be transactional and recover safely after interruption.
- Debug diagnostics are off in release builds.
- Production logs use event IDs and redacted metadata, never secret values.

## 6. Security tests

### Unit tests

- Vault data does not contain PEM, DER, or readable raw private-key material.
- Wrapped material fails to decrypt outside its intended user context.
- Tampered ciphertext, metadata, and record versions safely fail.
- RP ID matching rejects cross-origin or mismatched requests.

### Integration tests

- Assertion without completed Windows Hello verification produces no signature.
- Cancellation stops outstanding operations cleanly.
- Invalid credential IDs and malformed requests fail safely.
- A credential can register and subsequently authenticate against a controlled test relying party.

### Release gate

No release is eligible until a security review explicitly checks WebAuthn request handling, cryptographic storage, DPAPI usage, user verification handling, logging, error paths, and package integrity.
