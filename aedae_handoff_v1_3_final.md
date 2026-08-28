# æDæ — Project Status & Handoff Documentation

## R3 Labs • Windows Personal Authenticator

---

**Document Version:** 1.3 (FINAL — CORRECTED & APPROVED)  
**Date:** August 28, 2026  
**Status:** Bootstrap Complete — Security Gates Pending  
**Prepared By:** R3 Labs — Project æDæ Team  
**Next Milestone:** T-014 Contract Security Review → T-015 → T-016 → T-004

---

## TABLE OF CONTENTS

1. Executive Summary
2. Current Status Overview
3. What Is In Place (Bootstrap Complete)
4. Security Gates Blocking Real Work
5. Corrected Implementation Sequence (Post-Approval)
6. Repository Hygiene — Critical Action
7. Key Files & References
8. Action Items — Immediate
9. Full Findings & Technical Research
10. Market Analysis & Competitive Landscape
11. Brand Strategy & Domain Portfolio
12. Product Architecture & Technical Design
13. Monetization Strategy
14. Risk Assessment & Mitigation
15. Appendices

---

## 1. EXECUTIVE SUMMARY

### 1.1 Project Overview

**æDæ** (pronounced "idea") is a Windows 11 Personal Authenticator developed by R3 Labs. It aims to transform a standard Windows PC into a first-class authentication device by leveraging the Windows WebAuthn plugin architecture.

### 1.2 Current State

**Status:** Bootstrap Complete — Intentionally Nonfunctional

The project has successfully established:
- Native COM plugin skeleton
- Management executable
- Activation harness
- Package layout
- Locked SDK-header checks

**Critical:** The bootstrap is intentionally nonfunctional until security gates T-014, T-015, and T-016 close. The project explicitly prohibits registration, WebAuthn decoding, credential storage, signing, Windows Hello, and experimental v2 APIs before those gates close.

### 1.3 Immediate Priority

1. **Close T-014** — Pinned source/header contract verification + COM unload lifecycle
2. **Security approval for T-015** — Operation-signature gate
3. **Security approval for T-016** — Transaction lifecycle design
4. **Commit initial baseline** — After `.gitignore` staging audit

### 1.4 Critical Distinction: Stable vs. Experimental APIs

Your SDK finding shows enhanced plugin APIs are exposed as:

```text
EXPERIMENTAL_WebAuthNPluginAddAuthenticator2
EXPERIMENTAL_WebAuthNPluginPerformUserVerification2
```

Decision table:

| Situation | Correct decision |
|---|---|
| Stable v1 APIs provide all requirements, including secure broker-operation verification and required Hello interaction | Use stable APIs and keep v2 forbidden |
| The required security/process behavior depends on experimental v2 APIs | Do not build protocol v1 yet; the product is blocked pending a deliberate experimental-API acceptance decision |
| The stable API has no way to meet your required security invariants | Revise the invariant or stop/pause the platform-authenticator implementation; do not invent a workaround |

**Do not** state "Windows Hello wrapper — Call `PerformUserVerification` (v1 only)" until the pinned header manifest confirms that the stable function exists, has the required semantics, and is appropriate for this plugin flow.

---

## 2. CURRENT STATUS OVERVIEW

### 2.1 Milestone Status

| Task ID | Description | Status |
|---|---|---|
| T-001 | Project bootstrap | ✅ IN_REVIEW |
| T-002 | COM plugin skeleton | ✅ IN_REVIEW |
| T-003 | Management executable | ✅ IN_REVIEW |
| T-004 | Safe registration manager | 🔒 BLOCKED (requires T-015/T-016) |
| T-005 | Vault schema/CRUD | 🟡 CAN PROCEED (synthetic data only) |
| T-006 | DPAPI key protection | 🟡 CAN PROCEED (synthetic data only) |
| T-007 | Windows Hello wrapper | 🔒 BLOCKED (requires T-014 confirmation) |
| T-008 | Credential creation | 🔒 BLOCKED (requires T-015/T-016) |
| T-009 | Assertions | 🔒 BLOCKED (requires T-015/T-016) |
| T-010 | Management UI | 🟡 CAN PROCEED (static/mock only) |
| T-011 | Management UI (extended) | 🟡 CAN PROCEED (static/mock only) |
| T-012 | Final security review | 🔒 BLOCKED |
| T-013 | Package readiness | ✅ IN_REVIEW |
| T-014 | Contract security review | 🔴 ACTIVE GATE |
| T-015 | Operation-signature gate | 🔒 BLOCKED ON T-014 |
| T-016 | Transaction lifecycle design | 🔒 BLOCKED ON T-014 |

