# Windows Personal Authenticator – Functional Spec

## 1. Product overview

The Windows Personal Authenticator is a third-party WebAuthn plugin authenticator for Windows 11+ that treats **THIS PC** as a first-class authenticator.

It integrates with the Windows WebAuthn broker via the WebAuthn Plugin APIs (for example, `WebAuthNPluginAddAuthenticator` and `IPluginAuthenticator`) so browsers and apps can route `makeCredential` and `getAssertion` calls directly to this authenticator without browser extensions.

## 2. Primary user stories

- As a user, when a website requests a passkey, I can choose **This PC** as an authenticator and sign in using Windows Hello.
- As a user, I can see a dashboard that shows THIS PC status, including Hello + TPM protection and last verification time.
- As a user, I can see passkeys stored by this authenticator, including relying party, account label, and last-used time.
- As a user, I can see an Identity Health summary: total credentials, providers, devices, and risk flags.
- As a user, I can revoke or disable this authenticator from the application UI and from Windows Settings > Accounts > Passkeys > Advanced options.

## 3. Core functional requirements

### 3.1 WebAuthn plugin behavior

- The authenticator registers with Windows through WebAuthn Plugin APIs.
- It appears as a passkey provider in Windows Settings.
- Windows can route `MakeCredential` and `GetAssertion` operations to the authenticator.
- `IPluginAuthenticator.MakeCredential` must receive creation options, create a key pair using approved libraries, store the private key only in the encrypted vault, and return valid attestation/authenticator data.
- `IPluginAuthenticator.GetAssertion` must receive assertion options, locate a matching credential, require successful Windows Hello verification, sign the assertion, and return it through the Windows WebAuthn flow.
- `CancelOperation` and `GetLockStatus` must behave safely and predictably.

### 3.2 Vault and credential management

Each credential stores:

- RP ID
- User handle or privacy-safe account label
- Credential ID
- Encrypted key material
- Creation and last-used times
- User-verification requirements
- Backup and migration flags

The vault supports add, update, delete, look-up by RP ID and credential ID, and enumeration for the UI.

### 3.3 Identity Health and migration assistant

The app computes and displays:

- Total credentials, providers, and devices known to the app
- Credentials that exist only on this PC
- Accounts appearing to have one known authenticator
- Obvious duplicates for the same RP and user

The app must not attempt to export, clone, or extract key material from third-party managers or hardware security keys. It may use only legitimately exposed metadata or user-provided information.

### 3.4 User interaction model

- Normal passkey registration and sign-in stay inside the Windows WebAuthn flow.
- The only expected security prompt during assertions is Windows Hello when verification is required.
- Management work happens after an explicit app launch.
- The app avoids extra confirmation prompts unless an action deletes credentials, disables the provider, changes security settings, or creates a material risk.

## 4. Non-goals for v1

- No cross-device synchronization of private keys.
- No Windows sign-in credential provider implementation.
- No custom cryptographic primitives.
- No browser extension for core functionality.
- No migration of non-exportable foreign passkeys.
