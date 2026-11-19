import assert from 'node:assert/strict';
import test from 'node:test';
import { createEphemeralSigner, createSyntheticRequest } from './model.mjs';

const signer = createEphemeralSigner();
const original = createSyntheticRequest(Buffer.from([0xa1, 0x01, 0x02]));
const signature = signer.signRequest(original);

test('synthetic signature accepts its original request bytes', () => {
  assert.equal(signer.verifyRequest(original, signature), true);
});

test('a changed encoded request fails verification', () => {
  const changed = createSyntheticRequest(Buffer.from([0xa1, 0x01, 0x03]));
  assert.equal(signer.verifyRequest(changed, signature), false);
});

test('a changed transaction ID is invisible to the sample-observed signature boundary', () => {
  const changed = createSyntheticRequest(original.encodedRequest, {
    transactionId: 'synthetic-transaction-b',
  });
  assert.equal(signer.verifyRequest(changed, signature), true);
});

test('a changed request type and window context are also outside that boundary', () => {
  const changed = createSyntheticRequest(original.encodedRequest, {
    requestType: 'synthetic-get-assertion',
    windowContext: 'synthetic-window-b',
  });
  assert.equal(signer.verifyRequest(changed, signature), true);
});

test('a separate ephemeral signing key cannot authenticate the original signature', () => {
  assert.equal(createEphemeralSigner().verifyRequest(original, signature), false);
});

test('fixture overrides cannot replace the copied signed bytes', () => {
  assert.throws(
    () => createSyntheticRequest(original.encodedRequest, { encodedRequest: Buffer.alloc(0) }),
    TypeError,
  );
});