### 2.2 Security Gate Status

```text
SECURITY GATE STATUS

T-014: Contract Security Review
- Pinned source/header contract: NOT DONE
- COM unload lifecycle: NOT DONE
- Status: ACTIVE — BLOCKING CERTAIN WORK

T-015: Operation-Signature Gate
- Design approval: NOT DONE
- Status: BLOCKED ON T-014

T-016: Transaction Lifecycle Design
- Design approval: NOT DONE
- Status: BLOCKED ON T-014
```

### 2.3 Corrected Gate Ordering

```text
T-014: Contract/ABI + COM unload review
  ├─ T-015 → T-016 → T-004 → T-008 → T-009
  └─ T-005 → T-006 → T-007
                   └─────────── joins before T-008

T-005/T-006/T-007 may use synthetic test data only until
T-015/T-016 approve broker-operation behavior.
```

**Key insight:** Registration should **not** block isolated vault work. Synthetic local components can proceed, while real authentication behavior remains blocked.

---

## 3. WHAT IS IN PLACE (BOOTSTRAP COMPLETE)

### 3.1 Delivered Components

| Component | Description | Build Status |
|---|---|---|
| Native COM plugin skeleton | Base COM server implementation | ✅ 0 errors, 0 warnings |
| Management executable | CLI entry point | ✅ 0 errors, 0 warnings |
| Activation harness | COM activation testing | ✅ 0 errors, 0 warnings |
| Package layout | MSIX structure | ✅ 0 errors, 0 warnings |
| Locked SDK-header checks | Version verification | ✅ 0 errors, 0 warnings |

### 3.2 Documentation Complete

| Document | Location | Status |
|---|---|---|
| Microsoft sample mapping | `reports/` | ✅ Complete |
| Security report #1 | `reports/` | ✅ Complete |
| Security report #2 | `reports/` | ✅ Complete |
| Security report #3 | `reports/` | ✅ Complete |

### 3.3 Bootstrap Verification

```text
BUILD STATUS: PASS
WARNINGS: 0
ERRORS: 0
COM ACTIVATION: PASS
PACKAGE LAYOUT: PASS
SDK HEADER CHECK: PASS (locked versions)
```

### 3.4 Clarified v1 Baseline

```text
v1 baseline (tentative):
- Use only the stable, locked WebAuthn Plugin API surface.
- Experimental API symbols are compile-time prohibited unless a separate
  architecture decision record and security review explicitly approve them.
- No fallback from missing experimental APIs to guessed or unprefixed symbols.
- CONFIRMATION REQUIRED: Stable v1 APIs must provide all requirements,
  including secure broker-operation verification and required Hello interaction.
```

**Important:** Do not invent a stable-only path without confirming the stable API actually provides what the design requires.

---

## 4. SECURITY GATES BLOCKING REAL WORK

### 4.1 T-014: Contract Security Review (ACTIVE GATE)

#### Corrected Scope

```text
T-014 pins and verifies the source/header contract used to compile æDæ.
It does not independently certify undocumented broker behavior or provide
runtime C++ ABI reflection.

Runtime behavior is verified only through supported capability checks,
controlled integration tests, and supported Windows-build qualification.
```

#### Pinned Contract Acceptance Criteria

- [ ] A supported SDK version and exact header location are recorded.
- [ ] SHA-256 hashes are recorded for `webauthnplugin.h`, `pluginauthenticator.h`, and relevant IDL/header dependencies.
- [ ] A checked-in ABI manifest records:
  - required stable API symbols,
  - forbidden experimental symbols,
  - `IPluginAuthenticator` method order/count,
  - required IIDs/CLSIDs/AAGUID ownership,
  - required structure versions, sizes, and selected field offsets.
- [ ] Compile-time checks use `static_assert` and guarded preprocessor checks.
- [ ] The build fails closed when required symbols, versions, constants, sizes, offsets, or header hashes differ.
- [ ] A future SDK update requires deliberate re-pinning, security review, and an explicit compatibility decision.

#### Corrected ABI Verification Approach

| Verification type | What it does | What it does **not** do |
|---|---|---|
| Build-time source-contract verification | Header hashes, symbol token checks, method ordering, known constants, `static_assert`s | Does **not** prove Windows internal broker behavior |
| Runtime compatibility verification | DLL export/capability checks, controlled integration tests | Does **not** provide runtime C++ ABI reflection |

#### COM Unload Acceptance Criteria

