# Overnight Build Framework – Windows Personal Authenticator

## Phase A – Setup and reference (~30 minutes)

### Prerequisites

```powershell
winget install Microsoft.VisualStudio.2022.Community
winget install Microsoft.WindowsSDK.10.0.26100.7175
```

### Step 1: Clone the reference sample

```bash
git clone https://github.com/microsoft/Windows-classic-samples.git
cd Windows-classic-samples/Samples/PasskeyManager
```

Why: this is Microsoft's reference implementation for a third-party passkey manager plugin, including COM registration, `IPluginAuthenticator`, credential metadata management, and Windows Hello integration hooks.[page:99]

### Step 2: Study an independent implementation

```bash
git clone https://github.com/kee-org/KeePassPasskey.git
```

Focus on:

- `Provider/Provider.cpp` – COM server implementation.
- `Plugin/PasskeyPlugin.cpp` – vault/plugin integration.
- `PluginRegistrationManager.h` – registration logic.

Architecture takeaway: for tonight, a **two‑process model** (UI/provider process + plugin DLL loaded by Windows) is a good default; the Microsoft sample already separates plugin code, registration, and UI concerns into `PluginAuthenticatorImpl`, `PluginRegistrationManager`, `PluginCredentialManager`, and XAML pages.[page:99]

---

## Phase B – Core skeleton (3–4 hours)

### Step 1: Customize identifiers

Use the same fields the sample expects you to fill in.[page:99]

**`Package.appxmanifest`**

```xml
<Identity Name="YourCompany.PersonalAuthenticator"
          Publisher="CN=YourCompany"
          Version="1.0.0.0" />
```

**`PluginManagement/PluginRegistrationManager.h`**

```cpp
// Generate with guidgen.exe or VS "Create GUID"
const wchar_t* PLUGIN_AAGUID = L"YOUR-NEW-GUID-HERE";
const wchar_t* PLUGIN_NAME   = L"Personal Authenticator";
const wchar_t* PLUGIN_RPID   = L"yourdomain.com";
const CLSID    PLUGIN_CLSID  = { /* your new GUID */ };
```

### Step 2: Register the plugin

Follow the sample’s pattern and call the documented registration API from your UI or provider process.[page:99]

```cpp
HRESULT RegisterPlugin()
{
    HRESULT hr = WebAuthNPluginAddAuthenticator2(
        PLUGIN_AAGUID,
        PLUGIN_NAME,
        PLUGIN_RPID,
        WEBAUTHN_PLUGIN_OPTIONS_NONE,
        nullptr,
        nullptr
    );
    return hr;
}
```

Critical: after registering and enabling your plugin under **Settings → Accounts → Passkeys → Advanced options**, you should see your authenticator listed and selectable in WebAuthn flows.[page:99]

### Step 3: Implement `IPluginAuthenticator`

Use the sample’s `PluginAuthenticatorImpl` and its UI pages as the baseline.[page:99]

**`PluginAuthenticatorImpl.cpp` (simplified MakeCredential)**

```cpp
HRESULT PluginAuthenticatorImpl::PluginMakeCredential(
    IPluginMakeCredential* request,
    IPluginAuthenticatorResponse** response)
{
    // 1. Extract parameters from `request`
    // 2. Generate credential ID
    // 3. Create key pair using the WebAuthn helper library
    // 4. Store private key via vault (encrypted with DPAPI/TPM)
    // 5. Build authenticatorData and response structures
    // 6. Return the response to Windows
}
```

