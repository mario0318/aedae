import { createServer } from 'node:http';
import { randomBytes } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import {
  generateAuthenticationOptions,
  generateRegistrationOptions,
  verifyAuthenticationResponse,
  verifyRegistrationResponse,
} from '@simplewebauthn/server';

const rpID = 'localhost';
const sessionLifetimeMs = 15 * 60_000;
const challengeLifetimeMs = 2 * 60_000;
const maximumBodyBytes = 64 * 1024;
const maximumSessions = 128;
const assets = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
  ['/style.css', ['style.css', 'text/css; charset=utf-8']],
  ['/vendor.js', ['node_modules/@simplewebauthn/browser/dist/bundle/index.umd.min.js', 'text/javascript; charset=utf-8']],
]);

function send(response, status, body, contentType = 'application/json; charset=utf-8') {
  response.writeHead(status, {
    'Content-Type': contentType,
    'Cache-Control': 'no-store',
    'X-Content-Type-Options': 'nosniff',
    'Referrer-Policy': 'no-referrer',
    'X-Frame-Options': 'DENY',
    'Content-Security-Policy': "default-src 'none'; script-src 'self'; style-src 'self'; connect-src 'self'; base-uri 'none'; form-action 'none'; frame-ancestors 'none'",
  });
  response.end(contentType.startsWith('application/json') ? JSON.stringify(body) : body);
}

function fail(response, status, message) {
  send(response, status, { error: message });
}

function getSession(request, sessions) {
  const cookie = request.headers.cookie ?? '';
  const match = /(?:^|;\s*)aedae_demo=([A-Za-z0-9_-]{43})(?:;|$)/.exec(cookie);
  if (!match) return null;
  const session = sessions.get(match[1]);
  if (!session) return null;
  if (session.expiresAt < Date.now()) {
    sessions.delete(match[1]);
    return null;
  }
  session.expiresAt = Date.now() + sessionLifetimeMs;
  return session;
}

function newSession(response, sessions) {
  for (const [id, value] of sessions) {
    if (value.expiresAt < Date.now()) sessions.delete(id);
  }
  // Fail closed at capacity: do not evict a different active demo session.
  if (sessions.size >= maximumSessions) return null;
  const id = randomBytes(32).toString('base64url');
  const session = {
    userID: randomBytes(32),
    credentials: new Map(),
    pending: null,
    authenticated: false,
    expiresAt: Date.now() + sessionLifetimeMs,
  };
  sessions.set(id, session);
  // This origin is intentionally HTTP on loopback. Do not reuse this cookie policy on a deployed site.
  response.setHeader('Set-Cookie', `aedae_demo=${id}; HttpOnly; SameSite=Strict; Path=/`);
  return session;
}

async function readJSON(request) {
  const type = request.headers['content-type'] ?? '';
  if (!/^application\/json(?:\s*;|$)/i.test(type)) throw new Error('content type');
  if (Number(request.headers['content-length']) > maximumBodyBytes) throw new Error('body too large');
  let length = 0;
  const chunks = [];
  for await (const chunk of request) {
    length += chunk.length;
    if (length > maximumBodyBytes) throw new Error('body too large');
    chunks.push(chunk);
  }
  const parsed = JSON.parse(Buffer.concat(chunks).toString('utf8'));
  if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) throw new Error('body type');
  return parsed;
}

function consumePending(session, kind) {
  const pending = session.pending;
  session.pending = null;
  if (!pending || pending.kind !== kind || pending.expiresAt < Date.now()) return null;
  return pending.challenge;
}