- [ ] `DllCanUnloadNow` returns unloadable only when object count and in-flight operation count are both zero.
- [ ] All operation objects own lifetime through explicit strong references.
- [ ] Active-call accounting is exception-safe / RAII-managed.
- [ ] Cancellation, completion, destruction, and DLL unload are tested under race conditions.
- [ ] No operation can publish a response after its terminal state is won.

#### Corrected COM Unload Model

```text
Plugin DLL lifetime
  └─ Class factory / COM object count
       └─ Operation object (strong-ref owned)
            ├─ SignatureVerified
            ├─ Active
            ├─ CancelRequested
            └─ Terminal: Completed | Failed | Cancelled

Only one terminal transition may win.
Each active operation holds a DLL/module lifetime reference.
Cancellation authenticates to the exact active operation and requests
cancellation; it does not independently finalize or free it.
```

`DllCanUnloadNow` should report readiness only when the COM object count and active operation count are both zero. It should not invoke an imagined `NotifyUnloadComplete()` callback.

### 4.2 T-015: Operation-Signature Gate (BLOCKED ON T-014)

#### Corrected Language

The plugin should validate the **Windows-defined operation signature** and bind all later work to the exact verified operation object. The plugin header describes a broker-provided operation-signing public key and signed operation request structure.

#### Core Invariant (Verbatim)

```text
For MakeCredential, GetAssertion, CancelOperation, GetLockStatus, and
every future broker-originated operation:

1. Receive only opaque request bytes and broker metadata.
2. Verify the Windows-defined operation signature against the
   operation-signing public key obtained through the verified plugin
   registration flow and handled under the pinned header contract.
3. If verification fails, terminate without decoding request payload,
   allocating persistent state, invoking UI or Windows Hello, accessing
   the vault, changing credential metadata, or producing a protocol response.
4. Bind the verified request, operation ID, caller context, cancellation
   authority, and generation/epoch to one operation object.
5. Permit exactly one terminal completion path.
```

This is the core defense. It prevents an untrusted caller from using the plugin as a signing or UI-prompting oracle.

### 4.3 T-016: Transaction Lifecycle Design (BLOCKED ON T-014)

Required design elements:
- Transaction state machine
- Cancellation handling
- Timeout management
- Error recovery paths
- Operation object lifetime management

---

## 5. CORRECTED IMPLEMENTATION SEQUENCE (POST-APPROVAL)

### 5.1 Corrected Dependency Graph

```text
T-014 (Contract Security)
  ├─ T-015 → T-016 → T-004 → T-008 → T-009
  └─ T-005 → T-006 → T-007
                   └─────────── joins before T-008

T-005/T-006/T-007 may use synthetic test data only until
T-015/T-016 approve broker-operation behavior.
```

### 5.2 Allowed vs. Blocked Work

| May proceed after T-014 | Must remain blocked until T-015 + T-016 |
|---|---|
| SQLite schema/migrations using synthetic test records | Plugin registration with Windows |
| Vault CRUD with deliberately fake/non-key blobs | Broker request decoding |
| DPAPI wrapper tests using random test buffers | WebAuthn credential creation |
| Redacted logging framework | Assertion signing |
| WinUI shell and static/mock Identity Health screen | Windows-provided user-verification UX where supported by the pinned contract |
| COM unload test harness without WebAuthn operations | Credential metadata registration/mutation |

This preserves the central rule: **no real authentication behavior or persistent real credential state** before signed-operation and lifecycle gates are approved.

### 5.3 Full Implementation Sequence

```text
STEP 1: T-014 — Contract/ABI + COM Unload Review
- Pinned ABI manifest (build-time source contract)
- SHA-256 header hashes
- static_assert compile-time checks
- COM unload lifecycle
- Status: ACTIVE GATE

STEP 2: T-015 — Operation-Signature Gate
- Design document
- Security approval
- Status: BLOCKED ON T-014

STEP 3: T-016 — Transaction Lifecycle Design
- State machine design
- Security approval
- Status: BLOCKED ON T-014

STEP 4: T-005 — Vault Schema/CRUD (synthetic data)
- SQLite schema design
- CRUD operations (fake blobs only)
- Status: CAN PROCEED once T-014 completes

STEP 5: T-006 — DPAPI Key Protection (synthetic data)
- DPAPI wrapper tests with random buffers
- Status: CAN PROCEED once T-014 completes

STEP 6: T-007 — Windows Hello Wrapper
- Verify stable API exists in pinned header
- UV verification integration
- Status: BLOCKED until T-014 confirms stable API suitability

STEP 7: T-004 — Safe Registration Manager
- Registration lifecycle
- Status: BLOCKED ON T-015/T-016

STEP 8: T-008 — MakeCredential
- Credential creation
- Status: BLOCKED ON T-015/T-016

STEP 9: T-009 — GetAssertion
- Assertion generation
- Status: BLOCKED ON T-015/T-016

STEP 10: T-010/T-011 — Management UI
- Dashboard (static/mock first)
- Full credential management later
- Status: CAN PROCEED (static/mock)

STEP 11: T-012 — Final Security Review
- Comprehensive audit
- Status: BLOCKED

STEP 12: T-013 — Package Readiness
- MSIX finalization
- Status: IN_REVIEW (bootstrap only)
```

