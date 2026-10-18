import assert from 'node:assert/strict';
import { existsSync } from 'node:fs';
import { test } from 'node:test';
import { chromium } from 'playwright-core';
import { startServer } from '../server.mjs';

const browserPaths = [
  'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',
  'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',
  'C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe',
];
const executablePath = browserPaths.find(existsSync);

async function post(page, path, body = {}) {
  return page.evaluate(async ({ path, body }) => {
    const response = await fetch(path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
    return { status: response.status, body: await response.json() };
  }, { path, body });
}

async function browserResponse(page, kind) {
  const options = await post(page, `/api/${kind}/options`);
  assert.equal(options.status, 200, JSON.stringify(options.body));
  const answer = await page.evaluate(async ({ kind, optionsJSON }) => {
    return kind === 'register'
      ? window.SimpleWebAuthnBrowser.startRegistration({ optionsJSON })
      : window.SimpleWebAuthnBrowser.startAuthentication({ optionsJSON });
  }, { kind, optionsJSON: options.body });
  return answer;
}

function tamperBase64JSON(encoded, mutate) {
  const value = JSON.parse(Buffer.from(encoded, 'base64url').toString('utf8'));
  mutate(value);
  return Buffer.from(JSON.stringify(value)).toString('base64url');
}

test('virtual browser registers, authenticates, and rejects adversarial responses', async () => {
  assert.ok(executablePath, 'Chrome or Edge is required for CDP virtual WebAuthn; do not silently skip this gate');
  const { server, origin } = await startServer({ port: 0 });
  const browser = await chromium.launch({ executablePath, headless: true });
  try {
    const contextA = await browser.newContext();
    const pageA = await contextA.newPage();
    const cdp = await contextA.newCDPSession(pageA);
    await cdp.send('WebAuthn.enable');
    await cdp.send('WebAuthn.addVirtualAuthenticator', {
      options: {
        protocol: 'ctap2',
        transport: 'internal',
        hasResidentKey: true,
        hasUserVerification: true,
        isUserVerified: true,
        automaticPresenceSimulation: true,
      },
    });
    await pageA.goto(origin);
    const contextB = await browser.newContext();
    const pageB = await contextB.newPage();
    await pageB.goto(origin);

    const registration = await browserResponse(pageA, 'register');
    const bOptions = await post(pageB, '/api/register/options');
    assert.equal(bOptions.status, 200);
    const crossSession = await post(pageB, '/api/register/verify', registration);
    assert.equal(crossSession.status, 400);
    const aRegistration = await post(pageA, '/api/register/verify', registration);
    assert.deepEqual(aRegistration, { status: 200, body: { verified: true } });
    const registrationReplay = await post(pageA, '/api/register/verify', registration);
    assert.equal(registrationReplay.status, 400);

    let assertion = await browserResponse(pageA, 'authenticate');
    const wrongOrigin = structuredClone(assertion);
    wrongOrigin.response.clientDataJSON = tamperBase64JSON(assertion.response.clientDataJSON, (data) => {
      data.origin = 'http://evil.example';
    });
    assert.equal((await post(pageA, '/api/authenticate/verify', wrongOrigin)).status, 400);
    assert.equal((await post(pageA, '/api/authenticate/verify', assertion)).status, 400);

    assertion = await browserResponse(pageA, 'authenticate');
    const wrongChallenge = structuredClone(assertion);
    wrongChallenge.response.clientDataJSON = tamperBase64JSON(assertion.response.clientDataJSON, (data) => {
      data.challenge = 'wrong-challenge';
    });
    assert.equal((await post(pageA, '/api/authenticate/verify', wrongChallenge)).status, 400);

    assertion = await browserResponse(pageA, 'authenticate');
    const wrongRPID = structuredClone(assertion);
    const authenticatorData = Buffer.from(assertion.response.authenticatorData, 'base64url');
    authenticatorData.fill(0, 0, 32);
    wrongRPID.response.authenticatorData = authenticatorData.toString('base64url');
    assert.equal((await post(pageA, '/api/authenticate/verify', wrongRPID)).status, 400);

    assertion = await browserResponse(pageA, 'authenticate');
    const wrongCredential = structuredClone(assertion);
    wrongCredential.id = 'wrong-credential';
    assert.equal((await post(pageA, '/api/authenticate/verify', wrongCredential)).status, 400);

    assertion = await browserResponse(pageA, 'authenticate');
    assert.equal((await post(pageA, '/api/cancel')).status, 200);
    assert.equal((await post(pageA, '/api/authenticate/verify', assertion)).status, 400);

    assertion = await browserResponse(pageA, 'authenticate');
    assert.deepEqual(await post(pageA, '/api/authenticate/verify', assertion), {
      status: 200, body: { verified: true },
    });
    assert.equal((await post(pageA, '/api/authenticate/verify', assertion)).status, 400);
    assert.deepEqual(await post(pageA, '/api/status'), {
      status: 200, body: { registered: true, authenticated: true },
    });
    assert.deepEqual(await post(pageB, '/api/status'), {
      status: 200, body: { registered: false, authenticated: false },
    });

    const contextC = await browser.newContext();
    const pageC = await contextC.newPage();
    const cdpC = await contextC.newCDPSession(pageC);
    await cdpC.send('WebAuthn.enable');
    await cdpC.send('WebAuthn.addVirtualAuthenticator', {
      options: {
        protocol: 'ctap2', transport: 'internal', hasResidentKey: true,
        hasUserVerification: true, isUserVerified: true, automaticPresenceSimulation: true,
      },
    });
    await pageC.goto(origin);
    await pageC.getByRole('button', { name: 'Create test passkey' }).click();
    await pageC.waitForFunction(() => document.getElementById('status').textContent.includes('registered for this browser session'), null, { timeout: 5000 }).catch(async () => {
      throw new Error(`UI registration: ${await pageC.locator('#status').textContent()}`);
    });
    await pageC.getByRole('button', { name: 'Sign in' }).click();
    await pageC.waitForFunction(() => document.getElementById('status').textContent.includes('Signed in with'), null, { timeout: 5000 }).catch(async () => {
      throw new Error(`UI authentication: ${await pageC.locator('#status').textContent()}`);
    });
  } finally {
    await browser.close();
    await new Promise((resolve) => server.close(resolve));
  }
});
