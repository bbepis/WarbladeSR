// Guest play is a fresh, page-memory-only profile. Real-game checks never write accounts.
'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const fs = require('node:fs');
const path = require('node:path');
const { inflateSync } = require('node:zlib');

async function bridgePage(browser) {
  const context = await browser.newContext({ isMobile: true, hasTouch: true });
  const page = await context.newPage();
  const requests = [];
  await page.route('http://warblade.test/**', route => {
    const url = new URL(route.request().url());
    if (url.pathname.startsWith('/api/')) {
      requests.push(url.pathname);
      return route.abort();
    }
    if (url.pathname === '/loading-title.jpg') return route.fulfill({ status: 404, body: '' });
    if (url.pathname === '/warblade.js') return route.fulfill({ body: '' });
    const file = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
    return route.fulfill({ contentType: file.endsWith('.js') ? 'text/javascript' :
      file.endsWith('.css') ? 'text/css' : 'text/html',
      body: fs.readFileSync(path.join(__dirname, '../web', file)) });
  });
  await page.goto('http://warblade.test/');
  await page.evaluate(() => {
    // Keep an actual hierarchical RAM filesystem so deletion and later writes are exercised.
    const entries = new Map([['/save', null], ['/save/warblade', null],
      ['/save/warblade/profiles', null],
      ['/save/warblade/profiles/profile000.acc', new Uint8Array([1, 2])],
      ['/save/warblade/warblade_132.his', new Uint8Array([3, 4])]]);
    window.FS = {
      stat(p) { if (!entries.has(p)) throw Error('missing'); return { mode: entries.get(p) === null ? 1 : 0 }; },
      isDir: mode => mode === 1,
      readdir(p) { return ['.', '..', ...[...entries.keys()].filter(k =>
        k.startsWith(p + '/') && !k.slice(p.length + 1).includes('/')).map(k => k.slice(p.length + 1))]; },
      mkdir: p => entries.set(p, null),
      rmdir: p => entries.delete(p),
      unlink: p => entries.delete(p),
      writeFile: (p, value) => entries.set(p, new Uint8Array(value)),
      readFile: p => entries.get(p),
    };
    runtimeReady = true;
    document.getElementById('loading').hidden = true;
    Module._WebLoginReady = () => 1;
    Module.syncLoginInputs({ x: 200, y: 150, width: 400,
      name: 'temporary', password: 'temporary password', focus: 1,
      screenW: 800, screenH: 600 });
    saved.set('warblade/profiles/profile000.acc', { bytes: new Uint8Array([1, 2]), version: 7 });
    scoreBase = new Uint8Array([3, 4]);
    scoreVersion = '8';
    scoreReloadPending = true;
    saveConflict = true;
    Module.authError = 'Previous error';
  });
  return { context, page, requests };
}

test('guest bridge clears temporary saves and credentials without server or browser persistence', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { context, page, requests } = await bridgePage(browser);
    assert.equal(await page.evaluate(() => Module.beginGuestGame()), true);
    assert.deepEqual(await page.evaluate(() => ({ files: FS.readdir('/save'), saved: saved.size,
      scoreBase, scoreVersion, accountReady, name: Module.accountName,
      error: Module.authError, conflict: saveConflict,
      nameInput: document.getElementById('login-name').value,
      passwordInput: document.getElementById('login-password').value })), {
      files: ['.', '..'], saved: 0, scoreBase: null, scoreVersion: null,
      accountReady: false, name: 'GUEST', error: '', conflict: false,
      nameInput: '', passwordInput: '',
    });
    assert.equal(await page.locator('#login-password').isVisible(), false);
    await page.evaluate(async () => {
      mkdirs('/save/warblade/profiles');
      FS.writeFile('/save/warblade/profiles/profile000.acc', [9, 10]);
      FS.writeFile(SCORE_FILE, [11, 12]);
      Module.onGameScoreWritten();
      await new Promise(resolve => setTimeout(resolve, 0));
      await persistSaves();
      await persistSaves(true);
    });
    assert.deepEqual(requests, [], 'guest profile and scores must never reach the server');
    assert.deepEqual(await page.evaluate(async () => ({ databases: await indexedDB.databases(),
      local: localStorage.length, session: sessionStorage.length })),
    { databases: [], local: 0, session: 0 });
    assert.deepEqual(await context.cookies(), []);
    await page.reload();
    assert.equal(await page.evaluate(() => accountReady), false);
    assert.equal(await page.evaluate(() => Module.accountName), '');
  } finally { await browser.close(); }
});

