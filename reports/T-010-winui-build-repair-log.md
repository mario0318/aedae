# T-010 WinUI build repair log

Date: 2026-09-13

The first native WinUI build exposed six deterministic integration issues while preserving the
approved eight-package closure:

1. Direct-project builds had no `SolutionDir`, so output, cache and lock paths resolved under
   `build/`; paths now derive from normalized `MSBuildProjectDirectory\..`.
2. A trailing backslash escaped the projection generator's closing quote; the generated directory
   is now passed without a trailing separator.
3. WinUI's WebView2 projection must be an input rather than only a metadata reference; the pinned
   WebView2 winmd is now projected by the Windows SDK compiler.
4. Authored C++ needed the collections projections, a non-final implementation type, a non-shadowing
   brush helper name, and the standard Windows Runtime `runtimeobject.lib` import library.
5. A repository-root lock file was visible to unrelated solution projects. The reviewed lock now
   lives beside `AeDaeManagementApp.vcxproj`, and that project names it explicitly.
6. Native projects under `build/` share `build/obj` by default. The first management restore placed
   `project.assets.json` there, causing NuGet to inject WinUI assets into six projects with no package
   references. `BaseIntermediateOutputPath` and `MSBuildProjectExtensionsPath` now isolate restore
   state under `obj/AeDaeManagementApp/`; the generated collision files were moved to
   `G:\aeDae-build-obj-collision-backup` before the clean verification run.

No package version, lock entry, protected contract file, security boundary, registration state or
machine installation changed during these repairs.
