import {readFile} from 'node:fs/promises';
import {surfaces, validateClaim, validateStaticStylesheet, validateSurfaceRegistry} from './src/site.mjs';

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

const siteSource = await readFile('src/site.mjs', 'utf8');
const taskSource = await readFile('../tasks.md', 'utf8');
const referencedTasks = [...new Set([...siteSource.matchAll(/\bT-\d{3}\b/g)].map(match => match[0]))];
for (const task of referencedTasks) {
  check(new RegExp(`^##+ ${task}\\b`, 'm').test(taskSource), `site citation exists: ${task}`);
}
check(new Set(Object.values(surfaces).map(surface => surface.glyph)).size === Object.keys(surfaces).length,
  'every surface has a distinct form of the shared glyph skeleton');
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

const css = await readFile('src/styles.css', 'utf8');
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

console.log(`${assertions} focused website assertions passed`);
