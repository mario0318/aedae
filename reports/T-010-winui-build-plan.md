# T-010 WinUI dependency and build plan

Date: 2026-09-13

Status: exact eight-package dependency closure human-approved 2026-09-13; WinUI implementation is
in review; no package registration, signing or publishing approved

## Recommendation

Use native C++/WinRT with the reviewed component set: `Microsoft.WindowsAppSDK.WinUI` 2.3.6,
`Microsoft.WindowsAppSDK.InteractiveExperiences` 2.1.6 and `Microsoft.WindowsAppSDK.Runtime` 2.4.0,
all pinned exactly through `PackageReference` and a NuGet lock file. Do not reference the broad
`Microsoft.WindowsAppSDK` 2.4.0 meta-package and do not use an experimental package. Keep the existing
console bootstrap intact while introducing a separate management-app project; moving or retiring
the bootstrap becomes a later reviewed migration instead of an in-place project conversion.

For the release topology, use packaged, framework-dependent WinUI 3 because `architecture.md`
already selects WinUI 3 and MSIX. Framework-dependent is the Windows App SDK default and gives the
runtime a serviceable shared installation. Do not select self-contained deployment merely to make a
development launch easier; it materially increases output size and, for C++, introduces hybrid-CRT
configuration that deserves its own packaging review.

## Implemented repository topology

```text
src/App.UI/
  ManagementApp.cpp
  MockThisPcService.h
build/
  AeDaeManagementApp.vcxproj
  AeDaeManagementApp.packages.lock.json
```

The first page is programmatic WinUI rather than compiled XAML. This avoids adding another package
for generated C++/WinRT projections: the project uses the projection tool already present in the
pinned Windows SDK against the approved package metadata. It does not change the release decision
to use packaged, framework-dependent WinUI under T-013.

The existing `AeDaeApp.vcxproj` and `main.cpp` remain a diagnostic bootstrap during T-010. The new
project owns only the management UI. It consumes `IThisPcStatusService` and does not link the plugin,
vault, DPAPI implementation, WebAuthn contract, registration manager, or Windows Hello APIs.

## First-page behavior

The page contains four read-only status rows:

- Provider: `Not registered (simulated)`
- Windows Hello: `Unknown (not queried)`
- Key protection: `Unavailable (simulated)`
- Local credentials: `Unknown (not queried)`

The page must display `MOCK DATA ONLY — not a live security assessment` adjacent to the page title,
not merely in an about screen. Credential operations remain visibly disabled. Unknown must never be
rendered as zero, available, secure, registered, healthy, or protected.

The application takes an `IThisPcStatusService` snapshot and projects it into WinUI controls once. It performs
no machine capability query and has no write command. A non-synthetic snapshot must fail closed and
render an error state rather than live-looking values.

## Dependency gate

The completed review in `reports/T-010-windows-app-sdk-dependency-review.md` authenticated all 15
packages in the meta-package closure, reproduced locked restores, inspected licenses and build
targets, and rejected that broad closure for T-010. A tested component reference set reduces the
closure to eight packages while retaining WinUI, Runtime, WebView2 and required SDK build tools. The
binary packages use Microsoft software-license terms, not merely the source repositories' MIT
licenses. The human owner approved that exact narrower closure and its license/notice obligations on
2026-09-13. The approval covers a separate framework-dependent WinUI project with synthetic data;
it does not cover registration, signing, publishing, runtime installation, or security integration.

The approved dependency decision is:

1. the exact WinUI 2.3.6, InteractiveExperiences 2.1.6 and Runtime 2.4.0 direct references from
   `https://api.nuget.org/v3/index.json`, explicitly excluding the broad meta-package;
2. native C++/WinRT and framework-dependent runtime use for this synthetic UI;
3. the complete NuGet dependency graph and generated lock-file content hashes;
4. license/security review of the resolved packages;
5. unpackaged local launch for render evidence; packaged release topology remains governed by T-013.

No floating version, preview, experimental feed, package-manager install, global runtime install, or
unreviewed template-generated dependency is acceptable.

## Build and verification gates

The implementation is not complete merely because the WinUI target compiles. Required evidence is:

- clean Debug and Release x64 builds with warnings as errors;
- deterministic NuGet restore with locked mode and a clean second restore;
- existing contract, COM, vault, key-protection and management-model tests still pass;
- a focused view-model test covers unknown, known zero, simulated populated, invalid enum and
  non-synthetic refusal states;
- launched-page evidence at 100% and 200% scale;
- keyboard-only traversal, visible focus, screen-reader names, high-contrast rendering and no
  clipped status text;
- no network, vault, credential, registration, signing, Hello or live capability activity during
  the UI test;
- dependency and UX/UI review, then human milestone approval.

The unpackaged framework-dependent target creates a real window without package registration. That
closes the basic render requirement, but direct scale, keyboard, screen-reader and high-contrast
inspection remain before T-010 can be marked done.

## Known dependency interactions

T-013's current MSIX pipeline is denied and cannot be treated as packaging evidence for this UI.
T-018's hosted CI is also red on independently observed pinned-SDK header drift; adding WinUI must not
be used to bypass that contract gate. T-010 can proceed only as a separately reviewed UI/dependency
scope and must preserve the synthetic-only local-lane boundary.