test('guest request cannot erase an authenticated account or race sign-in', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, requests } = await bridgePage(browser);
    for (const state of ['accountReady', 'authBusy']) {
      await page.evaluate(state => {
        accountReady = state === 'accountReady';
        authBusy = state === 'authBusy';
      }, state);
      assert.equal(await page.evaluate(() => Module.beginGuestGame()), false, state);
      assert.deepEqual(await page.evaluate(() => [...FS.readFile('/save/warblade/profiles/profile000.acc')]), [1, 2]);
      assert.equal(await page.evaluate(() => saved.size), 1);
      assert.equal(await page.evaluate(() => scoreVersion), '8');
    }
    assert.deepEqual(requests, []);
  } finally { await browser.close(); }
});

test('mobile guest button receives trusted tap, queues once and clears focused credentials', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, requests } = await bridgePage(browser);
    await page.evaluate(() => {
      window.guestClicks = 0;
      Module._WebPlayGuest = () => { guestClicks++; return 1; };
    });
    const guest = page.locator('#login-guest');
    assert.equal(await guest.isVisible(), true);
    assert.equal(await guest.getAttribute('type'), 'button');
    assert.equal(await guest.getAttribute('aria-label'), 'Play as guest');
    await page.locator('#login-password').focus();
    await guest.evaluate(button => button.addEventListener('click', event => {
      window.trustedGuestClick = event.isTrusted;
    }));
    const bounds = await guest.boundingBox();
    await page.touchscreen.tap(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
    assert.equal(await page.evaluate(() => trustedGuestClick), true);
    assert.equal(await page.evaluate(() => guestClicks), 1);
    assert.equal(await guest.isVisible(), false);
    for (const id of ['login-name', 'login-password']) {
      assert.equal(await page.locator('#' + id).isVisible(), false);
      assert.equal(await page.locator('#' + id).inputValue(), '');
    }
    assert.notEqual(await page.evaluate(() => document.activeElement.id), 'login-password');
    assert.deepEqual(requests, []);
  } finally { await browser.close(); }
});

test('signing in from guest discards guest files before loading private account saves', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page } = await bridgePage(browser);
    assert.equal(await page.evaluate(() => Module.beginGuestGame()), true);
    await page.evaluate(() => {
      mkdirs('/save/warblade/profiles');
      FS.writeFile('/save/warblade/profiles/profile000.acc', [99]);
      FS.writeFile('/save/warblade/guest-only.txt', [42]);
    });
    const calls = [];
    await page.route('http://warblade.test/api/**', route => {
      const request = route.request();
      const pathname = new URL(request.url()).pathname;
      calls.push([request.method(), pathname]);
      if (pathname === '/api/login') return route.fulfill({ json: {
        username: 'TestPlayer',
      } });
      if (pathname === '/api/saves') return route.fulfill({ json: { files: [
        { path: 'warblade/profiles/profile000.acc', version: 3 },
      ] } });
      if (pathname === '/api/saves/warblade/profiles/profile000.acc')
        return route.fulfill({ body: Buffer.from([7, 8]) });
      if (pathname === '/api/hiscores') return route.fulfill({ headers: { ETag: '"9"' },
        body: Buffer.from([5, 6]) });
      return route.abort();
    });
    assert.equal(await page.evaluate(() => Module.authenticateGame('TestPlayer', '12345678', false)), true);
    assert.deepEqual(await page.evaluate(() => ({ ready: accountReady, name: Module.accountName,
      guestFile: exists('/save/warblade/guest-only.txt'),
      profile: [...FS.readFile('/save/warblade/profiles/profile000.acc')],
      savedVersion: saved.get('warblade/profiles/profile000.acc').version })),
    { ready: true, name: 'TestPlayer', guestFile: false, profile: [7, 8], savedVersion: 3 });
    assert.deepEqual(calls, [['POST', '/api/login'], ['GET', '/api/saves'],
      ['GET', '/api/saves/warblade/profiles/profile000.acc'], ['GET', '/api/hiscores']]);
  } finally { await browser.close(); }
});

