import {mkdir,rm,writeFile} from 'node:fs/promises';
import {dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {render} from './src/site.mjs';
process.chdir(dirname(fileURLToPath(import.meta.url)));
const pages={tech:['index','this-pc','security','build','colophon'],world:['index','horizons','colophon'],life:['index','colophon'],live:['index','n001-stopping','colophon'],online:['index','colophon']};
const forbidden=['aedae_handoff_v1_3_final.md','handoff §','data-status="built"'];
await rm('dist',{recursive:true,force:true});await mkdir('dist/assets',{recursive:true});
await writeFile('dist/assets/styles.css',await (await import('node:fs/promises')).readFile('src/styles.css'));
let n=0;for(const [surface, names] of Object.entries(pages))for(const page of names){const html=render(surface,page);const claims=[...html.matchAll(/class="status ([^"]+)"[\s\S]*?source: ([^<]+)/g)];for(const [,status,source] of claims){if(status==='built')throw Error('status "built" is refused');if(!source.trim())throw Error(`claim on ${surface}/${page} has no data-source`)}for(const x of forbidden)if(html.includes(x))throw Error(`prohibited citation or status in ${surface}/${page}: ${x}`);const dir=`dist/${surface}/${page==='index'?'':page}`;await mkdir(dir,{recursive:true});await writeFile(`${dir}/index.html`,html);n++}
console.log(`Built ${n} pages across ${Object.keys(pages).length} surfaces`);