export async function startServer({ port = 8787 } = {}) {
  if (!Number.isInteger(port) || port < 0 || port > 65535) throw new Error('invalid port');
  const sessions = new Map();
  let origin;
  const server = createServer(async (request, response) => {
    try {
      if (request.headers.host !== new URL(origin).host) {
        fail(response, 400, 'invalid host');
        return;
      }
      const path = new URL(request.url, origin).pathname;
      if (request.method === 'GET' && assets.has(path)) {
        if (path === '/' && !getSession(request, sessions) && !newSession(response, sessions)) {
          fail(response, 503, 'demo is full');
          return;
        }
        const [asset, contentType] = assets.get(path);
        const bytes = await readFile(fileURLToPath(new URL(asset, import.meta.url)));
        send(response, 200, bytes, contentType);
        return;
      }
      if (request.method !== 'POST' || !path.startsWith('/api/')) {
        fail(response, 404, 'not found');
        return;
      }
      if (request.headers.origin !== origin) {
        fail(response, 403, 'invalid origin');
        return;
      }
      const session = getSession(request, sessions);
      if (!session) {
        fail(response, 401, 'open the demo page first');
        return;
      }
      if (path === '/api/status') {
        send(response, 200, { registered: session.credentials.size > 0, authenticated: session.authenticated });
        return;
      }
      if (path === '/api/cancel') {
        session.pending = null;
        send(response, 200, { canceled: true });
        return;
      }
      if (path === '/api/register/options') {
        const options = await generateRegistrationOptions({
          rpName: 'aeDae local experiment',
          rpID,
          userName: 'local-demo',
          userID: session.userID,
          attestationType: 'none',
          supportedAlgorithmIDs: [-7, -257],
          excludeCredentials: [...session.credentials.values()].map((item) => ({ id: item.id, transports: item.transports })),
          authenticatorSelection: {
            authenticatorAttachment: 'platform',
            residentKey: 'preferred',
            userVerification: 'required',
          },
        });
        session.pending = { kind: 'register', challenge: options.challenge, expiresAt: Date.now() + challengeLifetimeMs };
        session.authenticated = false;
        send(response, 200, options);
        return;
      }
      if (path === '/api/register/verify') {
        const challenge = consumePending(session, 'register');
        if (!challenge) {
          fail(response, 400, 'registration was not requested or has expired');
          return;
        }
        const body = await readJSON(request);
        const result = await verifyRegistrationResponse({
          response: body,
          expectedChallenge: challenge,
          expectedOrigin: origin,
          expectedRPID: rpID,
          requireUserVerification: true,
        });
        const credential = result.registrationInfo?.credential;
        if (!result.verified || !credential || session.credentials.has(credential.id)) {
          fail(response, 400, 'registration rejected');
          return;
        }
        session.credentials.set(credential.id, credential);
        send(response, 200, { verified: true });
        return;
      }
      if (path === '/api/authenticate/options') {
        if (session.credentials.size === 0) {
          fail(response, 400, 'register a demo passkey first');
          return;
        }
        const options = await generateAuthenticationOptions({
          rpID,
          allowCredentials: [...session.credentials.values()].map((item) => ({ id: item.id, transports: item.transports })),
          userVerification: 'required',
        });
        session.pending = { kind: 'authenticate', challenge: options.challenge, expiresAt: Date.now() + challengeLifetimeMs };
        session.authenticated = false;
        send(response, 200, options);
        return;
      }
      if (path === '/api/authenticate/verify') {
        const challenge = consumePending(session, 'authenticate');
        if (!challenge) {
          fail(response, 400, 'sign-in was not requested or has expired');
          return;
        }
        const body = await readJSON(request);
        const credential = typeof body.id === 'string' ? session.credentials.get(body.id) : null;
        if (!credential) {
          fail(response, 400, 'credential not registered in this session');
          return;
        }
        const result = await verifyAuthenticationResponse({
          response: body,
          expectedChallenge: challenge,
          expectedOrigin: origin,
          expectedRPID: rpID,
          credential,
          requireUserVerification: true,
        });
        if (!result.verified) {
          fail(response, 400, 'sign-in rejected');
          return;
        }
        credential.counter = result.authenticationInfo.newCounter;
        session.authenticated = true;
        send(response, 200, { verified: true });
        return;
      }
      fail(response, 404, 'not found');
    } catch {
      if (!response.headersSent) fail(response, 400, 'invalid or rejected request');
      else response.destroy();
    }
  });
  await new Promise((resolve, reject) => {
    server.once('error', reject);
    server.listen(port, 'localhost', resolve);
  });
  origin = `http://localhost:${server.address().port}`;
  return { server, origin };
}

if (process.argv[1] && fileURLToPath(import.meta.url) === process.argv[1]) {
  const { origin } = await startServer({ port: Number(process.env.AEDAE_DEMO_PORT ?? 8787) });
  process.stdout.write(`Local WebAuthn experiment: ${origin}\n`);
  process.stdout.write('Clicking Create test passkey may prompt Windows Hello. No real credential action occurs automatically.\n');
}
