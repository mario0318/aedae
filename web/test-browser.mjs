// Browser acceptance checks for the Life README builder. Optional: needs Playwright and a
// prior `node build.mjs`. Skips cleanly when Playwright is not installed.
import {createServer} from 'node:http';
import {readFile} from 'node:fs/promises';
import {dirname, extname, join, normalize} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createRequire} from 'node:module';

let chromium;
try { ({chromium} = createRequire(import.meta.url)('playwright')); } catch {
  // CI sets REQUIRE_BROWSER=1 so a missing browser fails loudly instead of skipping.
  console.log(`${process.env.REQUIRE_BROWSER ? 'FAIL' : 'SKIP'} browser checks: playwright is not installed`);
  process.exit(process.env.REQUIRE_BROWSER ? 1 : 0);
}

const dist = join(dirname(fileURLToPath(import.meta.url)), 'dist');
const types = {'.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.jpg': 'image/jpeg'};
const server = createServer(async (req, res) => {
  let path = normalize(decodeURIComponent(new URL(req.url, 'http://x').pathname));
  if (path.endsWith('/')) path += 'index.html';
  try {
    const body = await readFile(join(dist, path));
    res.writeHead(200, {'content-type': types[extname(path)] || 'application/octet-stream'}).end(body);
  } catch { res.writeHead(404).end(); }
}).listen(0);
// BASE_URL runs the same checks against a deployed preview, e.g. https://<id>.aedae-life.pages.dev
const base = process.env.BASE_URL ? `${process.env.BASE_URL.replace(/\/$/, '')}/life/readme/` : `http://localhost:${server.address().port}/life/readme/`;
const origin = new URL(base).origin;

let passed = 0;
const check = (ok, label) => { if (!ok) throw new Error(`FAIL ${label}`); passed += 1; console.log(`PASS ${label}`); };

const browser = await chromium.launch();
try {
  const context = await browser.newContext({acceptDownloads: true, viewport: {width: 390, height: 844}});
  await context.grantPermissions(['clipboard-read', 'clipboard-write'], {origin});
  const fetched = [];
  const outside = [], errors = [];
  context.on('request', r => { const u = r.url(); fetched.push(u); if (!u.startsWith(origin) && !u.startsWith('blob:')) outside.push(u); });
  const open = async () => {
    const page = await context.newPage();
    page.on('pageerror', e => errors.push(e.message));
    page.on('console', m => { if (m.type() === 'error') errors.push(m.text()); });
    await page.goto(base);
    return page;
  };
  const nameOf = page => page.$eval('#name', el => el.value);

  // 1. Session-only by default.
  let page = await open();
  check(!(await page.isChecked('#keep')), 'keep box is unchecked by default');
  await page.fill('#name', 'Tab Only');
  await page.reload();
  check(await nameOf(page) === 'Tab Only', 'draft survives a refresh in the same tab');
  await page.close();
  page = await open();
  check(await nameOf(page) === '', 'draft is gone after closing the tab and reopening');

  // Opt in: draft persists across tabs, indicator shows.
  await page.check('#keep');
  await page.fill('#name', 'Kept Draft');
  check((await page.textContent('#savenote')).startsWith('Saved on this device'), 'saved-on-device indicator shows when kept');
  await page.close();
  page = await open();
  check(await nameOf(page) === 'Kept Draft' && await page.isChecked('#keep'), 'kept draft is present after reopening');

  // Opt out: next load is empty.
  await page.uncheck('#keep');
  await page.close();
  page = await open();
  check(await nameOf(page) === '', 'unchecking wipes the kept draft on next load');

  // 5. Essentials path: three sections, then a downloadable file.
  await page.fill('#name', 'Essential Person');
  await page.click('#essentials');
  check((await page.$$('#tabs button')).length === 3, 'essentials shows exactly three sections');
  await page.fill('#call_first', 'Ana');
  await page.click('#next');
  await page.fill('#executor', 'Ben');
  await page.click('#next');
  await page.fill('#conditions', 'None');
  check(!fetched.some(u => u.endsWith('/done.svg')), 'completion image is not fetched before download');
  const [download] = await Promise.all([page.waitForEvent('download'), page.click('#dl')]);
  const text = await readFile(await download.path(), 'utf8');
  check(/## First 24 hours[\s\S]*## People[\s\S]*## Health & care/.test(text) && !/## Basics|## Money map/.test(text),
    'essentials download contains the three sections and nothing else');
  check((await page.textContent('#lives')) === '3 of 3 essentials written', 'progress counts essentials');
  // Poll with evaluate(): waitForFunction uses string eval in some Playwright versions, which our CSP blocks.
  for (let i = 0; i < 50 && !(await page.evaluate(() => document.querySelector('.donemark').complete)); i++) await page.waitForTimeout(100);
  check(fetched.filter(u => u.endsWith('/done.svg')).length >= 1, 'download fetches the completion image');
  check(await page.isVisible('#member'), 'fake-door button appears after download');

  // Tell my people: personal message, copied, never sent.
  await page.click('#tell');
  const msg = await page.inputValue('#msg-executor');
  check(msg.startsWith('Hi Ben,') && msg.includes('the person in charge') && msg.endsWith('Essential Person'), 'invite is personal');
  await page.click('[data-copy="msg-executor"]');
  check(await page.evaluate(() => navigator.clipboard.readText()) === msg, 'copy button puts the invite on the clipboard');
  await page.evaluate(() => { navigator.clipboard.writeText = () => Promise.reject(new Error('denied')); document.execCommand = () => false; });
  await page.click('[data-copy="msg-executor"]');
  const selected = await page.evaluate(() => { const el = document.querySelector('#msg-executor'); return document.activeElement === el && el.selectionEnd - el.selectionStart === el.value.length; });
  check(selected && /Selected/.test(await page.textContent('[data-copy="msg-executor"]')), 'when clipboard is blocked, the message is selected for manual copy');

  // Fake door: honest page, no script.
  const door = await context.newPage();
  await door.goto(base + 'membership/');
  check((await door.textContent('h1')) === "Membership isn't open yet", 'membership page says it is not open yet');
  await door.close();

  // Download clears the tab draft unless kept.
  await page.reload();
  check(await nameOf(page) === '', 'tab draft is cleared after download');

  // 4. Secret warning shows its limitation line.
  await page.click('#full');
  await page.click('[data-i="1"]');
  await page.fill('#keys', 'pin: 1234');
  const warning = await page.textContent('#secret');
  check(await page.isVisible('#secret') && warning.includes("he's a cat, not a scanner"), 'secret warning shows the limitation line');

  // 2. No outside traffic, no CSP violations.
  check(outside.length === 0, 'no requests leave the page');
  check(errors.length === 0, `no console or CSP errors (${errors.join(' | ')})`);
} finally {
  await browser.close();
  server.close();
}
console.log(`${passed} browser acceptance checks passed`);
