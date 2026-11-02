export const surfaces = {
  tech: {
    name: 'aedae.tech', accent: 'var(--tech)', role: 'product / technical authority',
    glyph: '[æDæ]',
    state: 'specified', stateSource: 'see claim registry',
    title: 'A personal authenticator for This PC',
    lead: 'A Windows personal authenticator being specified with the gates on. Nothing is available to install yet.',
    facts: [
      {heading: 'What is true now', body: 'This reviewed repository tree contains a bootstrap COM skeleton, package layout, and contract checks. It does not contain a working authenticator.', status: 'verified', source: 'README.md; tasks.md T-001, T-002, T-014'},
      {heading: 'What is specified', body: 'The design calls for Windows Hello verification before assertions and fail-closed authenticated request handling. Those protocol capabilities are not implemented.', status: 'specified', source: 'security.md; functional.md; tasks.md T-007, T-015'},
      {heading: 'What remains open', body: 'The public plugin contract does not define the operation-signing envelope needed to complete request-authentication design.', status: 'research', source: 'tasks.md T-015; reports/operation-signature-gate-design.md'}
    ],
    links: [['this-pc', 'This PC, in plain language'], ['security', 'Security boundaries'], ['build', 'Build gates'], ['colophon', 'Colophon']]
  },
  world: {
    name: 'aedae.world', accent: 'var(--world)', role: 'thesis / research horizon',
    glyph: 'æDæ↗',
    state: 'horizon', stateSource: 'aedae venture positioning; public direction',
    title: 'Tools that leave more of you in your hands',
    lead: 'æDæ begins with identity. Its wider direction is practical capability without needless surrender, extraction, or gatekeeping.',
    facts: [
      {heading: 'The direction', body: 'Local-first tools can make more room for autonomy, dignity, and informed choice. This is an intention, not a product roadmap.', status: 'horizon', source: 'aedae venture positioning; public direction'},
      {heading: 'The test', body: 'A future direction earns its place only if it improves real capability without making access conditional on surveillance or status.', status: 'horizon', source: 'aedae venture positioning'},
      {heading: 'The limit', body: 'No funding, staffing, launch date, or market outcome is implied here.', status: 'verified', source: 'tasks.md; current repository state'}
    ],
    links: [['horizons', 'Read the horizons'], ['colophon', 'Colophon']]
  },
  life: {
    name: 'aedae.life', accent: 'var(--life)', role: 'practical capability / practice',
    glyph: 'æDæ+',
    state: 'practice', stateSource: 'general practice, not an aeDae product claim',
    title: 'Small practices for keeping agency close',
    lead: 'Vendor-neutral notes about recovery, access, and understanding your own systems. These are practices, not æDæ product claims.',
    facts: [
      {heading: 'Keep recovery legible', body: 'Write down which recovery paths you control, then test one before you need it.', status: 'practice', source: 'general account-recovery practice, not sourced from the aeDae repository'},
      {heading: 'Separate convenience from custody', body: 'A convenient sign-in flow may still depend on someone else holding the recovery path. Learn which is which.', status: 'practice', source: 'general digital-literacy practice, not sourced from the aeDae repository'},
      {heading: 'Name the blind spots', body: 'No dashboard can automatically show every credential held by every provider. Treat a missing view as a limit, not proof.', status: 'practice', source: 'general passkey ecosystem limitation, not sourced from the aeDae repository'},
      {heading: 'Write a readme for your life', body: 'Joey the cat walks you through nine short sections and you download your Life README. It runs in your browser and sends nothing. Drafts last only for the browser tab unless you choose to keep them on your device. It is not a will.', status: 'verified', source: 'web/src/readme; reports/readme-life-proposal.md'}
    ],
    links: [['readme', 'Write your Life README with Joey'], ['colophon', 'What practice means here']]
  },
  live: {
    name: 'aedae.live', accent: 'var(--live)', role: 'field notes / experiments',
    glyph: 'æDæ:',
    state: 'research', stateSource: 'tasks.md T-015; reports/operation-signature-gate-design.md',
    title: 'A place for work in motion',
    lead: 'Experiments, decisions, and interruptions. This surface does not pretend that activity is progress.',
    facts: [
      {heading: 'First note', body: 'The authenticator protocol lane is paused pending a stable, public answer about operation signing. Stopping is part of the record.', status: 'research', source: 'tasks.md T-015; reports/operation-signature-gate-design.md'},
      {heading: 'What belongs here', body: 'Real notes: decisions, test results, reversals, and unanswered questions.', status: 'horizon', source: 'aedae web information architecture'},
      {heading: 'What does not', body: 'Artificial cadence, manufactured demos, or a feed maintained only to appear alive.', status: 'verified', source: 'current public-site policy'}
    ],
    links: [['n001-stopping', 'Note 001: stopping'], ['colophon', 'Colophon']]
  },
  online: {
    name: 'aedae.online', accent: 'var(--online)', role: 'directory / entry point',
    glyph: 'æDæ→',
    state: 'horizon', stateSource: 'aedae web information architecture',
    title: 'Find the right aeDae surface',
    lead: 'A quiet directory for an institution that may grow in several directions without becoming several disconnected brands.',
    facts: [
      {heading: 'Technical product state', body: 'Product, security, and build claims are authoritative on aedae.tech only.', status: 'verified', source: 'aedae web information architecture'},
      {heading: 'The present directory', body: 'The five surfaces are public roles, not five independent products.', status: 'verified', source: 'aedae web information architecture'},
      {heading: 'The future', body: 'This may become a platform entry point only if a real platform exists and earns the added complexity.', status: 'horizon', source: 'aedae web information architecture'}
    ],
    links: [['colophon', 'Colophon']]
  }
};