---

## 6. REPOSITORY HYGIENE — CRITICAL ACTION

### 6.1 Current State

```text
Baseline committed and published: `ff78cf6` on `main`
Repository: `https://github.com/mario0318/aedae`
The bootstrap and its initial T-014 remediation are intentionally co-located in that baseline; all subsequent remediation work must use focused follow-up commits.
```

### 6.2 Required `.gitignore` (Corrected)

**Important:** Do **not** blanket-ignore `.exe` or `.dll`; this can unintentionally hide fixture binaries or intentionally versioned helper artifacts.

```gitignore
# Visual Studio / build outputs
.vs/
**/bin/
**/obj/
out/
artifacts/
x64/
x86/
ARM64/

# Debug artifacts
*.pdb
*.ilk
*.tlog

# Packaging output
AppPackages/
BundleArtifacts/
TestResults/
*.msix
*.msixbundle
*.appx
*.appxbundle

# Never commit signing material or local secrets
*.pfx
*.p12
*.snk
*.key
.env
.env.*
secrets/
credentials.json
*.config.local
```

### 6.3 Staging Audit Commands

Before the initial commit:

```powershell
# Navigate to project root
cd C:\Projects\aeDae

# Initialize repository
git init

# Create .gitignore from above
# ... create file ...

# Check what would be added
git status --short

# Verify .gitignore is working
git check-ignore -v .\path\to\known-test-certificate.pfx

# Stage changes
git add .

# Check staged changes
git diff --cached --stat
git diff --cached -- . ':!*.md'
```

**Manually inspect** staged changes before committing. Confirm any Microsoft reference checkout is either excluded or deliberately included as a pinned submodule/reference artifact.

### 6.4 Initial Commit

```powershell
git commit -m "Initial baseline: bootstrap complete, security gates pending

- COM plugin skeleton
- Management executable
- Activation harness
- Package layout
- Locked SDK-header checks
- Microsoft sample mapping
- Security reports 1-3

Status: T-001, T-002, T-003, T-013 IN_REVIEW
Security gates T-014, T-015, T-016 blocking real work
All tests pass: 0 errors, 0 warnings

CRITICAL: No registration, decoding, Hello, vault access, metadata
mutation, or response before T-014/T-015/T-016 are closed.
Experimental v2 APIs are compile-time prohibited.
T-005/T-006/T-010/T-011 may use synthetic test data only."
```

---

## 7. KEY FILES & REFERENCES

### 7.1 Critical Files

| File | Location | Purpose |
|---|---|---|
| `tasks.md` | `C:\Projects\aeDae\tasks.md` | Canonical backlog |
| `contract-security-review.md` | `C:\Projects\aeDae\reports\contract-security-review.md` | Security gate T-014 |
| `PluginRegistrationManager.h` | `C:\Projects\aeDae\` | Registration manager |
| `PluginAuthenticatorImpl.cpp` | `C:\Projects\aeDae\` | Core authenticator |
| `HealthEngine.cpp` | `C:\Projects\aeDae\` | Health dashboard |
| `Package.appxmanifest` | `C:\Projects\aeDae\` | MSIX packaging |

### 7.2 Reference Implementations

| Resource | URL |
|---|---|
| Microsoft Passkey Manager Sample | [https://github.com/microsoft/Windows-classic-samples](https://github.com/microsoft/Windows-classic-samples) |
| KeePassPasskey | [https://github.com/kee-org/KeePassPasskey](https://github.com/kee-org/KeePassPasskey) |
| WebAuthn Plugin API Docs | [https://learn.microsoft.com/en-us/windows/security/identity-protection/hello-for-business/webauthn-apis](https://learn.microsoft.com/en-us/windows/security/identity-protection/hello-for-business/webauthn-apis) |
| WebAuthn Plugin Header | [https://github.com/microsoft/webauthn/blob/master/pluginauthenticator.idl](https://github.com/microsoft/webauthn/blob/master/pluginauthenticator.idl) |
| WebAuthn Standard | [https://www.w3.org/TR/webauthn-3/](https://www.w3.org/TR/webauthn-3/) |
| Windows 11 Passkey Manager Support | [https://techcommunity.microsoft.com/blog/windows-itpro-blog/windows-11-expands-passkey-manager-support/4467572](https://techcommunity.microsoft.com/blog/windows-itpro-blog/windows-11-expands-passkey-manager-support/4467572) |
| Third-Party Passkey Managers | [https://learn.microsoft.com/en-us/windows/apps/develop/security/third-party](https://learn.microsoft.com/en-us/windows/apps/develop/security/third-party) |

### 7.3 Domain Portfolio

| Domain | Price/yr | Purpose | Status |
|---|---:|---|---|
| **ædæ.tech** | $7.98 | Primary product domain | ✅ Owned |
| **ædæ.it.com** | $4.98 | IT/tech redirect | ✅ Owned |

**Total annual cost:** $12.96/yr

---

## 8. ACTION ITEMS — IMMEDIATE

### 8.1 Today's Priorities

```text
REPOSITORY
- Create .gitignore from corrected version
- git init (if needed)
- git status --short
- git check-ignore -v .\path\to\known-test-certificate.pfx
- git add .
- git diff --cached --stat
- git diff --cached -- . ':!*.md'
- MANUALLY INSPECT staged changes
- Initial commit
- Create develop branch