Other required methods (match the sample's shape):

```cpp
HRESULT PluginAuthenticatorImpl::PluginGetAssertion(
    IPluginGetAssertion* request,
    IPluginAuthenticatorResponse** response);

HRESULT PluginAuthenticatorImpl::CancelOperation();
HRESULT PluginAuthenticatorImpl::GetLockStatus(BOOL* isLocked);
```

Keep this layer thin: protocol parsing/serialization plus calls into a separate vault/key‑protection layer.

---

## Phase C – Health engine and minimal UI (2–3 hours)

### Step 1: User verification (security critical)

Always require Windows Hello before private‑key use.

```cpp
HRESULT VerifyUserWithHello()
{
    // Use the documented WebAuthn plugin UV API (WebAuthNPluginPerformUv or newer helper)
    // and treat any failure/cancel as a hard stop.
}
```

```cpp
HRESULT PluginAuthenticatorImpl::PluginGetAssertion(...)
{
    HRESULT hr = VerifyUserWithHello();
    if (FAILED(hr)) return hr;

    auto credential = LoadCredentialFromVault(credentialId); // still encrypted at rest
    return credential->Sign(challenge, response);
}
```

Never allow `PluginGetAssertion` to sign without a successful verification call.

### Step 2: Credential metadata sync

Use the plugin credential APIs to let Windows know about credentials you manage (for autofill / listing), matching the sample’s `PluginCredentialManager` behavior.[page:99]

```cpp
HRESULT SyncCredentialToWindows(const CredentialInfo& cred)
{
    WEBAUTHN_CREDENTIAL_DETAILS details = {};
    details.dwVersion      = WEBAUTHN_CREDENTIAL_DETAILS_VERSION_1;
    details.rpId           = cred.rpId.c_str();
    details.userName       = cred.userName.c_str();
    details.userId         = cred.userId.data();
    details.userIdLen      = static_cast<ULONG>(cred.userId.size());
    details.credentialId   = cred.credentialId.data();
    details.credentialIdLen= static_cast<ULONG>(cred.credentialId.size());

    HRESULT hr = WebAuthNPluginAuthenticatorAddCredentials(
        PLUGIN_AAGUID,
        &details,
        1
    );
    return hr;
}
```

On deletion, call `WebAuthNPluginAuthenticatorRemoveCredentials` with matching metadata.

### Step 3: Identity Health dashboard

For tonight, a console view is enough; later you can reuse the same data for a WinUI screen.

```cpp
void BuildHealthDashboard()
{
    WEBAUTHN_PLATFORM_CREDENTIAL_LIST* platformCreds = nullptr;
    HRESULT hr = WebAuthNGetPlatformCredentialList(
        WEBAUTHN_CTX,
        WEBAUTHN_PLATFORM_CREDENTIAL_LIST_VERSION_1,
        &platformCreds
    );

    auto myCreds = LoadVaultCredentials();

    int totalCreds = static_cast<int>(myCreds.size());
    if (SUCCEEDED(hr) && platformCreds)
        totalCreds += platformCreds->dwCredentialCount;

    int rps       = CountRelyingParties(myCreds, platformCreds);
    int localOnly = CountLocalOnly(myCreds);

    RenderHealthView(totalCreds, rps, localOnly, "THIS PC", "YES", "TPM: OK");
}

void RenderHealthView(int total, int rps, int localOnly,
                      const char* device, const char* hello, const char* tpm)
{
    printf("
=== IDENTITY HEALTH ===
");
    printf("Device: %s (Protected ✓)
", device);
    printf("Windows Hello: %s
", hello);
    printf("TPM Status: %s
", tpm);
    printf("Total Credentials: %d
", total);
    printf("Relying Parties: %d
", rps);
    printf("Local-Only Credentials: %d
", localOnly);

    if (localOnly > 0)
        printf("⚠ Consider adding backup authenticators.
");
}
```

Key point: this uses only metadata from your vault plus `WebAuthNGetPlatformCredentialList` for Windows Hello platform credentials; it does not try to enumerate other managers’ private stores.

---

## Quick build and deployment commands

```powershell
cd Windows-classic-samples\Samples\PasskeyManager
msbuild PasskeyManager.sln /p:Configuration=Release

# After packaging your own MSIX variant:
Add-AppxPackage -Path .ind\Release\YourAuthenticator.msix
```

You can then enable/disable your authenticator under **Settings → Accounts → Passkeys → Advanced options**.

---

## API implementation checklist

| Component                | Task                                   | Status |
|--------------------------|----------------------------------------|--------|
| Plugin registration      | `WebAuthNPluginAddAuthenticator2`     | ☐      |
| `IPluginAuthenticator`   | `PluginMakeCredential`                | ☐      |
|                          | `PluginGetAssertion`                  | ☐      |
|                          | `CancelOperation`                     | ☐      |
|                          | `GetLockStatus`                       | ☐      |
| User verification        | `WebAuthNPluginPerformUv` / newer API | ☐      |
| Credential metadata sync | `WebAuthNPluginAuthenticatorAddCredentials` / `RemoveCredentials` | ☐ |
| Platform audit           | `WebAuthNGetPlatformCredentialList`   | ☐      |
| Packaging                | MSIX manifest with correct CLSID      | ☐      |

---

## Tonight’s minimum viable tests

1. Plugin appears and is toggleable in **Settings → Accounts → Passkeys → Advanced options**.
2. `makeCredential` works against `https://webauthn.io` or a local test RP.
3. `getAssertion` works against the same RP.
4. Windows Hello prompt appears before any key‑use operation; failure/cancel aborts.
5. Console (or UI) prints basic Identity Health metrics.
6. `CancelOperation` returns control to the browser without hanging the WebAuthn flow.

---

## Common pitfalls to avoid

- Don’t try to use an unpackaged EXE as the plugin; the platform expects a packaged app (MSIX).
- Don’t invent new crypto or WebAuthn serialization; rely on the Microsoft `webauthn` library and documented structures.
- Don’t skip user verification or weaken it to reduce prompts.
- Don’t hardcode a shared AAGUID across products; generate unique IDs.
- Don’t ignore `CancelOperation`; it runs in the hot path when the user cancels or the platform times out.

---

## Next steps beyond tonight

- Add a proper encrypted vault (SQLite + DPAPI/TPM) and move test keys into it.
- Introduce Hello session caching (with strict time bounds) to reduce repetitive prompts.
- Replace the console dashboard with a WinUI 3 Identity Health view.
- Design optional, explicit backup/export that still avoids ever writing plaintext private keys.
