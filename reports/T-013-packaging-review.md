# T-013 unsigned MSIX review packet

Date: 2026-09-13

## Final disposition

APPROVED. Claude's second pass returned no findings and the human owner approved the final
post-review candidate on 2026-09-13. This closes the unsigned bootstrap scope of T-013 only. It
does not authorize signing, installation, publishing, production identity/assets, plugin
registration, protocol operations, credentials, or Windows Hello.

## Files in scope

- `build/AeDaeApp.vcxproj`
- `packaging/Package.appxmanifest`
- `packaging/README.md`
- `scripts/package.ps1`
- the T-013 entry in `tasks.md`
- `reports/T-013-packaging-blocker.md`
- this review packet

## Candidate evidence

Two complete invocations of `scripts/package.ps1` ran the full Release build and every build-wrapper
gate before packaging. Both outputs are 18,509 bytes and have this SHA-256:

```text
192871d6839a243343ab1e56e70c52d0dc94e2901f16603b03dcf0006d318fb6
```

The two exact packages passed:

- MakeAppx 10.0.26100.7705 unpack, independently, with six extracted payload/metadata files each;
- byte-for-byte package equality and equality of all seven archive-entry hashes;
- normalized 2000-01-01 00:00:00 DOS timestamps on all entries;
- exact 50x50, 150x150, and 44x44 developer PNG dimensions;
- x64 `Windows.FullTrustApplication`, `runFullTrust`, and no `internetClient` manifest checks;
- absence of `AppxSignature.p7x` and `Get-AuthenticodeSignature` status `NotSigned`;
- matching SHA-256 sidecars;
- PE header checks for Dynamic Base, NX compatibility, and Control Flow Guard; and
- compiler mitigation counts for `/GS`, `/sdl`, and CFG instrumentation.

Negative probes rejected an invalid four-part version and an output path outside
`artifacts/packages` with exit code 1 before a build or file write.

## Required reviewer questions

1. Does the ZIP64 parser reject malformed, multi-disk, out-of-bounds, or inconsistent structures
   before writing any bytes?
2. Does timestamp normalization modify only the four DOS time/date bytes in each central and local
   header while preserving MakeAppx data descriptors and the block-map-covered payloads?
3. Are the package capability, full-trust, identity, version, and output-path checks fail closed?
4. Can any script parameter or manifest capability introduce signing, secrets, network access,
   registration, or protocol behavior?
5. Is the evidence sufficient for an unsigned developer bootstrap, while the production preflight
   correctly remains separate and human-approved?

## Residual boundary

No installation test was performed because the artifact is deliberately unsigned and no
certificate creation or trust-store modification is authorized. The publisher identity and logos
remain developer placeholders. T-013 is `DONE` only within this explicitly bounded unsigned
developer-bootstrap scope.

## Claude independent review

Claude's first review found two medium issues in `scripts/package.ps1`: direct overwrite could leave
a corrupt package after a partial write, and the EOCD scan did not prove that a candidate record's
comment ended exactly at EOF. Both findings were accepted and fixed. Normalized bytes now go to a
same-directory temporary file and are installed with `File.Replace` plus an explicit recovery
backup; an EOCD candidate is accepted only when its declared comment reaches the exact file end.

The first review's ZIP64 field-order concern was rejected after source verification: the parser
advances over each optional value only when its corresponding fixed field contains the ZIP64
sentinel, so a local-offset-only field is read from the start as required. The DOS-date observation
confirmed correct behavior, and the overflow observation is already bounded before conversion.

New adversarial probes demonstrate that a fake EOCD signature inside a valid ZIP comment is ignored
and that a forced temporary-write collision leaves the original package hash unchanged. Two fresh
full Release/package runs and two MakeAppx unpack checks passed after the fixes. Claude's second
pass returned an empty findings array and stated that both medium findings are closed, with no
critical, high, or new medium issues. Raw inputs and results:

- `reports/T-013-claude-review-packet.txt`
- `reports/T-013-claude-review-first-pass.txt`
- `reports/T-013-claude-review-second-pass-packet.txt`
- `reports/T-013-claude-review-second-pass.txt`