T-014: CONTRACT SECURITY REVIEW
- Record SDK version and exact header location
- Record SHA-256 hashes for webauthnplugin.h, pluginauthenticator.h,
  and IDL dependencies
- Create pinned ABI manifest:
  - required stable API symbols
  - forbidden experimental symbols
  - IPluginAuthenticator method order/count
  - required IIDs/CLSIDs/AAGUID
  - structure versions, sizes, offsets
- Add static_assert compile-time checks
- Ensure build fails closed on mismatch
- Implement COM unload lifecycle:
  - DllCanUnloadNow only when both counts are zero
  - Strong-ref operation objects
  - RAII active-call accounting
  - Race condition testing
- Update contract-security-review.md

DOCUMENTATION
- Update tasks.md with T-014 progress
- Update status in reports
- Tag bootstrap milestone
```

### 8.2 This Week's Priorities

```text
T-015: Operation-Signature Gate
- Design document
- Security review
- Approval

T-016: Transaction Lifecycle Design
- State machine design
- Security review
- Approval

T-005: Vault Schema/CRUD (synthetic data)
- SQLite schema
- CRUD implementation (fake blobs only)

REVIEW
- Send bundle for hostile review:
  AGENTS.md
  security.md
  architecture.md
  tasks.md
  reports/webauthnplugin-contract.md
  reports/contract-security-review.md
  pinned ABI manifest
  COM unload/lifecycle design
```

### 8.3 Review Bundle for Hostile Analysis

Send **only** the following to Claude for hostile cross-family review:

1. `AGENTS.md`
2. `security.md`
3. `architecture.md`
4. `tasks.md`
5. `reports/webauthnplugin-contract.md`
6. `reports/contract-security-review.md`
7. The pinned ABI manifest
8. The COM unload/lifecycle design

Ask:

```text
Can stable v1 APIs alone support the security invariants?
If an invariant depends on experimental v2, force a binary decision:
- Accept experimental APIs for a non-production prototype with explicit
  version/build support boundaries, OR
- Keep protocol implementation paused until Microsoft publishes a stable
  API path.

Do NOT quietly downgrade the security requirements.
```

---

## 9. FULL FINDINGS & TECHNICAL RESEARCH

### 9.1 Platform Capability Validation

Windows provides a plugin-authenticator API surface. æDæ's compatibility is limited to the pinned SDK/header contract and supported Windows builds.

Windows does provide a packaged third-party passkey-manager model, where the manager can create, manage, and use passkeys through Windows plugin integration and Windows-provided user-verification UX where supported by the pinned contract. The key question is whether the pinned API contract and security model match what the actual stable header exposes.

### 9.2 Technical Constraints

| Constraint | Impact | Mitigation |
|---|---|---|
| MSIX packaging required | Cannot use unpackaged EXE | Package as MSIX |
| Hello UV mandatory | Private-key use needs UV | Confirm stable API availability |
| No crypto invention | Must use official libraries | Use Microsoft `webauthn.h` |
| Experimental v2 uncertain | May or may not be required | Binary decision after review |
| DPAPI / TPM | Not interchangeable | DPAPI v1, TPM optional enhancement |
| Other managers opaque | Cannot access 1Password/Bitwarden vaults | Advisory/orchestration approach |

### 9.3 Architecture

```text
WINDOWS 11

