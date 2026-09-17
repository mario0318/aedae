# T-010 Windows App SDK dependency review

Date: 2026-09-13

Disposition: **APPROVED 2026-09-13 — broad meta-package rejected; locked WinUI component set accepted**

## Outcome

The authorized isolated restore is complete. No package was installed or registered, no application
project or solution was changed, and no signing or publishing occurred.

`Microsoft.WindowsAppSDK` 2.4.0 is authentic and reproducible, but it is not an acceptable direct
dependency for aeDae's two-page management UI. It resolves 15 packages, including AI, ML, Search,
Widgets, DWrite and Windows AI Machine Learning components that T-010 does not use. The closure is
365,005,324 compressed bytes and 1,101,034,802 extracted bytes in the isolated cache.

A narrower framework-dependent native C++ component set was also resolved and tested:

- `Microsoft.WindowsAppSDK.WinUI` `[2.3.6]`;
- `Microsoft.WindowsAppSDK.InteractiveExperiences` `[2.1.6]`; and
- `Microsoft.WindowsAppSDK.Runtime` `[2.4.0]`.

That set resolves eight packages, restores from a fresh cache in locked mode, and completes a native
MSBuild evaluation/build. It removes the unused AI, ML, Windows AI Machine Learning, Search, Widgets,
DWrite and meta-package references. This reduces the review closure by 70,299,721 compressed bytes
and 247,380,580 extracted bytes. It remains large because WinUI, the multi-architecture Runtime,
WebView2 and Windows SDK build tools are real requirements.

The human owner accepted the narrower set and its binary license/notice obligations on 2026-09-13.
That approval is limited to the exact eight-package closure and reviewed hashes in this report.

## Reproducibility and authenticity evidence

The review fixture is under `artifacts/t010-dependency-review/`, which is ignored build evidence and
not application source. It uses a source-mapped `NuGet.Config` containing only NuGet.org and a
project-local package cache.

The broad meta-package fixture passed:

- initial native `PackageReference` restore and lock generation;
- locked restore against the generated lock;
- fresh-cache locked restore;
- native MSBuild evaluation/build; and
- NuGet audit at `low` severity with transitive auditing enabled and no advisory warning reported.

The recommended component fixture passed:

- initial native restore and lock generation;
- a deliberate version-compatibility check from `Microsoft.WindowsAppSDK.Runtime`, which rejected
  WinUI's older transitive InteractiveExperiences 2.1.3;
- correction to the Runtime-required InteractiveExperiences 2.1.6;
- fresh-cache locked restore;
- native MSBuild evaluation/build; and
- NuGet audit at `low` severity with transitive auditing enabled and no advisory warning reported.

All 15 downloaded archives passed `dotnet nuget verify --all`. Every archive had both a Microsoft
author signature and a NuGet.org repository signature. The common author certificate SHA-256 was
`566A31882BE208BE4422F7CFD66ED09F5D4524A5994F50CCC8B05EC0528C1353`; the common repository
certificate SHA-256 was `1F4B311D9ACC115C8DC8018B5A49E00FCE6DA8E2855F9F014CA6F34570BC482D`.

The broad lock SHA-256 is
`B2FF9989E80BFEAD9220ED9388060E09A5E41924BE8DB94855E7BF8D3EA27A6A`. The recommended lock
SHA-256 is `82C45DE659CDF37F7164170B426DF75BBF952B93F77CBF592098EC117824724D`. Every lock `contentHash`
matched the corresponding fresh-cache `.nupkg.metadata` value. Raw archive SHA-256/SHA-512 values,
lock content hashes and sizes are preserved in
`reports/T-010-windows-app-sdk-dependency-manifest.json`.

NuGet's lock `contentHash` for a signed package is not the same field as the raw archive SHA-512.
Both are recorded so a future review does not incorrectly compare unlike hash definitions.

## Full meta-package closure

