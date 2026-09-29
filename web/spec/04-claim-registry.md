# Claim registry

Permitted statuses: `verified`, `specified`, `research`, `horizon`, and `practice`. `verified` is
current repository fact. `specified` is a requirement, not an implementation. `practice` is
vendor-neutral guidance, explicitly not an aeDae product claim. Every rendered status chip requires
a source. `built` is refused by the build.

The former T-002 header discrepancy is closed: `reports/sample-mapping.md` and
`reports/webauthnplugin-contract.md` agree that the locked local SDK exposes
`webauthnplugin.h`. Public pages may describe that verified fact, but must not infer unsupported
protocol behavior from header presence alone.