Browser/App → WebAuthn Broker → æDæ Plugin → Hello UV
                                   │            │
                                   ▼            ▼
                              Encrypted Vault ← DPAPI v1
                                               TPM optional in v1
```

---

## 10. MARKET ANALYSIS & COMPETITIVE LANDSCAPE

### 10.1 Corrected Market Hypothesis

Hypothesis: people and organizations using several authenticators may benefit from a clearer local view of credential coverage, recovery gaps, and device status. This must be validated through interviews and a capability study. Cross-provider visibility remains product/API-dependent.

### 10.2 Competitive Landscape

| Competitor | Strength | Weakness |
|---|---|---|
| **1Password** | Established brand, cross-platform | Vault-centric, not device-centric |
| **Bitwarden** | Open source, affordable | No identity health engine |
| **Windows Hello** | OS-level, TPM secured | Windows-only, limited unified view |
| **æDæ (R3 Labs)** | Identity-health framing, device-centric hypothesis | New entrant, unproven |

### 10.3 The Gap to Validate

Hypotheses to test:
- People using multiple authenticators may benefit from a clearer local view.
- Organizations may need recovery-gap visibility.
- This requires interviews and capability studies, not assumption.

---

## 11. BRAND STRATEGY & DOMAIN PORTFOLIO

### 11.1 Brand Identity

| Element | Value |
|---|---|
| **Product Name** | æDæ |
| **Pronunciation** | "idea" (eye-DEE-uh) |
| **Company** | R3 Labs |
| **Tagline** | "Your Identity. Your Idea. Your Device." |
| **Company Slogan** | "Technology. Security. Identity." |

### 11.2 Domain Portfolio

| Domain | Price/yr | Purpose | Status |
|---|---:|---|---|
| **ædæ.tech** | $7.98 | Primary product domain | ✅ Owned |
| **ædæ.it.com** | $4.98 | IT/tech redirect | ✅ Owned |

### 11.3 Brand Guidelines

```text
Logo Concept:
- Primary:  æDæ (with ligature)
- ASCII:    aedae (fallback for compatibility)
- Full:     æDæ — Personal Authenticator
- Short:    æDæ

Color Palette:
- Primary:   #0a0a0f (Deep Space Black)
- Accent:    #00ff88 (Security Green)
- Secondary: #1a1a2e (Dark Navy)
- Text:      #ffffff (Pure White)

Fonts:
- Headers:   Segoe UI (bold)
- Body:      Segoe UI (regular)
- Mono:      Consolas (code, terminal)
```

---

## 12. PRODUCT ARCHITECTURE & TECHNICAL DESIGN

### 12.1 Component Design

#### Component 1: PluginRegistrationManager

**Purpose:** Register/unregister æDæ with Windows (post-approval)  
**Key APIs:** Stable WebAuthn Plugin API symbols explicitly listed in the pinned ABI manifest

#### Component 2: PluginAuthenticatorImpl

**Purpose:** Core WebAuthn protocol handling (post-approval)  
**Interfaces:** `IPluginAuthenticator` only after T-014 through T-016 are closed

#### Component 3: HealthEngine

**Purpose:** Local identity health dashboard  
**Data Sources:** æDæ vault, Windows Hello platform credentials where legitimately exposed

#### Component 4: Encrypted Vault

**Purpose:** Secure credential storage (post-approval)  
**Tech Stack:** SQLite, DPAPI v1, TPM optional enhancement

### 12.2 Corrected Key Protection Claim

```text
v1 requires DPAPI-backed encryption at rest.

TPM- or hardware-backed binding is an optional enhancement only after:
- a specific Windows-supported protection mechanism is selected,
- its security properties are documented,
- behavior is tested across supported devices,
- and a security review approves the implementation.

IMPORTANT: DPAPI and TPM binding are NOT interchangeable.
DPAPI alone does not automatically mean a credential key is TPM-bound.
```

### 12.3 Corrected "Every Key Operation" Claim

```text
æDæ requires fresh Windows Hello user verification before every
private-key use that produces a WebAuthn assertion or attestation
signature.

