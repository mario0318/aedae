import { startServer } from '../server.mjs';

// Starts the loopback server, then the browser, and always closes whatever started, so a
// browser that fails to launch cannot leave the HTTP server holding the test process open.
export async function withServerAndBrowser(launch, run, { start = startServer } = {}) {
  const { server, origin } = await start({ port: 0 });
  let browser;
  try {
    browser = await launch();
    return await run({ browser, origin });
  } finally {
    try {
      await browser?.close();
    } finally {
      await new Promise((resolve) => server.close(resolve));
    }
  }
}
