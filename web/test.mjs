import {readFile} from 'node:fs/promises';
import {builderScriptTag, render, renderMembershipPage, validateMembershipPage, waitlistPattern, surfaces, validateBuilderPage, validateBuilderScript, validateClaim, validateStaticStylesheet, validateSurfaceRegistry} from './src/site.mjs';

let assertions = 0;
const check = (condition, label) => {
  if (!condition) throw new Error(label);
  assertions += 1;
  console.log(`PASS ${label}`);
};
const rejects = (action, pattern, label) => {
  try { action(); } catch (error) {
    check(pattern.test(error.message), label);
    return;
  }
  throw new Error(`${label}: expected rejection`);
};

validateSurfaceRegistry(surfaces);
check(true, 'production claim registry accepted');

const siteSource = (await readFile('src/site.mjs', 'utf8')).replace(/\r\n/g, '\n');
const taskSource = (await readFile('../tasks.md', 'utf8')).replace(/\r\n/g, '\n');
const referencedTasks = [...new Set([...siteSource.matchAll(/\bT-\d{3}\b/g)].map(match => match[0]))];
for (const task of referencedTasks) {
  check(new RegExp(`^##+ ${task}\\b`, 'm').test(taskSource), `site citation exists: ${task}`);
}
check(new Set(Object.values(surfaces).map(surface => surface.glyph)).size === Object.keys(surfaces).length,
  'every surface has a distinct form of the shared glyph skeleton');
for (const surface of Object.keys(surfaces)) {
  const rendered = render(surface, 'index');
  check(rendered.includes(`<body class="surface-${surface}">`), `${surface} renders a stylesheet-owned accent class`);
  check(!rendered.includes('<body style='), `${surface} avoids CSP-blocked inline body styles`);
}
rejects(() => validateClaim('verified', ''), /no source/, 'unsourced claim rejected directly');
rejects(() => validateClaim('built', 'tasks.md'), /refused status/, 'built status rejected directly');
rejects(() => validateClaim('invented', 'tasks.md'), /invalid status/, 'unknown status rejected directly');
rejects(() => validateClaim('verified', '<em>not a source</em>'), /contains markup/,
  'claim source markup rejected');
rejects(() => validateStaticStylesheet('@import "https://example.test/style.css";'), /resource reference/,
  'stylesheet import rejected');
rejects(() => validateStaticStylesheet('.hero{background:url(https://example.test/a.png)}'), /resource reference/,
  'stylesheet URL rejected');

const adjacentSourceTrap = structuredClone(surfaces);
adjacentSourceTrap.tech.facts[0].source = '';
adjacentSourceTrap.tech.facts[1].source = 'later source must not satisfy the first claim';
rejects(() => validateSurfaceRegistry(adjacentSourceTrap), /tech fact 0 has no source/,
  'later claim source cannot satisfy an unsourced earlier claim');

const css = (await readFile('src/styles.css', 'utf8')).replace(/\r\n/g, '\n');
for (const selector of ['.mark', '.nav a']) {
  const rule = css.match(new RegExp(`\\${selector.replace(' ', '\\s+')}\\{([^}]*)\\}`));
  const minimum = rule?.[1].match(/min-height:\s*(\d+)px/);
  check(minimum && Number(minimum[1]) >= 44, `${selector} declares at least a 44px minimum target`);
}

const luminance = hex => {
  const channels = hex.match(/[0-9a-f]{2}/gi).map(value => parseInt(value, 16) / 255)
    .map(value => value <= 0.04045 ? value / 12.92 : ((value + 0.055) / 1.055) ** 2.4);
  return 0.2126 * channels[0] + 0.7152 * channels[1] + 0.0722 * channels[2];
};
const contrast = (a, b) => {
  const values = [luminance(a), luminance(b)].sort((x, y) => y - x);
  return (values[0] + 0.05) / (values[1] + 0.05);
};
const root = css.match(/:root\{([^}]*)\}/)[1];
const dark = css.match(/@media \(prefers-color-scheme:dark\)\{\s*:root\{([^}]*)\}/)[1];
const token = (block, name) => block.match(new RegExp(`--${name}:\\s*(#[0-9a-fA-F]{6})`))[1];
for (const status of ['verified', 'specified', 'research', 'horizon', 'practice']) {
  check(contrast(token(root, `status-${status}`), token(root, 'mineral')) >= 4.5,
    `${status} status meets light-mode text contrast`);
  check(contrast(token(dark, `status-${status}`), token(dark, 'mineral')) >= 4.5,
    `${status} status meets dark-mode text contrast`);
}
for (const surface of ['tech', 'world', 'life', 'live', 'online']) {
  check(contrast(token(root, surface), token(root, 'mineral')) >= 4.5,
    `${surface} accent meets light-mode small-text contrast`);
  check(contrast(token(dark, surface), token(dark, 'mineral')) >= 4.5,
    `${surface} accent meets dark-mode small-text contrast`);
}