Viewing local metadata, opening the management UI, and reading
nonsecret health metrics do not invoke or require Windows Hello
by default.
```

Otherwise, "every key operation" could accidentally force biometric prompts for internal maintenance, metadata handling, or protected-blob housekeeping that does not use a private key.

### 12.4 Corrected Operation Object Lifecycle

```text
Plugin DLL lifetime
  └─ Class factory / COM object count
       └─ Operation object (strong-ref owned)
            ├─ SignatureVerified
            ├─ Active
            ├─ CancelRequested
            └─ Terminal: Completed | Failed | Cancelled

Only one terminal transition may win.
Each active operation holds a DLL/module lifetime reference.
Cancellation authenticates to the exact active operation and requests
cancellation; it does not independently finalize or free it.
```

### 12.5 Corrected Request-Signature Invariant

```text
For MakeCredential, GetAssertion, CancelOperation, GetLockStatus, and
every future broker-originated operation:

1. Receive only opaque request bytes and broker metadata.
2. Verify the Windows-defined operation signature against the
   operation-signing public key obtained through the verified plugin
   registration flow and handled under the pinned header contract.
3. If verification fails, terminate without decoding request payload,
   allocating persistent state, invoking UI or Windows Hello, accessing
   the vault, changing credential metadata, or producing a protocol response.
4. Bind the verified request, operation ID, caller context, cancellation
   authority, and generation/epoch to one operation object.
5. Permit exactly one terminal completion path.
```

---

## 13. MONETIZATION STRATEGY

### 13.1 Revenue Models

| Model | Target | Pricing | Notes |
|---|---|---|---|
| **B2C Subscription** | Consumers | $2.99–3.99/mo | Competitive, low margin |
| **B2B Enterprise** | Businesses | $3–6/user/mo | Higher value if security/compliance story proves out |
| **B2B2C API** | Platforms | $35–$7,500/mo | Hypothesis to validate |

### 13.2 Corrected Path

Not a product commitment until local security architecture, packaging, code signing, support model, privacy posture, and platform compatibility are proven.

- **Phase 1 (Months 1–3):** Consumer MVP — prove concept, build brand.
- **Phase 2 (Months 4–6):** Enterprise pilots — pending security architecture validation.
- **Phase 3 (Months 7–12):** B2B2C API — license health engine technology.

---

## 14. RISK ASSESSMENT & MITIGATION

### 14.1 Technical Risks

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| API changes in Windows updates | Medium | High | Feature detection, graceful degradation |
| Crypto implementation bugs | Low | Critical | Use official Microsoft libraries |
| MSIX packaging issues | Medium | Medium | Test thoroughly on multiple builds |
| Stable API insufficient for invariants | Medium | Critical | Binary decision: accept experimental or pause |

### 14.2 Market Risks

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Established players expand | High | Medium | Focus on identity health, not storage |
| Consumer adoption slower than expected | Medium | Medium | Target enterprise first |
| Regulatory changes | Medium | High | Build compliance features early |

### 14.3 Security Risks

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| Vault compromise | Low | Critical | DPAPI, TPM optional, no raw keys on disk |
| Hello bypass | Very Low | Critical | Follow platform security model exactly |
| Supply chain attack | Low | High | Code signing, secure build pipeline |
| Operation-signature forgery | Low | Critical | Windows-defined signature verification |
| COM unload race condition | Medium | Medium | Lifecycle model and race testing |

---

## 15. APPENDICES

### Appendix A: API Status (Corrected)

```markdown
Approved reference APIs
- Stable WebAuthn Plugin API symbols explicitly listed in the pinned ABI manifest.
- IPluginAuthenticator, only after T-014 through T-016 are closed.
- Windows Hello plugin user-verification API only after its exact stable
  symbol, contract, and error behavior are approved.

Explicitly forbidden pending architecture decision
- EXPERIMENTAL_WebAuthNPluginAddAuthenticator2
- EXPERIMENTAL_WebAuthNPluginPerformUserVerification2
- Any unprefixed v2 call assumed from online snippets
- Key export, private-key synchronization, or credential cloning

Critical decision needed
- Does stable v1 provide all required security invariants?
- If not, binary decision: accept experimental APIs for prototype OR pause.
```

### Appendix B: Build Commands

```powershell
# Build project
msbuild /p:Configuration=Release

# Register with Windows (DO NOT RUN UNTIL T-014/T-015/T-016 CLOSED)
# .\bin\x64\Release\æDæ.exe /register

# Test on webauthn.io (DO NOT RUN UNTIL T-014/T-015/T-016 CLOSED)
# start https://webauthn.io

