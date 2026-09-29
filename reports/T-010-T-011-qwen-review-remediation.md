# T-010/T-011 independent review remediation

Date: 2026-09-13

## Review disposition

The independent review returned `APPROVE WITH NON-BLOCKING FINDINGS`. It confirmed no prohibited
security-boundary crossing, no dependency-integrity defect and no restore-isolation defect. It
reported two MEDIUM accessibility/theme defects and two LOW refusal/test-coverage defects. All four
confirmed defects are remediated below. The human owner subsequently accepted T-010 and T-011 with
direct visual and assistive-technology inspection preserved as documented limitations.

The post-remediation rereview returned `APPROVE` with no findings remaining. That approval is pinned
to rereview packet SHA-256
`EEAA7BB0B7C8815F7FFD0DA0906D5C8BA483B10686684A61B7DFB04D68BEC82E` and the source hashes recorded
in `reports/T-010-T-011-qwen-rereview.md`.

## Remediation

### Theme and high-contrast resources

`ManagementApp.cpp` no longer creates literal authored brushes. It resolves the following resources
from the WinUI application dictionary:

- `ApplicationPageBackgroundThemeBrush`
- `CardBackgroundFillColorDefaultBrush`
- `TextFillColorPrimaryBrush`
- `TextFillColorSecondaryBrush`
- `AccentTextFillColorPrimaryBrush`
- `SystemFillColorNeutralBackgroundBrush`
- `SystemFillColorAttentionBackgroundBrush`
- `LayerFillColorDefaultBrush`

The package's native `generic.xaml` defines light, dark and high-contrast variants for these
resources. The root page subscribes to `ActualThemeChanged` and rebuilds from the same synthetic
snapshot path so newly selected resources are resolved. Runtime launch proves that every named
resource resolves on this host; it does not replace direct dark/high-contrast visual inspection.

### Accessible card grouping

Status and warning cards are now `ContentControl` instances rather than `Border` instances. Each is
non-tabbable, explicitly included in `AccessibilityView::Content`, and has one combined name. Warning
names contain the warning label, stable rule ID and all source record references. Visual child labels
are marked `AccessibilityView::Raw` so content-view navigation does not announce both the group and
each fragment. The remaining UIA-tree and screen-reader behavior must be verified with direct tools.

### Refusal-only provenance

`DecideThisPcPresentation` is a platform-independent helper shared by the console and WinUI paths.
A synthetic snapshot shows mock provenance and no refusal. A non-synthetic snapshot shows refusal
and no mock provenance. The deterministic test checks both the decision and rendered text.

`CanPresentIdentityHealthSummary` similarly permits projection only when status is `ok` and a summary
is present. The WinUI path uses this helper; live and invalid test results are explicitly refused.

### Expanded deterministic tests

`ManagementModelTests` now verifies:

- non-synthetic This PC input produces refusal with no mock provenance;
- both synthetic local-only records emit local-only warnings;
- live and invalid Identity Health results cannot be projected;
- missing locations fail closed;
- an empty provider ID fails closed;
- an empty device ID fails closed.

## Recommendation disposition

- The suggested `OnLaunched override` was attempted and rejected by MSVC C3668 because the projected
  C++/WinRT method is not a virtual base override. The specifier was removed.
- The two included model headers are now listed as `ClInclude` for IDE visibility.
- `WindowsAppSDKFrameworkPackageReference=true` is retained intentionally. The pinned Runtime 2.4.0
  native targets state that the property is required for C++ projects and use it to import framework
  and component references. The successful framework-dependent launch is consistent with that use.
- Narrow-width and 200% scaling remain direct-inspection items; no unsupported claim was added.

## Post-remediation verification

```text
Focused ManagementModelTests: PASS (24 assertions)
Focused Debug WinUI build: PASS
Debug WinUI launch/resource resolution: PASS
Full Debug wrapper: PASS, 0 build warnings, 0 build errors
Full Release wrapper: PASS, 0 build warnings, 0 build errors
Contract verifier and negative guard: PASS in both wrappers
COM activation harness: PASS in both wrappers
VaultStoreTests: PASS in both wrappers
ManagementModelTests: PASS in both wrappers
KeyProtectionTests: PASS in both wrappers
Release window title: aeDae — This PC
Release window responding: yes
Approved lock SHA-256: 82C45DE659CDF37F7164170B426DF75BBF952B93F77CBF592098EC117824724D
Release executable SHA-256: 489815B020E8032DCE1C8887427565481A5B6C02D48D944457C8D113F7AD07A6
```

The vault suite retains its documented Win32 1307 limitation when attempting to create a
foreign-owner fixture; its production foreign-owner refusal test passes.

## Remaining gate

Future validation should directly inspect light, dark and high-contrast themes, 100% and 200% scale,
narrow resizing, keyboard navigation, visible focus and the actual accessibility tree/screen-reader
announcements. The human owner accepted those items as documented limitations for this milestone.
No package registration, signing, publishing, protocol work, Windows Hello query, vault access,
credential access or key operation is authorized by this review.
