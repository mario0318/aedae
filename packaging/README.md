# MSIX bootstrap package

`scripts/package.ps1` creates a deterministic, unsigned x64 developer MSIX for the bootstrap
console application. It always performs the full Release build first, selects an x64
`MakeAppx.exe` whose file version is at least `10.0.26100.7175`, validates the manifest's
full-trust capability, generates deterministic developer-only visual assets, packs with manifest
validation enabled, verifies the archive contents are unsigned, and writes a SHA-256 sidecar.

Run from the repository root:

```powershell
.\scripts\package.ps1
```

Outputs are written beneath `artifacts/packages/`. A custom output is accepted only inside that
directory. Repeating the command against unchanged inputs must produce the same package hash.
MakeAppx writes current timestamps into its ZIP64 container; the script normalizes only the DOS
time/date fields in the existing local and central headers. It deliberately preserves MakeAppx's
data-descriptor layout, and MakeAppx unpack is the required external container-validity oracle.

## Deliberate limits

- The generated geometric logos are developer assets, not final brand/store assets.
- The identity publisher is development metadata. It has not been matched to a certificate subject.
- The package contains the bootstrap status application only. It does not register the WebAuthn
  plugin, expose `IPluginAuthenticator`, install a provider, invoke Windows Hello, or perform a
  credential operation.
- The package requests `runFullTrust` because its executable uses
  `Windows.FullTrustApplication`; it does not request `internetClient`.
- The script has no signing path and accepts no certificate or key parameter.
- Installation, signing, Store submission, publishing, and production identity/asset selection
  require separate explicit human approval.

## Production preflight

Do not sign or publish until all of the following are independently reviewed and human-approved:

1. final publisher identity and certificate subject match;
2. final visual assets replace the generated developer assets;
3. the intended full-trust extension and capability set match the approved product scope;
4. Release hardening and package-content inspection pass on the exact candidate;
5. package version and upgrade/downgrade policy are approved;
6. output hash is recorded on the reviewed artifact; and
7. protocol, registration, credential, Hello, and signing gates are separately approved.
