// Create (or find) the Life README membership waitlist form on Tally and print its share link.
// Needs TALLY_API_KEY in the environment (a GitHub Actions secret, or a cloud-session network secret).
// Usage: node deploy/create-tally-form.mjs [--write]   (--write also saves the link to src/readme/config.json)
// Without TALLY_API_KEY it prints the request it would send and exits 0, so it is safe to dry-run.
import {randomUUID} from 'node:crypto';
import {readFile, writeFile} from 'node:fs/promises';
import {dirname, join} from 'node:path';
import {fileURLToPath} from 'node:url';

const NAME = 'Life README membership waitlist';
const API = 'https://api.tally.so';
const key = process.env.TALLY_API_KEY;
const configPath = join(dirname(fileURLToPath(import.meta.url)), '..', 'src', 'readme', 'config.json');

const block = (type, payload) => { const uuid = randomUUID(); return {uuid, type, groupUuid: uuid, groupType: type, payload}; };
const body = {
  status: 'PUBLISHED',
  blocks: [
    block('FORM_TITLE', {title: NAME, html: "<h1>Membership isn't open yet</h1>"}),
    block('TEXT', {html: "<p>Leave your email and we'll tell you if Life README membership opens ($39/yr: a yearly reminder and a backup your trusted people could reach). Nothing is charged, and we'll use your email only for this.</p>"}),
    block('INPUT_EMAIL', {isRequired: true, placeholder: 'you@example.com'})
  ]
};

if (!key) {
  console.log('TALLY_API_KEY is not set. Dry run: this is the request that would be sent.');
  console.log(`POST ${API}/forms`);
  console.log(JSON.stringify(body, null, 2));
  process.exit(0);
}

const call = async (method, path, payload) => {
  const res = await fetch(API + path, {
    method,
    headers: {Authorization: `Bearer ${key}`, 'Content-Type': 'application/json'},
    body: payload ? JSON.stringify(payload) : undefined
  });
  const text = await res.text();
  if (!res.ok) throw new Error(`${method} ${path} -> ${res.status}: ${text.slice(0, 300)}`);
  return text ? JSON.parse(text) : {};
};

// Reuse an existing form with the same name so re-running never creates duplicates.
const listed = await call('GET', '/forms');
const forms = Array.isArray(listed) ? listed : (listed.items || listed.forms || listed.data || []);
let form = forms.find(f => f.name === NAME && !f.isClosed);
console.log(form ? `Found existing form ${form.id}` : 'Creating form');
if (!form) form = await call('POST', '/forms', body);

const url = `https://tally.so/r/${form.id}`;
if (!/^https:\/\/tally\.so\/r\/[A-Za-z0-9]+$/.test(url)) throw new Error(`Unexpected form id: ${form.id}`);
console.log(`WAITLIST_URL=${url}`);

if (process.argv.includes('--write')) {
  const config = JSON.parse(await readFile(configPath, 'utf8'));
  config.waitlistUrl = url;
  await writeFile(configPath, JSON.stringify(config, null, 2) + '\n');
  console.log(`Saved to ${configPath}`);
}
