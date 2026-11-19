# Local Windows Hello relying-party experiment

This is a **loopback-only test RP**, not the aeDae system-wide authenticator.
It cannot be selected as a provider by unrelated sites and does not access
Windows Hello keys. The plugin lane remains paused on T-019.

Requires Node 22+ and a current Chrome or Edge installation. From this folder:

```powershell
npm ci
npm test
npm run test:browser
npm start
```

Then open the exact URL printed by `npm start` (default
`http://localhost:8787`) and click **Create test passkey** and **Sign in**.
These clicks can show a real browser/Windows passkey prompt, so run them only
if you intend to create a local passkey for `localhost`. The test passkey may
remain in your Windows/browser passkey store after this server stops; the
server does not delete it. Each server restart forgets its in-memory public
credential records, so old test passkeys will no longer authenticate here.
Manage any real test passkey using your browser or Windows passkey settings.

The default `npm test` suite never calls a real authenticator. The browser
suite uses Chrome DevTools' virtual authenticator and no real Hello prompt.
Neither suite is evidence of successful real Windows Hello verification.

Only `localhost` is accepted as the request host. The listener binds to
loopback, sends no telemetry, and stores synthetic account names, public
credential keys, and counters in memory for the life of the process. It is
not a production authentication server: no account bootstrap, persistence,
rate limits, recovery, or deployment is provided.
At 128 active sessions the demo refuses new sessions until one expires; it
does not evict an active session. Its HTTP loopback cookie is intentionally
not a production HTTPS cookie policy.