| Package | Version | Archive bytes | Extracted bytes | Recommended set |
| --- | ---: | ---: | ---: | :---: |
| Microsoft.Web.WebView2 | 1.0.3719.77 | 8,971,244 | 55,186,449 | yes |
| Microsoft.Windows.AI.MachineLearning | 2.1.74 | 50,314,157 | 174,455,847 | no |
| Microsoft.Windows.SDK.BuildTools | 10.0.26100.4654 | 22,466,142 | 79,349,313 | yes |
| Microsoft.Windows.SDK.BuildTools.MSIX | 1.7.251221100 | 3,213,649 | 12,513,322 | yes |
| Microsoft.WindowsAppSDK | 2.4.0 | 72,204 | 462,875 | no |
| Microsoft.WindowsAppSDK.AI | 2.4.4 | 6,978,190 | 25,056,472 | no |
| Microsoft.WindowsAppSDK.Base | 2.0.4 | 90,877 | 549,089 | yes |
| Microsoft.WindowsAppSDK.DWrite | 2.1.0 | 5,296,114 | 18,436,828 | no |
| Microsoft.WindowsAppSDK.Foundation | 2.3.9 | 6,416,865 | 23,582,008 | yes |
| Microsoft.WindowsAppSDK.InteractiveExperiences | 2.1.6 | 27,892,449 | 108,885,278 | yes |
| Microsoft.WindowsAppSDK.ML | 2.1.74 | 102,737 | 529,820 | no |
| Microsoft.WindowsAppSDK.Runtime | 2.4.0 | 164,053,946 | 329,142,755 | yes |
| Microsoft.WindowsAppSDK.Search | 2.4.4 | 4,777,703 | 15,432,711 | no |
| Microsoft.WindowsAppSDK.Widgets | 2.0.5 | 2,758,616 | 13,006,027 | no |
| Microsoft.WindowsAppSDK.WinUI | 2.3.6 | 61,600,431 | 244,446,008 | yes |

Recommended closure totals: eight packages, 294,705,603 compressed bytes and 853,654,222 extracted
bytes.

## Build-time behavior

The closure is not passive header material. It imports MSBuild props/targets transitively. Notable
reviewed behavior includes:

- WinUI XAML markup compiler tasks and conditional compiler executable invocations;
- Windows App SDK inline Roslyn tasks for dependency validation and manifest generation;
- framework-package and self-contained deployment selection;
- runtime component-version enforcement;
- MSIX/PRI packaging targets and Windows SDK build tools; and
- WebView2 build integration.

The inspection found no package target that downloads dependencies during the build. Restore remains
the only network-dependent step in this review. Any eventual project must use locked restore and
must not disable Runtime's component-version validation.

## License and notice obligations

These are Microsoft binary packages under Microsoft software license terms, not merely the MIT
license used by portions of their source repositories. The Windows App SDK terms require license
acceptance, restrict the development/test grant to Windows, define redistribution conditions for
binplaced files, and include warranty, damages and arbitration terms. Packages also contain large
third-party notice files that must be preserved as required.

WebView2 carries its own license and notice files. Windows SDK BuildTools and BuildTools.MSIX carry
Windows SDK license terms. The AI/ML packages add separate third-party-material obligations; their
removal is one reason to reject the broad meta-package.

NuGet audit produced no known-vulnerability warning at review time. That is time-limited advisory
evidence, not a guarantee that the packages are vulnerability-free; CI must repeat locked restore
and audit when the dependency is adopted.

## Approval boundary and next implementation gate

Human acceptance is explicitly limited to the eight-package recommended closure and the
lock/manifest hashes above. Direct use of `Microsoft.WindowsAppSDK` 2.4.0 remains rejected.

After acceptance, T-010 may create the separate WinUI application project with these constraints:

1. exact direct versions and committed lock file;
2. framework-dependent mode (`WindowsAppSDKSelfContained=false`);
3. no AI, ML, Search, Widgets or DWrite package references;
4. no plugin, vault, DPAPI, WebAuthn or Windows Hello integration in the UI task;
5. synthetic management data only until the later integration gate;
6. package licenses/notices retained in release documentation; and
7. a real rendered UI check before claiming the WinUI milestone complete.

No dependency has been added to aeDae production code by this review.
