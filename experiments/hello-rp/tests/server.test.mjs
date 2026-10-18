import assert from 'node:assert/strict';
import { request as httpRequest } from 'node:http';
import { after, before, test } from 'node:test';
import { startServer } from '../server.mjs';

let server;
let origin;

before(async () => {
  ({ server, origin } = await startServer({ port: 0 }));
});

after(async () => {
  await new Promise((resolve) => server.close(resolve));
});

async function session() {
  const page = await fetch(origin);
  assert.equal(page.status, 200);
  const cookie = page.headers.get('set-cookie')?.split(';')[0];
  assert.match(cookie, /^aedae_demo=[A-Za-z0-9_-]{43}$/);
  return cookie;
}

async function post(path, cookie, body = {}, extraHeaders = {}) {
  return fetch(`${origin}${path}`, {
    method: 'POST',
    headers: { Origin: origin, Cookie: cookie, 'Content-Type': 'application/json', ...extraHeaders },
    body: JSON.stringify(body),
  });
}

test('loopback page has a strict session cookie and no-store headers', async () => {
  const page = await fetch(origin);
  assert.equal(page.status, 200);
  assert.match(page.headers.get('set-cookie'), /HttpOnly; SameSite=Strict/);
  assert.equal(page.headers.get('cache-control'), 'no-store');
  assert.match(page.headers.get('content-security-policy'), /default-src 'none'/);
  assert.match(await page.text(), /NOT THE WINDOWS PLUGIN/);
});

test('host, origin, and session are required', async () => {
  const cookie = await session();
  const badHostStatus = await new Promise((resolve, reject) => {
    const request = httpRequest(`${origin}/api/status`, {
      method: 'POST', headers: { Host: 'other.localhost', Origin: origin, Cookie: cookie },
    }, (response) => {
      response.resume();
      response.on('end', () => resolve(response.statusCode));
    });
    request.on('error', reject);
    request.end();
  });
  assert.equal(badHostStatus, 400);
  const badOrigin = await post('/api/status', cookie, {}, { Origin: 'http://evil.example' });
  assert.equal(badOrigin.status, 403);
  const noSession = await post('/api/status', 'aedae_demo=missing');
  assert.equal(noSession.status, 401);
});

test('challenges are one-use, session-bound, and cancelable', async () => {
  const alice = await session();
  const bob = await session();
  const aliceOptions = await post('/api/register/options', alice);
  const bobOptions = await post('/api/register/options', bob);
  assert.equal(aliceOptions.status, 200);
  assert.equal(bobOptions.status, 200);
  const a = await aliceOptions.json();
  const b = await bobOptions.json();
  assert.notEqual(a.challenge, b.challenge);
  assert.equal(a.rp.id, 'localhost');
  assert.equal(a.authenticatorSelection.userVerification, 'required');
  const rejected = await post('/api/register/verify', alice, { id: 'not-a-credential' });
  assert.equal(rejected.status, 400);
  const replay = await post('/api/register/verify', alice, { id: 'not-a-credential' });
  assert.equal(replay.status, 400);
  assert.match((await replay.json()).error, /not requested/);
  const cancel = await post('/api/cancel', bob);
  assert.equal(cancel.status, 200);
  const afterCancel = await post('/api/register/verify', bob, { id: 'not-a-credential' });
  assert.equal(afterCancel.status, 400);
  assert.match((await afterCancel.json()).error, /not requested/);
});

test('body and content type are bounded', async () => {
  const cookie = await session();
  await post('/api/register/options', cookie);
  const tooLarge = await post('/api/register/verify', cookie, { payload: 'x'.repeat(70_000) });
  assert.equal(tooLarge.status, 400);
  await post('/api/register/options', cookie);
  const wrongType = await post('/api/register/verify', cookie, {}, { 'Content-Type': 'text/plain' });
  assert.equal(wrongType.status, 400);
});
