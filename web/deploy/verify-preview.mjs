// Verify a deployed aedae-life preview: security headers, built files, then the browser
// acceptance checks against the real origin. Read-only; it only sends GET requests.
// Usage: node deploy/verify-preview.mjs https://<id>.aedae-life.pages.dev
import {spawnSync} from 'node:child_process';
import {dirname, join} from 'node:path';
import {fileURLToPath} from 'node:url';

const base = (process.argv[2] || '').replace(/\/$/, '');
if (!/^https:\/\/[a-z0-9.-]+$/i.test(base) && !/^http:\/\/localhost:\d+$/.test(base)) {
  console.error('Usage: node deploy/verify-preview.mjs https://<preview-host>');
  process.exit(2);
}

let failures = 0;
const check = (ok, label) => { console.log(`${ok ? 'PASS' : 'FAIL'} ${label}`); if (!ok) failures += 1; };
const get = async path => {
  const res = await fetch(base + path, {redirect: 'manual'});
  return {status: res.status, csp: res.headers.get('content-security-policy') || '', headers: res.headers, body: await res.text()};
};
// Duplicate CSP headers arrive joined with ", ", so count policies by their default-src.
const policies = csp => (csp.match(/default-src/g) || []).length;

const builder = await get('/life/readme/');
check(builder.status === 200, `/life/readme/ returns 200 (got ${builder.status})`);
check(policies(builder.csp) === 1, `/life/readme/ sends exactly one CSP (got ${policies(builder.csp)})`);
for (const directive of ["script-src 'self'", "img-src 'self'", "connect-src 'none'", "form-action 'none'", "base-uri 'none'", "frame-ancestors 'none'"]) {
  check(builder.csp.includes(directive), `/life/readme/ CSP has ${directive}`);
}
check(!/\sstyle\s*=|\son[a-z]+\s*=/i.test(builder.body), 'served builder HTML has no inline style= or on*= attributes');
check(!/\baction=/i.test(builder.body.replace(/form-action/g, '')), 'served builder HTML has no action=');

const script = await get('/life/readme/app.js');
check(script.status === 200 && !/\blocation\b|\baction\s*=/i.test(script.body), 'served app.js has zero location / action= matches');

const door = await get('/life/readme/membership/');
check(door.status === 200 && policies(door.csp) === 1 && !/<script\b/i.test(door.body), 'membership page: one CSP, no script');
const tally = door.body.match(/href="(https:\/\/tally\.so\/r\/[A-Za-z0-9]+)"/)?.[1];
if (tally) {
  const form = await fetch(tally, {method: 'GET'});
  check(form.status === 200, `waitlist form ${tally} is live (got ${form.status})`);
} else {
  console.log('NOTE membership page has no waitlist link yet (config.json waitlistUrl is empty)');
}
const pixel = await get('/life/readme/done.svg');
check(pixel.status === 200 && /image\/svg\+xml/.test(pixel.headers.get('content-type') || ''), 'completion image is served as SVG');

for (const path of ['/', '/life/']) {
  const page = await get(path);
  check(policies(page.csp) === 1 && !page.csp.includes('script-src'), `${path} keeps the strict no-script CSP`);
}
for (const [name, value] of [['strict-transport-security', 'max-age='], ['x-content-type-options', 'nosniff'], ['referrer-policy', 'no-referrer']]) {
  check((builder.headers.get(name) || '').includes(value), `/life/readme/ sends ${name}`);
}

console.log(failures ? `${failures} header/file checks failed` : 'All header/file checks passed');
const browser = spawnSync(process.execPath, [join(dirname(fileURLToPath(import.meta.url)), '..', 'test-browser.mjs')],
  {stdio: 'inherit', env: {...process.env, BASE_URL: base}});
process.exit(failures || browser.status ? 1 : 0);