const builderHtml = (await readFile('src/readme/index.html', 'utf8')).replace(/\r\n/g, '\n');
const builderJs = (await readFile('src/readme/app.js', 'utf8')).replace(/\r\n/g, '\n');
validateBuilderPage(builderHtml);
check(true, 'readme builder page accepted');
validateBuilderScript(builderJs);
check(true, 'readme builder script accepted');
const pageRejects = [
  [builderHtml.replace(builderScriptTag, builderScriptTag + '<script>alert(1)</script>'), /only the same-origin/, 'inline script'],
  [builderHtml.replace(builderScriptTag, '<script src="https://cdn.example/x.js"></script>'), /only the same-origin/, 'external script'],
  [builderHtml.replace('</body>', '<script src="/x.js"></body>'), /only the same-origin/, 'unclosed second script'],
  [builderHtml.replace('<main>', '<main onclick="x()">'), /inline event handler/, 'inline handler'],
  [builderHtml.replace("connect-src 'none'", 'connect-src *'), /connect-src 'none'/, 'missing connect-src none'],
  [builderHtml.replace("form-action 'none'", ''), /form-action 'none'/, 'missing form-action none'],
  [builderHtml.replace('</head>', '<meta http-equiv="refresh" content="0;url=https://x.test/"></head>'), /meta refresh/, 'meta refresh'],
  [builderHtml.replace('href="/life/"', 'href=//x.test/'), /disallowed href/, 'unquoted protocol-relative link'],
  [builderHtml.replace('href="/life/"', 'href="https://x.test/"'), /disallowed href/, 'external link'],
  [builderHtml.replace('href="/life/"', 'href="javascript:alert(1)"'), /disallowed href/, 'javascript: link'],
  [builderHtml.replace('<main>', '<main><form action="/x">'), /form, frame or base/, 'form element']
];
for (const [html, pattern, label] of pageRejects) rejects(() => validateBuilderPage(html), pattern, `builder page rejects ${label}`);

const scriptBypasses = [
  'fetch("/x")', 'fetch?.("/x")', 'window["fe"+"tch"]("/x")', 'globalThis["fetch"]', 'navigator.sendBeacon.call(navigator,"/x")',
  'new WebSocket("wss://x")', 'document.createElement("img").src="/p"', 'new Image().src="/p"', 'setTimeout("x()",1)',
  'location.href="mailto:"+data.name', 'window.location.assign("/x")', 'location.replace("/x")', 'window.open("/x?d="+data)',
  'a.href="/x?d="+JSON.stringify(data)', 'el.setAttribute("href","/x")', 'f.action="/x"', 'document.write("<a>")',
  'eval("1")', 'import("./x.js")', 'x.innerHTML=`<a href="/x">`', 'x.innerHTML=`<meta http-equiv=refresh>`'
];
for (const sample of scriptBypasses) {
  rejects(() => validateBuilderScript(builderJs + '\n' + sample), /network-capable|assigns an href/, `builder script rejects ${sample}`);
}
check(!/\blocation\b|\baction\s*=/i.test(builderJs) && !/\baction=/i.test(builderHtml.replace(/form-action/g, '')),
  'builder bundle contains no location or action= at all');
check(/<a href="mailto:\?subject=[^"&]*">Open my email app<\/a>/.test(builderHtml) && !builderJs.includes('mailto'),
  'email app link is a fixed mailto with no draft data');
check(/navigator\.clipboard\.writeText/.test(builderJs) && /class="msg" readonly/.test(builderJs),
  'tell-my-people shows personal messages to copy instead of sending them');
check(/Aedae never contacts anyone\./.test(builderHtml), 'tell-my-people says Aedae never contacts anyone');
check(/Unticking deletes it from this device right away\./.test(builderJs), 'keep-on-device note says unticking deletes immediately');
check(/<a class="btn" id="member" href="\/life\/readme\/membership\/">/.test(builderHtml), 'fake-door button links to the same-origin membership page');
check(/<img class="donemark" src="\/life\/readme\/done\.svg"[^>]*loading="lazy"[^>]*>/.test(builderHtml) && /\$\("#after"\)\.hidden=false/.test(builderJs),
  'download reveals the fixed same-origin completion image');

// 1. Drafts are tab-only unless the person opts in.
check(/sessionStorage/.test(builderJs) && /keep\?ls\(\):ss\(\)/.test(builderJs), 'drafts default to sessionStorage');
check(/<input type="checkbox" id="keep">/.test(builderHtml), 'keep-on-device checkbox exists and is unchecked by default');
check(builderHtml.includes('Keep this on this device between visits.'), 'keep-on-device checkbox has the agreed label');
check(/removeItem\(OLD_KEY\)/.test(builderJs), 'drafts saved by earlier versions without consent are removed');
check(/if\(!keep\)try\{ss\(\)\?\.removeItem\(KEY\)\}/.test(builderJs), 'tab draft is cleared on download unless kept');

// 3. Copy rules: no mortality jokes or game-score language.
for (const banned of ['You get one', '6pm', 'nap', 'Joey approves', 'nine lives done', 'windowsill', "I'll make sure each one hears"]) {
  check(!builderJs.includes(banned) && !builderHtml.includes(banned), `builder copy no longer says "${banned}"`);
}
check(builderHtml.includes('Joey has nine lives. Yours deserves a plan.'), 'builder headline uses the agreed line');

