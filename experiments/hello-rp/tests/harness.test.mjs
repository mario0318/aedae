import assert from 'node:assert/strict';
import { test } from 'node:test';
import { startServer } from '../server.mjs';
import { withServerAndBrowser } from './harness.mjs';

test('server is closed when the browser launch rejects', async () => {
  let started;
  const start = async (options) => (started = await startServer(options));
  let ran = false;
  await assert.rejects(
    withServerAndBrowser(async () => {
      assert.equal(started.server.listening, true);
      throw new Error('launch failed');
    }, async () => { ran = true; }, { start }),
    /launch failed/,
  );
  assert.equal(ran, false);
  assert.equal(started.server.listening, false);
});

test('server is closed when browser close rejects', async () => {
  let started;
  const start = async (options) => (started = await startServer(options));
  const browser = { close: async () => { throw new Error('close failed'); } };
  await assert.rejects(
    withServerAndBrowser(async () => browser, async () => {}, { start }),
    /close failed/,
  );
  assert.equal(started.server.listening, false);
});