# Package as MSIX
msbuild /p:Configuration=Release /p:AppxPackage=true
```

**Important:** Do not run `/register` or test on `webauthn.io` until security gates are closed.

### Appendix C: Security Checklist

```text
- Verify Windows Hello UV before every private-key use (post-approval)
- Store private keys encrypted (DPAPI v1, TPM optional)
- No raw keys on disk
- Use official webauthn.h library for crypto
- Implement CancelOperation cleanly
- Package as MSIX
- Sign binaries with code signing cert
- Handle API version differences gracefully
- Pinned ABI manifest verified (T-014)
- COM unload lifecycle approved (T-014)
- Operation-signature gate approved (T-015)
- Transaction lifecycle design approved (T-016)
- Experimental v2 APIs compile-time prohibited pending decision
- No registration before T-014/T-015/T-016 closed
- T-005/T-006/T-010/T-011 use synthetic data only
```

### Appendix D: Corrected Product Claims

| Current claim | Safer canonical wording |
|---|---|
| "Windows 11 fully supports third-party passkey plugin authenticators" | "Windows provides a plugin-authenticator API surface; æDæ compatibility is limited to the pinned SDK/header contract and supported Windows builds." |
| "Hello UV mandatory: every key operation needs UV" | "æDæ requires fresh Windows Hello verification before every private-key use that produces a WebAuthn assertion or attestation signature. Viewing local metadata, opening the management UI, and reading nonsecret health metrics do not require Windows Hello by default." |
| "Neutral integrator across providers" | "Local identity-health viewer; foreign providers remain opaque except for metadata Windows legitimately exposes." |
| "Target banks and large enterprises in months 4–6" | "Not a product commitment until local security architecture, packaging, code signing, support model, privacy posture, and platform compatibility are proven." |
| "Add credential backup/export" | "Deferred indefinitely for v1; private-key export is prohibited." |
| "B2B2C API strongest play" | "Hypothesis to be validated through interviews and capability study." |
| "DPAPI/DPAPI-NG, TPM binding" | "v1 requires DPAPI-backed encryption at rest. TPM- or hardware-backed binding is an optional enhancement only after specific Windows-supported protection is selected, documented, tested, and security-approved." |

### Appendix E: Corrected Review Bundle

```text
Send ONLY the following to Claude for hostile cross-family review:
1. AGENTS.md
2. security.md
3. architecture.md
4. tasks.md
5. reports/webauthnplugin-contract.md
6. reports/contract-security-review.md
7. the pinned ABI manifest
8. the COM unload/lifecycle design

Ask: Can stable v1 APIs alone support the security invariants?
If invariant depends on experimental v2, force binary decision:
- Accept experimental APIs for non-production prototype with explicit
  version/build support boundaries, OR
- Keep protocol implementation paused until Microsoft publishes a stable
  API path.

Do NOT quietly downgrade security requirements.
```

---

## FINAL SUMMARY

**æDæ** has completed its bootstrap phase successfully. The build is clean, COM activation works, and documentation is in place.

The critical path forward is closing the security gates:

- **T-014** — Pinned source/header contract + COM unload lifecycle
- **T-015** — Operation-signature gate approval
- **T-016** — Transaction lifecycle approval

Immediate action required:

1. Commit the corrected baseline to Git after the `.gitignore` staging audit.
2. Implement T-014 requirements.
3. Get security sign-off.
4. Send the review bundle for hostile analysis.
5. T-005/T-006/T-010/T-011 may proceed with synthetic data only.
6. Do **not** begin T-004, T-008, or T-009 real authentication behavior.

**Key message:** The project is in the right place: a clean, nonfunctional bootstrap with deliberate gates. The mistake would be treating that as permission to rush into plugin registration or protocol implementation before the authenticated-operation and lifecycle boundaries are formally approved.

---

## CONTACT & RESOURCES

**Company:** R3 Labs  
**Product:** æDæ  
**Domains:** ædæ.tech, ædæ.it.com  
**Tagline:** "Your Identity. Your Idea. Your Device."

**Canonical Backlog:** `C:\Projects\aeDae\tasks.md`  
**Security Gate:** `C:\Projects\aeDae\reports\contract-security-review.md`

---

**Document Version 1.3 (FINAL CORRECTIONS APPLIED)**  
**Generated:** August 28, 2026  
**Status:** Bootstrap Complete — Security Gates Pending (Final)  
**Next Milestone:** T-014 Contract Security Review → T-015 → T-016 → T-004

---

*R3 Labs • æDæ — Technology. Security. Identity.*