export const allowedStatuses = new Set(['verified', 'specified', 'research', 'horizon', 'practice']);

export function validateClaim(status, source, context = 'claim') {
  if (status === 'built') throw new Error(`${context} uses refused status "built"`);
  if (!allowedStatuses.has(status)) throw new Error(`${context} has invalid status: ${status}`);
  if (typeof source !== 'string' || !source.trim()) throw new Error(`${context} has no source`);
  if (/[<>]/.test(source)) throw new Error(`${context} source contains markup`);
}

export function validateStaticStylesheet(css) {
  if (/@import\b|url\s*\(/i.test(css)) throw new Error('stylesheet contains an external-capable resource reference');
}

// The Life README builder is the only route allowed to run script. It must load exactly one
// same-origin script, carry no inline code, and the script must have no way to reach the network.
export const builderScriptTag = '<script src="/life/readme/app.js" defer></script>';
export function validateBuilderPage(html) {
  const scriptOpens = [...html.matchAll(/<script\b/gi)].length;
  const scripts = [...html.matchAll(/<script\b[^>]*>[\s\S]*?<\/script>/gi)].map(match => match[0]);
  if (scriptOpens !== 1 || scripts.length !== 1 || scripts[0] !== builderScriptTag) throw new Error('builder page must load only the same-origin builder script');
  if (/\son[a-z]+\s*=/i.test(html)) throw new Error('builder page contains an inline event handler');
  if (/\sstyle\s*=/i.test(html)) throw new Error('builder page contains an inline style');
  if (/<form\b|<iframe\b|<object\b|<embed\b|<base\b/i.test(html)) throw new Error('builder page contains a form, frame or base element');
  if (/http-equiv\s*=\s*["']?refresh/i.test(html)) throw new Error('builder page contains a meta refresh');
  // Every link and resource must be a fixed same-origin path, an in-page anchor, or a static mailto.
  for (const [, attr, raw] of html.matchAll(/\b(href|src|action|formaction|srcset|poster|data)\s*=\s*("[^"]*"|'[^']*'|[^\s>]+)/gi)) {
    const value = raw.replace(/^["']|["']$/g, '');
    const ok = /^\/(?!\/)[\w./-]*$/.test(value) || /^#[\w-]*$/.test(value) || (attr.toLowerCase() === 'href' && /^mailto:/i.test(value));
    if (!ok) throw new Error(`builder page has a disallowed ${attr} target: ${value}`);
  }
  const csp = html.match(/http-equiv="Content-Security-Policy"\s+content="([^"]*)"/)?.[1] ?? '';
  for (const directive of ["connect-src 'none'", "form-action 'none'", "base-uri 'none'", "default-src 'none'"]) {
    if (!csp.includes(directive)) throw new Error(`builder page CSP is missing ${directive}`);
  }
}
// A CSP cannot stop top-level navigation, so the script itself must never navigate or build URLs.
// The one allowed href write is a same-origin blob download link.
export function validateBuilderScript(js) {
  const banned = [
    /\b(?:fetch|XMLHttpRequest|WebSocket|EventSource|sendBeacon|importScripts|RTCPeerConnection|SharedWorker|Worker|postMessage|eval|Function)\b/,
    /\blocation\b/, /\bopen\s*\(/, /\.assign\s*\(/, /\.replace\s*\(\s*["'`]?(?:https?:|\/\/)/i,
    /\b(?:window|self|globalThis|top|parent|frames|navigator|document)\s*\??\.?\s*\[/,
    /\bimport\s*\(/, /\bdocument\.(?:cookie|write|writeln|domain)\b/, /\bsrcdoc\b/,
    /\.(?:src|action|formAction|srcset|data)\s*=(?!=)/, /setAttribute\s*\(\s*["'`](?:href|src|action|formaction)/i,
    /\baction\s*=/i, /<form\b|<iframe\b|<meta\b|<base\b|<a\s[^>]*href/i,
    /\bset(?:Timeout|Interval)\s*\(\s*["'`]/, /createElement\s*\(\s*["'`](?!a["'`])/,
    /\b(?:https?|wss?|ftp):\/\//i, /["'`]\/\/[a-z0-9]/i, /\bmailto:/i
  ];
  for (const pattern of banned) {
    const hit = js.match(pattern);
    if (hit) throw new Error(`builder script contains a network-capable, navigating or dynamic-code construct: ${hit[0]}`);
  }
  const hrefWrites = [...js.matchAll(/\.href\s*=(?!=)/g)].length;
  const blobWrites = [...js.matchAll(/\.href\s*=\s*URL\.createObjectURL\(/g)].length;
  if (hrefWrites !== blobWrites) throw new Error('builder script assigns an href other than a blob download');
}

export function validateSurfaceRegistry(registry) {
  for (const [surfaceKey, surface] of Object.entries(registry)) {
    validateClaim(surface.state, surface.stateSource, `${surfaceKey} state`);
    if (typeof surface.glyph !== 'string' || !surface.glyph.includes('æDæ')) {
      throw new Error(`${surfaceKey} has no shared glyph skeleton`);
    }
    if (!Array.isArray(surface.facts) || !surface.facts.length) throw new Error(`${surfaceKey} has no facts`);
    surface.facts.forEach((fact, index) => {
      if (!fact || typeof fact.heading !== 'string' || !fact.heading.trim() ||
          typeof fact.body !== 'string' || !fact.body.trim()) {
        throw new Error(`${surfaceKey} fact ${index} is incomplete`);
      }
      validateClaim(fact.status, fact.source, `${surfaceKey} fact ${index}`);
    });
  }
}

const chip = (status, source) => {
  validateClaim(status, source);
  return `<div class="claim"><span class="status ${status}">${status}</span><p class="source">source: ${source}</p></div>`;
};

const body = (surfaceKey, surface, content) => `<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="description" content="${surface.lead}"><title>${surface.name} — ${surface.title}</title>
<link rel="stylesheet" href="/assets/styles.css"></head>
<body class="surface-${surfaceKey}"><a class="skip" href="#main">Skip to content</a><div class="shell">
<nav class="nav" aria-label="aeDae surfaces"><a class="mark" href="/${surfaceKey}/" aria-label="${surface.name} home">æDæ</a>${Object.entries(surfaces).map(([key, item]) => `<a ${key === surfaceKey ? 'aria-current="page"' : ''} href="/${key}/">${item.name}</a>`).join('')}</nav>
<main id="main">${content}</main>
<footer>Product state, security claims, and build status are authoritative on <a href="/tech/">aedae.tech</a> only. This site sets no cookies, uses no analytics, and has no third-party embeds.</footer>
</div></body></html>`;

export function render(surfaceKey, page) {
  const surface = surfaces[surfaceKey];
  if (!surface) throw new Error(`Unknown surface: ${surfaceKey}`);
  let extra = '';
  if (page === 'security') extra = '<section class="section"><h1>Security boundaries</h1><p>Private keys, Windows Hello invocation, credential registration, and signing paths are not implemented. The project must fail closed when those paths are eventually considered.</p>' + chip('specified', 'security.md; tasks.md') + '</section><section class="section continuation"><h2>What the header proves</h2><p>The locked local Windows SDK exposes <code>webauthnplugin.h</code>. Header presence does not establish a supported operation-signing envelope.</p>' + chip('verified', 'reports/sample-mapping.md; reports/webauthnplugin-contract.md') + '</section>';
  if (page === 'build') extra = '<section class="section"><h1>Gates before protocol work</h1><p>The local lane may proceed with synthetic records and mock UI. The protocol lane is paused at T-015 because the required signing envelope is absent from the reviewed contract. A pause is not a delivery date.</p>' + chip('research', 'tasks.md T-015; reports/operation-signature-gate-design.md') + '</section><section class="section continuation"><h2>What the contract verifier proves</h2><p>The verifier checks the locked plugin headers and recorded hashes. That verifies the build contract only; it does not resolve operation signing.</p>' + chip('verified', 'reports/webauthnplugin-contract.md; reports/webauthnplugin-abi-manifest.json') + '</section>';
  if (page === 'this-pc') extra = '<section class="section"><h1>Where it would sit</h1><p>Browser → Windows WebAuthn broker → æDæ plugin → Windows verification → local vault. This is the intended architecture, not a live flow.</p>' + chip('specified', 'architecture.md; functional.md') + '</section>';
  if (page === 'horizons') extra = '<section class="section"><h1>What would have to be true</h1><p>A stable platform path, demonstrated user need, maintainable security practice, and a way to extend capability without making people surrender data or agency.</p>' + chip('horizon', 'aedae venture positioning') + '</section>';
  if (page === 'n001-stopping') extra = '<section class="section"><h1>Note 001: stopping</h1><p>The public headers do not define the operation-signing envelope. The responsible action is to keep protocol work paused until the question is resolved or recorded as unavailable.</p>' + chip('research', 'tasks.md T-015; reports/operation-signature-gate-design.md') + '</section>';
  if (page === 'colophon') extra = '<section class="section"><h1>Colophon</h1><p>This website is static. It sets no cookies, runs no analytics, and embeds no third-party content. One route, the Life README builder at /life/readme/, runs a same-origin script and makes no network requests. Its draft lasts only for the browser tab unless you tick \u201ckeep on this device\u201d. Its password check is a heuristic nudge, not a guarantee: the format\u2019s rule is pointers, never secrets, so check the file before you share it. Every other page runs no script and writes no local storage. That describes this website, not unshipped software.</p>' + chip('verified', 'aedae web public-site policy') + '</section>';

  const home = `<section class="hero"><div><p class="kicker">${surface.name} / ${surface.role}</p><h1>${surface.title}</h1><p>${surface.lead}</p>${chip(surface.state, surface.stateSource)}</div><div><div class="specimen" role="img" aria-label="${surface.name} glyph construction specimen"><span class="glyph glyph-${surfaceKey}" aria-hidden="true">${surface.glyph}</span></div><p class="metrics">role: ${surface.role} / anchors: 24·56·104 / state: ${surface.state}</p></div></section><section class="section"><h2>What is on the record</h2><div class="grid">${surface.facts.map(fact => `<article class="item"><h3>${fact.heading}</h3><p>${fact.body}</p>${chip(fact.status, fact.source)}</article>`).join('')}</div></section><section class="section"><h2>Continue</h2><p class="continue-links">${surface.links.map(([path, label]) => `<a href="/${surfaceKey}/${path === 'colophon' ? 'colophon/' : `${path}/`}">${label}</a>`).join(' · ')}</p></section>`;
  return body(surfaceKey, surface, page === 'index' ? home : extra || `<section class="section"><h1>${page}</h1><p>This page is intentionally reserved for a sourced, reviewable record.</p></section>`);
}
