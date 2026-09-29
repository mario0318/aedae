# Qwen independent review handoff: aeDae T-010 and T-011

Date: 2026-09-13

## Review request

Act as an independent, adversarial reviewer. Review the attached aeDae synthetic WinUI management
work for correctness, security-boundary preservation, dependency integrity, native build topology,
and accessibility/UX risks. Do not modify files. Report findings with severity, affected file and
line, exploit or failure condition, smallest required remediation, and a verification step. Separate
confirmed defects from recommendations and questions. If there are no findings in a category, say
so explicitly.

This is a security-sensitive Windows authenticator project. Apply the source-of-truth order from
`AGENTS.md`: `security.md`, `functional.md`, `architecture.md`, `tasks.md`, then code and tests. Stop
and report any specification conflict rather than inventing a resolution.

## Human-approved scope

The human owner approved this exact framework-dependent NuGet closure for a synthetic-only WinUI
application:

- Microsoft.WindowsAppSDK.WinUI 2.3.6
- Microsoft.WindowsAppSDK.InteractiveExperiences 2.1.6
- Microsoft.WindowsAppSDK.Runtime 2.4.0
- transitive Microsoft.Web.WebView2 1.0.3719.77
- transitive Microsoft.Windows.SDK.BuildTools 10.0.26100.4654
- transitive Microsoft.Windows.SDK.BuildTools.MSIX 1.7.251221100
- transitive Microsoft.WindowsAppSDK.Base 2.0.4
- transitive Microsoft.WindowsAppSDK.Foundation 2.3.9

The approved lock SHA-256 is
`82C45DE659CDF37F7164170B426DF75BBF952B93F77CBF592098EC117824724D`.

The approval does not authorize package registration, runtime installation, signing, publishing,
WebAuthn protocol calls, plugin integration, Windows Hello queries, vault access, credential access,
key operations, or live provider/identity metadata.

## Implemented work

T-010 adds a separate framework-dependent, unpackaged C++ WinUI target. Its first section consumes
`MockThisPcStatusService` and displays provider, Windows Hello, key protection and local credential
states. Every state is simulated or explicitly not queried. Credential actions are disabled, a
non-synthetic snapshot is refused, and the page contains automation names and a vertical scroller.

T-011 projects `IdentityHealthModel.h` into the same window using three synthetic records. The model
computes credential/provider/device totals and local-only, single-known-authenticator and possible
duplicate warnings. Every visible warning includes its stable rule ID and exact synthetic source
record references. The incomplete third-party visibility statement is visible. Non-OK model results
render refusal rather than partial data.

The implementation uses programmatic WinUI and the `cppwinrt.exe` already present in the pinned
Windows SDK. No ninth NuGet package was added.

## Build-topology incident to verify

The initial management restore wrote `project.assets.json` to the shared `build/obj` directory,
causing six unrelated native projects to enter `ResolveNuGetPackageAssets` and fail with `Sequence
contains no elements`. The repair:

1. moved the lock beside the management project and set `NuGetLockFilePath` explicitly;
2. set `BaseIntermediateOutputPath` and `MSBuildProjectExtensionsPath` before the C++ default-props
   import so generated NuGet state lives under `obj/AeDaeManagementApp/`;
3. preserved the generated collision files outside the repository at
   `G:\aeDae-build-obj-collision-backup`;
4. confirmed `build/obj/project.assets.json` is absent and the isolated asset file exists.

Review whether those MSBuild properties are early and complete enough, whether any configuration or
platform can still collide, and whether restore remains deterministic without affecting projects
that have no package references.

## Reported verification

After the combined T-010/T-011 source change:

- focused Debug management build: passed;
- `scripts/build.ps1 -Configuration Debug`: passed;
- `scripts/build.ps1 -Configuration Release`: passed;
- both full solution builds: zero warnings and zero errors;
- protected contract verification and negative guard: passed in both wrappers;
- COM activation harness: passed;
- VaultStoreTests, including crash, concurrency and temp-substitution coverage: passed;
- ManagementModelTests: passed;
- KeyProtectionTests against random synthetic buffers: passed;
- Release executable launched into a responsive window titled `aeDae — This PC`;
- latest Release executable SHA-256:
  `C7D72650C2A7F1F09CDDC83D885AD9374DEF89F4BB28D1245C42EB0657D2BC6A`.

The vault suite reports its already documented Win32 1307 limitation when constructing a
foreign-owner fixture; the production foreign-owner refusal test passes.

## Known unverified boundary

Window creation and responsiveness were observed, but pixel layout, 200% scaling, high-contrast
behavior, keyboard traversal, visible focus and screen-reader output were not directly inspected.
The code uses hard-coded brushes, so high-contrast and dark-theme behavior deserve particular
scrutiny. T-010 and T-011 remain `IN_REVIEW`, not `DONE`.

## Required review questions

1. Does any UI or model path imply live, complete, secure, registered, protected or healthy status
   that is not supported by its synthetic source?
2. Can a non-synthetic or invalid Identity Health input produce any totals or warnings?
3. Does every displayed warning visibly and accessibly identify both its stable rule and source
   records, as required by T-011?
4. Are the exact direct package versions and lock enforced, with no broad Windows App SDK
   meta-package or floating dependency?
5. Is NuGet restore state isolated from every other project and configuration?
6. Does the programmatic WinUI lifetime or metadata-provider implementation contain a crash,
   activation, ownership or shutdown defect?
7. Could the scroll/layout choices clip or hide essential information at narrow widths or 200%
   scaling?
8. Are automation properties likely to surface meaningful names, or are non-control `Border`
   elements likely to be absent from the useful accessibility tree?
9. Do fixed colors create unreadable dark-theme or high-contrast states? Recommend a concrete WinUI
   resource-based remedy if confirmed.
10. Do the reports and `tasks.md` claim more verification or completion than the evidence supports?
11. Did the implementation cross any prohibited protocol, credential, Hello, vault, key,
    registration, signing or publishing boundary?
12. Identify any missing deterministic test that can be added without introducing live credential or
    operating-system security state.

## Expected response format

Begin with an overall disposition: `APPROVE`, `APPROVE WITH NON-BLOCKING FINDINGS`, or `REJECT`.
List findings from highest severity to lowest. For each finding provide:

- severity and short title;
- file and exact line or symbol;
- evidence and failure condition;
- why it matters;
- required remediation;
- exact verification step.

End with separate sections for verified strengths, unverified claims, and the minimum gate to move
T-010/T-011 from `IN_REVIEW` to `DONE`.
