import {mkdir, readFile, rm, writeFile} from 'node:fs/promises';
import {dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {render, surfaces, validateRenderedPage, validateStaticStylesheet, validateSurfaceRegistry} from './src/site.mjs';

process.chdir(dirname(fileURLToPath(import.meta.url)));

const pages = {
  tech: ['index', 'this-pc', 'security', 'build', 'colophon'],
  world: ['index', 'horizons', 'colophon'],
  life: ['index', 'colophon'],
  live: ['index', 'n001-stopping', 'colophon'],
  online: ['index', 'colophon']
};
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
    if (href.startsWith('#') || href.startsWith('data:image/svg+xml,')) continue;
    if (/^[a-z][a-z0-9+.-]*:/i.test(href) || href.startsWith('//')) throw new Error(`${route} has external link: ${href}`);
    const target = href.split('#', 1)[0];
    if (!outputRoutes.has(target)) throw new Error(`${route} has broken internal link: ${href}`);
  }
}

if (renderedPages.length !== 15 || outputRoutes.size !== 16) throw new Error('Unexpected generated route count');
console.log(`Built and validated ${renderedPages.length} pages across ${Object.keys(pages).length} surfaces`);
