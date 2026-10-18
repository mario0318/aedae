import { constants, generateKeyPairSync, sign, verify } from 'node:crypto';

// Models only the sample-observed signed input: encoded request bytes.
// This is not a Windows plugin verifier or a claim about the platform contract.
export function createSyntheticRequest(encodedRequest, overrides = {}) {
  if (Object.hasOwn(overrides, 'encodedRequest')) {
    throw new TypeError('encodedRequest must be supplied as the first argument');
  }
  return {
    encodedRequest: Buffer.from(encodedRequest),
    transactionId: 'synthetic-transaction-a',
    requestType: 'synthetic-make-credential',
    windowContext: 'synthetic-window-a',
    ...overrides,
  };
}

export function createEphemeralSigner() {
  const { publicKey, privateKey } = generateKeyPairSync('rsa', {
    modulusLength: 2048,
  });
  const options = { padding: constants.RSA_PKCS1_PSS_PADDING, saltLength: 32 };
  return {
    signRequest(request) {
      return sign('sha256', request.encodedRequest, { key: privateKey, ...options });
    },
    verifyRequest(request, signature) {
      return verify('sha256', request.encodedRequest, { key: publicKey, ...options }, signature);
    },
  };
}