for (const mobile of [false, true]) test(`real ${mobile ? 'mobile touch' : 'desktop native'} guest button starts a fresh profile and reload discards it`,
  { skip: !process.env.WARBLADE_GAME_URL, timeout: 300000 }, async () => {
    const browser = await chromium.launch({ args: ['--no-sandbox'] });
    try {
      const context = await browser.newContext({ viewport: mobile ? { width: 375, height: 812 } :
        { width: 800, height: 600 }, isMobile: mobile, hasTouch: mobile });
      const page = await context.newPage();
      const errors = [], requests = [];
      page.on('pageerror', error => errors.push(error.message));
      await page.route('**/api/**', route => {
        if (new URL(route.request().url()).pathname === '/api/me')
          return route.fulfill({ json: { username: null } });
        requests.push(route.request().url()); return route.abort();
      });
      await page.goto(process.env.WARBLADE_GAME_URL);
      const waitForLogin = async () => {
        await page.waitForFunction(() => gameStarted && Module._WebLoginReady() === 1,
          null, { timeout: 120000 });
        await page.locator('#loading').waitFor({ state: 'hidden' });
        await page.waitForTimeout(1500);
      };
      await waitForLogin();
      if (mobile) {
        const guest = page.locator('#login-guest');
        await guest.waitFor({ state: 'visible' });
        const bounds = await guest.boundingBox();
        await page.touchscreen.tap(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
      } else {
        const canvas = await page.locator('#canvas').boundingBox();
        await page.mouse.move(canvas.x + canvas.width * 160 / 800,
          canvas.y + canvas.height * 392 / 600);
        await page.mouse.down();
        await page.waitForTimeout(150);
        await page.mouse.up();
      }
      await page.waitForFunction(() => Module._WebIsGuest() === 1, null, { timeout: 15000 });
      assert.equal(await page.evaluate(() => Module._WebCanPlay()), 1);
      assert.equal(await page.evaluate(() => Module._WebAccountStatus()), 0);
      assert.equal(await page.evaluate(() => Module._WebLoginReady()), 0);
      assert.equal(await page.locator('#login-name').isVisible(), false);
      assert.equal(await page.locator('#login-password').isVisible(), false);
      // Account management belongs to the title menu. Leave the guest game
      // through its normal quit-current-game confirmation before opening it.
      await page.locator('#canvas').focus();
      await page.keyboard.down('Escape');
      await page.waitForTimeout(150);
      await page.keyboard.up('Escape');
      await page.waitForTimeout(1000);
      await page.keyboard.down('Enter');
      await page.waitForTimeout(150);
      await page.keyboard.up('Enter');
      await page.waitForTimeout(1500);
      // Returning to guest after opening sign-in keeps progress from this visit.
      await page.evaluate(() => {
        FS.writeFile('/save/warblade/current-visit.txt', new Uint8Array([21]));
        Module._WebOpenLogin(0);
      });
      await page.waitForFunction(() => Module._WebLoginReady() === 1);
      await page.evaluate(() => Module._WebPlayGuest());
      await page.waitForFunction(() => Module._WebLoginReady() === 0 && Module._WebIsGuest() === 1);
      assert.deepEqual(await page.evaluate(() => [...FS.readFile('/save/warblade/current-visit.txt')]), [21]);
      await page.locator('#canvas').focus();
      await page.keyboard.down('F1');
      await page.waitForTimeout(150);
      await page.keyboard.up('F1');
      await page.waitForTimeout(1500);
      assert.equal(await page.evaluate(() => Module._WebCanPlay()), 1);
      assert.equal(await page.evaluate(() => Module._WebLoginReady()), 0);
      const profile = await page.evaluate(async () => {
        Module._WebSaveSettings();
        FS.writeFile('/save/warblade/guest-discard.txt', new Uint8Array([42]));
        Module.onGameScoreWritten();
        await new Promise(resolve => setTimeout(resolve, 0));
        await persistSaves();
        await persistSaves(true);
        return { names: FS.readdir('/save/warblade/profiles').filter(n => /\.acc$/.test(n)),
          bytes: [...FS.readFile('/save/warblade/profiles/profile000.acc')] };
      });
      assert.deepEqual(profile.names, ['profile000.acc']);
      const raw = inflateSync(Buffer.from(profile.bytes));
      assert.equal(raw.subarray(7, 37).toString().split('\0')[0], 'GUEST');
      assert.equal(raw.subarray(37, 53).every(value => value === 0), true);
      assert.deepEqual(requests, [], 'guest must not call any account API');
      assert.deepEqual(await context.cookies(), []);
      assert.deepEqual(await page.evaluate(async () => ({ databases: await indexedDB.databases(),
        local: localStorage.length, session: sessionStorage.length })),
      { databases: [], local: 0, session: 0 });
      await page.reload();
      await waitForLogin();
      assert.equal(await page.evaluate(() => Module._WebIsGuest()), 0);
      assert.equal(await page.evaluate(() => Module._WebCanPlay()), 0);
      assert.equal(await page.evaluate(() => exists('/save/warblade/guest-discard.txt')), false);
      assert.equal(await page.evaluate(() => exists('/save/warblade/current-visit.txt')), false);
      assert.deepEqual(requests, []);
      assert.deepEqual(errors, []);
    } finally { await browser.close(); }
  });
