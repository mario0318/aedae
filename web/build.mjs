import {mkdir, readFile, rm, writeFile} from 'node:fs/promises';
import {dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {render, surfaces, validateClaim, validateStaticStylesheet, validateSurfaceRegistry} from './src/site.mjs';

process.chdir(dirname(fileURLToPath(import.meta.url)));

const pages = {
  tech: ['index', 'this-pc', 'security', 'build', 'colophon'],
  world: ['index', 'horizons', 'colophon'],
  life: ['index', 'colophon'],
  live: ['index', 'n001-stopping', 'colophon'],
  online: ['index', 'colophon']
};
const forbiddenText = ['aedae_handoff_v1_3_final.md', 'handoff §'];
const forbiddenPatterns = [/<script\b/i, /<form\b/i, /\blocalStorage\b/i, /\bsessionStorage\b/i,
  /data-status\s*=\s*["']built["']/i, /\b(?:href|src)\s*=\s*["'](?:https?:)?\/\//i];

export function validateRenderedPage(html, route) {
  const h1Count = [...html.matchAll(/<h1(?:\s[^>]*)?>/g)].length;
  if (h1Count !== 1) throw new Error(`${route} has ${h1Count} h1 elements; expected exactly one`);
  for (const token of forbiddenText) if (html.includes(token)) throw new Error(`${route} contains prohibited token: ${token}`);
  for (const pattern of forbiddenPatterns) if (pattern.test(html)) throw new Error(`${route} matches prohibited pattern: ${pattern}`);

  const statuses = [...html.matchAll(/class="status /g)].length;
  const claims = [...html.matchAll(/<div class="claim"><span class="status ([^"]+)">[^<]+<\/span><p class="source">source: ([^<]+)<\/p><\/div>/g)];
  if (claims.length !== statuses) throw new Error(`${route} contains an unpaired rendered claim`);
  for (const [, status, source] of claims) validateClaim(status, source, `${route} rendered claim`);
}

validateSurfaceRegistry(surfaces);
await rm('dist', {recursive: true, force: true});
await mkdir('dist/assets', {recursive: true});
const stylesheet = await readFile('src/styles.css');
validateStaticStylesheet(stylesheet.toString('utf8'));
await writeFile('dist/assets/styles.css', stylesheet);

const outputRoutes = new Set(['/assets/styles.css']);
const renderedPages = [];
for (const [surface, names] of Object.entries(pages)) {
  for (const page of names) {
    const route = `/${surface}/${page === 'index' ? '' : `${page}/`}`;
    const html = render(surface, page);
    validateRenderedPage(html, route);
    const directory = `dist/${surface}/${page === 'index' ? '' : page}`;
    await mkdir(directory, {recursive: true});
    await writeFile(`${directory}/index.html`, html);
    outputRoutes.add(route);
    renderedPages.push([route, html]);
  }
}

for (const [route, html] of renderedPages) {
  for (const [, href] of html.matchAll(/href="([^"]+)"/g)) {
    if (href.startsWith('#')) continue;
    if (/^[a-z][a-z0-9+.-]*:/i.test(href) || href.startsWith('//')) throw new Error(`${route} has external link: ${href}`);
    const target = href.split('#', 1)[0];
    if (!outputRoutes.has(target)) throw new Error(`${route} has broken internal link: ${href}`);
  }
}

if (renderedPages.length !== 15 || outputRoutes.size !== 16) throw new Error('Unexpected generated route count');
console.log(`Built and validated ${renderedPages.length} pages across ${Object.keys(pages).length} surfaces`);
