# T-010 WinUI implementation evidence

Date: 2026-09-13

## Disposition

`DONE`. The approved synthetic-only management page is implemented as a real C++ WinUI window,
builds in the full solution, and launches against the installed Windows App Runtime. Independent
rereview returned `APPROVE`, and the human owner accepted the milestone on 2026-09-13. Direct visual
layout, Windows high-contrast behavior, keyboard traversal and screen-reader output remain
documented, unperformed limitations and are not claimed as verified.

## Implemented boundary

- `build/AeDaeManagementApp.vcxproj` owns the separate framework-dependent, unpackaged UI target.
- `build/AeDaeManagementApp.packages.lock.json` contains the exact approved eight-package closure.
- `src/App.UI/ManagementApp.cpp` constructs the page programmatically with WinUI controls.
- `src/App.UI/MockThisPcService.h` is the only status source used by the page.
- `AeDae.sln` builds the management target in Debug and Release x64 configurations.

The page presents provider, Windows Hello, key-protection and local-credential states. It visibly
labels the snapshot as mock data, distinguishes unknown from a known zero, sets automation names,
wraps content in a vertical scroller, and leaves credential management disabled. A non-synthetic
snapshot is refused rather than rendered as trusted status.

After independent review, all authored colors resolve through WinUI theme resources and the page is
rebuilt on `ActualThemeChanged`. Status and warning cards are non-tabbable `ContentControl` elements
explicitly placed in the content accessibility view with a combined accessible name; their visual
child text is kept out of the content view to avoid duplicate announcements. Mock provenance is now
rendered only for a synthetic snapshot, while a non-synthetic snapshot renders refusal alone.

This work does not query Windows Hello or registration state, touch the vault, protect or retrieve a
key, enumerate credentials, call the plugin, register a package, sign an artifact, or publish it.

## Dependency and restore evidence

The management project explicitly scopes both its lock and NuGet-generated restore state. This
prevents its `project.assets.json` from being consumed by unrelated native projects in the shared
`build/` directory.

```text
Locked restore: PASS
Shared build/obj/project.assets.json after restore: absent
Isolated obj/AeDaeManagementApp/project.assets.json: present
Lock SHA-256: 82C45DE659CDF37F7164170B426DF75BBF952B93F77CBF592098EC117824724D
```

The exact generated files from the initial shared-directory collision were preserved at
`G:\aeDae-build-obj-collision-backup`; no source or user data was deleted.

## Verification evidence

```powershell
scripts/build.ps1 -Configuration Debug
scripts/build.ps1 -Configuration Release
```

Both commands passed on 2026-09-13. Each run included the protected contract verification and
negative guard, a complete solution build, COM activation harness, VaultStoreTests,
ManagementModelTests and KeyProtectionTests. Both solution builds reported zero warnings and zero
errors. The vault suite retained its previously documented Win32 1307 limitation for constructing a
foreign-owner fixture; the production validator's foreign-owner refusal test still passed.

The Release binary was then started directly:

```text
Process ID: 34852
Exited during observation: no
Main window handle: 3082730
Main window title: aeDae — This PC
Responding: yes
Release executable SHA-256: 489815B020E8032DCE1C8887427565481A5B6C02D48D944457C8D113F7AD07A6
```

This latest launch is the combined T-010/T-011 executable after adding the synthetic Identity
Health projection; the T-010 status area and its refusal boundary are unchanged.

This proves real window creation and responsiveness, not pixel correctness or assistive-technology
behavior. Those direct inspections remain explicit review items rather than inferred claims.

## Independent review remediation

The 2026-09-13 independent review disposition was `APPROVE WITH NON-BLOCKING FINDINGS`. Its four
confirmed findings covered theme/high-contrast integration, accessible card grouping, contradictory
mock provenance on the refusal path, and missing deterministic edge/refusal tests. All four were
remediated in code and tests. Both full wrappers passed again afterward. Direct dark-theme,
high-contrast, 200% scale, narrow-width and screen-reader inspection remains necessary because a
runtime resource lookup and an authored automation name do not prove the resulting pixels or UIA
tree. See `reports/T-010-T-011-qwen-review-remediation.md`.

The subsequent post-remediation rereview returned `APPROVE` with no confirmed findings remaining.
See `reports/T-010-T-011-qwen-rereview.md` for the exact reviewed packet and source hashes.

The human owner accepted the final T-010/T-011 milestone on 2026-09-13 with the remaining direct
inspection items preserved as limitations. No broader security, packaging or publishing authority
was granted.
