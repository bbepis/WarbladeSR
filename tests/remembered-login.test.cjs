'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const fs = require('node:fs');
const path = require('node:path');

async function fixture(browser) {
  const context = await browser.newContext();
  const page = await context.newPage();
  const errors = [], requests = [];
  let failedDownload = false;
  page.on('pageerror', error => errors.push(error.message));
  await page.route('http://warblade.test/**', async route => {
    const request = route.request(), url = new URL(request.url());
    if (url.pathname.startsWith('/api/')) {
      requests.push([request.method(), url.pathname]);
      assert.equal(request.headers().authorization, undefined, 'sessions belong only in HttpOnly cookies');
      const cookie = request.headers().cookie || '';
      const username = cookie.includes('warblade_session=alice') ? 'Alice' :
        cookie.includes('warblade_session=bob') ? 'Bob' : null;
      if (url.pathname === '/api/me') return route.fulfill({ json: { username } });
      if (url.pathname === '/api/login') {
        const fields = request.postDataJSON();
        assert.equal(fields.password, 'Eight123!');
        const player = fields.username.toLowerCase();
        return route.fulfill({ headers: { 'Set-Cookie':
          `warblade_session=${player}; Path=/; HttpOnly; SameSite=Lax; Max-Age=2592000` },
        json: { username: player === 'alice' ? 'Alice' : 'Bob' } });
      }
      if (url.pathname === '/api/logout') return route.fulfill({ headers: {
        'Set-Cookie': 'warblade_session=; Path=/; HttpOnly; Max-Age=0' }, json: {} });
      if (!username) return route.fulfill({ status: 401, json: {} });
      if (url.pathname === '/api/saves') return route.fulfill({ json: { files: [
        { path: 'warblade/profiles/profile000.acc', version: 3 },
        { path: 'warblade/settings.dat', version: 4 },
      ] } });
      if (url.pathname.endsWith('/profile000.acc')) return route.fulfill({
        body: Buffer.from(username + ' progress') });
      if (url.pathname.endsWith('/settings.dat')) return route.fulfill(failedDownload ?
        { status: 503, body: '' } : { body: Buffer.from(username + ' settings') });
      if (url.pathname === '/api/hiscores') return route.fulfill({ headers: { ETag: '"9"' },
        body: Buffer.from('public scores') });
      throw new Error('Unexpected API request ' + url.pathname);
    }
    if (url.pathname === '/loading-title.jpg') return route.fulfill({ status: 404, body: '' });
    if (url.pathname === '/warblade.js') return route.fulfill({ body: '' });
    const file = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
    return route.fulfill({ contentType: file.endsWith('.js') ? 'text/javascript' :
      file.endsWith('.css') ? 'text/css' : 'text/html',
    body: fs.readFileSync(path.join(__dirname, '../web', file)) });
  });
  await page.goto('http://warblade.test/');
  const prepare = () => page.evaluate(() => {
    const entries = new Map([['/save', null]]);
    window.FS = {
      stat(p) { if (!entries.has(p)) throw Error('missing'); return { mode: entries.get(p) === null ? 1 : 0 }; },
      isDir: mode => mode === 1,
      readdir(p) { return ['.', '..', ...[...entries.keys()].filter(k =>
        k.startsWith(p + '/') && !k.slice(p.length + 1).includes('/')).map(k => k.slice(p.length + 1))]; },
      mkdir: p => entries.set(p, null), rmdir: p => entries.delete(p), unlink: p => entries.delete(p),
      writeFile: (p, value) => entries.set(p, new Uint8Array(value)), readFile: p => entries.get(p),
    };
    persistSaves = async () => {};
  });
  await prepare();
  return { context, page, errors, requests, prepare,
    failDownload: () => { failedDownload = true; } };
}

test('remembered cookie reload restores the same saves; signout and change player isolate accounts', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { context, page, errors, prepare } = await fixture(browser);
    assert.equal(await page.evaluate(() => Module.authenticateGame('aLiCe', 'Eight123!', false)), true);
    const state = () => page.evaluate(() => ({ name: Module.accountName, ready: accountReady,
      profile: new TextDecoder().decode(FS.readFile('/save/warblade/profiles/profile000.acc')) }));
    assert.deepEqual(await state(), { name: 'Alice', ready: true, profile: 'Alice progress' });
    const cookie = (await context.cookies()).find(cookie => cookie.name === 'warblade_session');
    assert.equal(cookie.httpOnly, true);
    assert.equal(cookie.sameSite, 'Lax');
    assert.ok(cookie.expires > Date.now() / 1000 + 29 * 24 * 60 * 60);
    assert.equal(await page.evaluate(() => document.cookie), '');
    await page.reload();
    await prepare();
    await page.evaluate(() => restoreSession());
    assert.deepEqual(await state(), { name: 'Alice', ready: true, profile: 'Alice progress' });
    assert.deepEqual(await page.evaluate(async () => ({ local: localStorage.length,
      session: sessionStorage.length, databases: await indexedDB.databases() })),
    { local: 0, session: 0, databases: [] });
    await Promise.all([page.waitForNavigation(), page.evaluate(() => Module.signOutGame())]);
    await prepare();
    await page.evaluate(() => restoreSession());
    assert.equal(await page.evaluate(() => Module.accountName), '');
    assert.equal(await page.evaluate(() => accountReady), false);
    assert.equal((await context.cookies()).length, 0);
    assert.equal(await page.evaluate(() => Module.authenticateGame('BOB', 'Eight123!', false)), true);
    assert.deepEqual(await state(), { name: 'Bob', ready: true, profile: 'Bob progress' });
    assert.deepEqual(errors, []);
  } finally { await browser.close(); }
});

test('invalid cookie stays anonymous; interrupted restore discards partial private saves', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { context, page, requests, failDownload } = await fixture(browser);
    await context.addCookies([{ name: 'warblade_session', value: 'invalid',
      url: 'http://warblade.test', httpOnly: true }]);
    await page.evaluate(() => restoreSession());
    assert.deepEqual(requests, [['GET', '/api/me']]);
    assert.equal(await page.evaluate(() => Module.accountName), '');
    assert.equal(await page.evaluate(() => accountReady), false);
    await context.addCookies([{ name: 'warblade_session', value: 'alice',
      url: 'http://warblade.test', httpOnly: true }]);
    failDownload();
    await page.evaluate(() => restoreSession());
    assert.equal(await page.evaluate(() => Module.accountName), '');
    assert.equal(await page.evaluate(() => accountReady), false);
    assert.deepEqual(await page.evaluate(() => FS.readdir('/save')), ['.', '..']);
    assert.equal(await page.evaluate(() => saved.size), 0);
    assert.match(await page.evaluate(() => Module.authError), /account|connection|load/i);
  } finally { await browser.close(); }
});
