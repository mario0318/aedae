# MSIX Bootstrap

This folder contains package metadata only. The production package must use the final publisher subject, approved visual assets, a verified full-trust manifest extension, and an authorized signing certificate.

`scripts/package.ps1` creates only an unsigned developer package. It fails closed when MakeAppx is missing and never creates or uses certificates.

