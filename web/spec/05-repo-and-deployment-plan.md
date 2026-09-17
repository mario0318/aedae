# Repository and deployment plan

This source lives in `web/` inside the existing aeDae repository. `web/deploy/prepare.ps1` builds five self-contained preview bundles: each bundle has the selected surface at its root and retains every root-relative internal route. Preview deployment uses five separate Cloudflare Pages projects and temporary `*.pages.dev` URLs. `web/deploy/deploy.ps1` requires the human operator to authenticate Wrangler and never configures custom domains or DNS. Domain binding remains a separate approved step.
