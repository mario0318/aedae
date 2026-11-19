# T-013 packaging blocker

Date: 2026-09-13

## Disposition

RESOLVED after explicit owner authorization for one additional repair cycle. The historical
failure evidence remains below because it defines the regression oracle. The unsigned package is
now a review candidate, not an approved, installable, signed, or publishable release.

## Resolution

The timestamp normalizer now parses MakeAppx's ZIP64 locator, ZIP64 end record, bounded central
directory, and each entry's `0x0001` extra field to find its local-header offset. It changes only
the DOS time/date fields in the original central and local headers, preserving MakeAppx data
descriptors. Two complete Release build/package runs produced byte-identical files with SHA-256:

```text
192871d6839a243343ab1e56e70c52d0dc94e2901f16603b03dcf0006d318fb6
```

MakeAppx 10.0.26100.7705 successfully unpacked both exact outputs. All seven entry hashes matched;
timestamps, developer-asset dimensions, manifest semantics, sidecars, and signature absence were
verified. Dumpbin confirmed Dynamic Base, NX compatibility, Control Flow Guard, `/GS`, and `/sdl`
on the packaged bootstrap executable. Invalid versions and out-of-scope output paths still fail
before a build or write. See `reports/T-013-packaging-review.md` for the review boundary.

Claude's independent first pass found two medium normalization defects. Both were fixed and covered
by adversarial probes; its second pass returned no findings. The human owner approved the final
candidate on 2026-09-13, closing the bounded unsigned bootstrap task.

## What passed

- Full Release builds completed repeatedly, including the contract verifier, negative contract
  guard, COM activation harness, vault adversarial/crash/substitution suite, management-model
  tests, and random synthetic key-protection tests.
- MakeAppx 10.0.26100.7705 created packages whose payloads were identical between runs.
- Manifest validation passed with x64 identity, `Windows.FullTrustApplication`,
  `runFullTrust`, no `internetClient`, and the Windows Desktop target-device dependency.
- Invalid versions and output paths outside `artifacts/packages` failed before building or writing.
- Required archive entries and exact developer-logo dimensions were verified; no
  `AppxSignature.p7x` was present.

## Reproduction and failed oracle

MakeAppx assigns the current packaging time to every ZIP entry. Two otherwise identical packages
therefore had different hashes. Rewriting the archive through .NET `ZipArchive` normalized the
timestamps and produced identical hashes, but the independent MakeAppx unpack oracle rejected that
rewrite:

```text
MakeAppx : error: 0x80511007 - The specified package format is not valid: A file descriptor is missing.
```

The safer in-place timestamp patch preserves MakeAppx data descriptors, but the installed MakeAppx
emits a ZIP64 end-of-central-directory record even for this small package. The current parser fails
closed with `Multi-disk packages are not supported.` rather than interpreting ZIP64 sentinels as
ordinary offsets.

## Required next repair

The authorized repair completed these steps:

1. parse the ZIP64 locator immediately before the ordinary end-of-central-directory record;
2. require disk numbers zero and bounded 64-bit entry-count, central-size, and central-offset values;
3. obtain any `0xffffffff` local-header offsets from the central entry's ZIP64 extra field;
4. patch only the DOS time/date fields in central and local headers;
5. require MakeAppx unpack to succeed on both independently produced packages;
6. require identical package SHA-256 values, identical per-entry hashes, normalized timestamps,
   correct image dimensions, required manifest semantics, and absence of a signature; and
7. rerun the two fail-fast negative tests and `git diff --check`.

The MakeAppx unpack oracle remains mandatory. The first timestamp normalization attempt proved
that generic ZIP readability is insufficient.
