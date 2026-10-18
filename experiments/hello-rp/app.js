const { startRegistration, startAuthentication } = window.SimpleWebAuthnBrowser;
const status = document.getElementById('status');
const registerButton = document.getElementById('register');
const authenticateButton = document.getElementById('authenticate');

async function post(path, body) {
  const response = await fetch(path, {
    method: 'POST',
    credentials: 'same-origin',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body ?? {}),
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error ?? 'Request rejected');
  return result;
}

async function run(kind) {
  registerButton.disabled = true;
  authenticateButton.disabled = true;
  status.textContent = kind === 'register' ? 'Waiting for the passkey prompt…' : 'Waiting for sign-in…';
  try {
    const optionsJSON = await post(`/api/${kind}/options`);
    const answer = kind === 'register'
      ? await startRegistration({ optionsJSON })
      : await startAuthentication({ optionsJSON });
    const result = await post(`/api/${kind}/verify`, answer);
    if (!result.verified) throw new Error('Verification failed');
    status.textContent = kind === 'register'
      ? 'Test passkey registered for this browser session. Now click Sign in.'
      : 'Signed in with the registered test passkey.';
  } catch (error) {
    await post('/api/cancel').catch(() => {});
    status.textContent = error.name === 'NotAllowedError'
      ? 'Passkey prompt canceled or timed out. No sign-in was completed.'
      : `Operation failed: ${error.message}`;
  } finally {
    registerButton.disabled = false;
    authenticateButton.disabled = false;
  }
}

registerButton.addEventListener('click', () => run('register'));
authenticateButton.addEventListener('click', () => run('authenticate'));