// 4. The password check admits its limits.
check(builderJs.includes("he's a cat, not a scanner. Check the file before you share it."), 'secret warning says Joey can miss things');
check(render('life', 'colophon').includes('heuristic nudge, not a guarantee'), 'colophon states the password check is a heuristic');

// 5. Essentials path covers exactly three sections.
check(/const ESSENTIALS=\["first24","people","health"\];/.test(builderJs), 'essentials path is First 24 hours, People, Health & care');
check(/id="essentials"/.test(builderJs) && /id="full"/.test(builderJs), 'builder offers essentials and full entry points');
check(/src="\/life\/readme\/done\.svg"[^>]*referrerpolicy="no-referrer"/.test(builderHtml), 'completion image sends no referrer');
const headersTemplate = (await readFile('deploy/_headers.template', 'utf8')).replace(/\r\n/g, '\n');
check(/# .*img-src 'self' is intentional[\s\S]*\/life\/readme\/\*\n  ! Content-Security-Policy\n  Content-Security-Policy: [^\n]*img-src 'self'[^\n]*connect-src 'none'/.test(headersTemplate),
  'headers file documents the intentional img-src and keeps connect-src none');
check(/\$headerText = \$headerText -replace "`r`n", "`n"/.test((await readFile('deploy/prepare.ps1', 'utf8')).replace(/\r\n/g, '\n')),
  'prepare writes _headers with LF line endings');
const deployScript = (await readFile('deploy/deploy.ps1', 'utf8')).replace(/\r\n/g, '\n');
check(/\[string\[\]\]\$Surface = @\('life'\)/.test(deployScript), 'deploy defaults to the life surface only');
check(/'--branch', \$Branch/.test(deployScript) && /ValidateScript\(\{ \$_ -ne 'main' \}\)/.test(deployScript),
  'deploy always uses a non-production preview branch');
check(/selected: press ctrl\+c/i.test(builderJs), 'copy button falls back to selecting the text');
check(/\[switch\]\$DryRun/.test(deployScript) && /DRY RUN: \$wranglerCommand pages deploy `"\$dist`" --project-name \$project --branch \$Branch/.test(deployScript) && /continue/.test(deployScript),
  'deploy has a dry-run mode that prints the exact preview command');
{
  // Structural, not cosmetic: in a dry run, no Wrangler call may be reachable.
  const at = text => deployScript.indexOf(text);
  const guardStart = at('        if ($DryRun) {'), skip = at('            continue'), create = at("'pages' 'project' 'create'"), upload = at("Invoke-Wrangler @('pages', 'deploy'");
  const whoamiGuard = /if \(\$DryRun\) \{[^}]*\} else \{\s*Write-Host "`n-- Auth check --"\s*Invoke-Wrangler @\('whoami'\)/.test(deployScript);
  const calls = [...deployScript.matchAll(/Invoke-Wrangler @\(|& \$npx /g)].length;
  check(whoamiGuard && guardStart > 0 && guardStart < skip && skip < create && create < upload && calls === 4,
    'dry run skips every Wrangler call: login is in the else branch, create and deploy come after continue');
  check(/--branch \$Branch --config `"\$config`""/.test(deployScript) && /'--branch', \$Branch, '--config'/.test(deployScript),
    'dry run prints the same $Branch variable the real deploy sends');
}
const membershipTemplate = (await readFile('src/readme/membership.html', 'utf8')).replace(/\r\n/g, '\n');
const tally = 'https://tally.so/r/abc123';
validateMembershipPage(renderMembershipPage(membershipTemplate, ''), '');
check(true, 'membership page renders without a waitlist link');
const withTally = renderMembershipPage(membershipTemplate, tally);
validateMembershipPage(withTally, tally);
check(withTally.includes(`href="${tally}"`) && withTally.includes('hosted by Tally (tally.so)'), 'membership page links and names Tally when configured');
check(/isn't open yet/.test(withTally) && /Nothing is charged today/.test(withTally), 'membership page says plainly it does not exist yet');
rejects(() => renderMembershipPage(membershipTemplate, 'https://evil.test/r/x'), /Tally share link/, 'non-Tally waitlist URL rejected');
rejects(() => validateMembershipPage(withTally.replace('</main>', '<a href="https://x.test/">x</a></main>'), tally), /disallowed href/, 'membership page rejects other external links');
rejects(() => validateMembershipPage(withTally.replace('</body>', '<script src="/x.js"></script></body>'), tally), /must be static/, 'membership page rejects script');
const config = JSON.parse((await readFile('src/readme/config.json', 'utf8')).replace(/\r\n/g, '\n'));
check(config.waitlistUrl === '' || waitlistPattern.test(config.waitlistUrl), 'configured waitlist URL is empty or a Tally link');

const lifeHome = render('life', 'index');
check(lifeHome.includes('href="/life/readme/"'), 'life surface links to the readme builder');
for (const surface of ['tech', 'world', 'live', 'online']) {
  check(!render(surface, 'index').includes('<script'), `${surface} still renders no script`);
}

console.log(`${assertions} focused website assertions passed`);
