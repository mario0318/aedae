# T-010 mock status preparation

Date: 2026-09-10. Status: partial implementation, not a completed WinUI page.

Implemented a header-only IThisPcStatusService, a mock provider and a mock-only text presenter.
The existing application supports --mock-status without calling the bootstrap's plugin availability
query. Its default bootstrap behavior is preserved. There is no connection to the vault, credential
store, Windows Hello, key protection, registration, signing, or live capability probing on this branch.

The preview labels every value as mock/simulated or unknown. A missing credential count is distinct
from a known zero. Non-synthetic input is refused by this presenter. This is an application guard,
not a security boundary against a caller that falsely labels data synthetic.

Files: src/App.UI/MockThisPcService.h, src/App.UI/main.cpp,
tests/unit/ManagementModelTests.cpp, build/ManagementModelTests.vcxproj,
the solution project entry and scripts/build.ps1's test invocation.

Tests cover default disabled/not-registered state, unknown capability/count, explicit simulated
provenance, zero vs unknown, populated mock scenarios, non-synthetic input refusal and unknown enum
fallback. The mock preview and original bootstrap have both been exercised directly.

The repository has no WinUI project, XAML files, package reference or lock file. Local .NET SDKs
8.0.425 and 9.0.316 are installed; this does not establish that a WinUI build/runtime is available.
No Windows App SDK dependency was fetched, added or installed. The next UI step requires a reviewed,
pinned WinUI dependency/build plan and the actual page, then render/accessibility verification.
Do not describe this console preview as the WinUI acceptance criterion being satisfied.

Verification: Debug and Release wrapper runs returned 0, including contract verification,
negative guards, full compilation, COM harness, ordinary vault suite and eight management-model
assertions (artifacts/t010-debug.log and artifacts/t010-release.log). Build-gate rejection fixtures
also passed. The mock preview shows unknown Hello/count rather than invented machine facts;
the no-argument bootstrap still runs. git diff --check passed with existing line-ending warnings.
The separate security substitution regression fails; this UI preparation does not resolve T-005.
