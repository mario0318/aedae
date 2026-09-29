# Repository and deployment plan

This source lives in `web/` inside the existing aeDae repository. `deploy/prepare.ps1` may create
local, gitignored preview bundles for five independent Pages projects. It has no network action.
Any Pages upload, hosting decision, DNS, redirects, mail, analytics, security-header change, or
custom-domain binding requires separate human approval.
