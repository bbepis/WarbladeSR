// Real WASM checkpoint/resume path against a disposable in-memory account API.
// No account or save writes reach the configured game server.
'use strict';

const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const zlib = require('node:zlib');

const GAME_URL = process.env.WARBLADE_GAME_URL;
const USER = 'CheckpointTester';
const PASSWORD = 'checkpoint-test-password';
const PROFILE_SAVE = 'warblade/profiles/profile000.svg';
const PROFILE_ACCOUNT = 'warblade/profiles/profile000.acc';
const SAVEFILE_SIZE = 0xa9a18;
const SAVEFILE_MODE_OFFSET = 0xa9a00;
const SAVE_ID_OFFSET = 8;
const BOARD_SIZE = 0xa1558;

async function nativeKey(page, key) {
  await page.keyboard.down(key);
  await page.waitForTimeout(400);
  await page.keyboard.up(key);
  await page.waitForTimeout(400);
}

async function nativeType(page, text) {
  for (const key of text) await nativeKey(page, key);
}

function newMockApi() {
  const saves = new Map();
  const saveWrites = [];
  const requests = [];
  const board = Buffer.alloc(BOARD_SIZE);
  board.write('WARX', 0, 'ascii');
  const compressedBoard = zlib.deflateSync(board);
  let boardVersion = 0;
  let nextSaveVersion = 1;

  const response = (route, status, body = '', headers = {}) =>
    route.fulfill({ status, body, headers });
  const json = (route, status, value, headers = {}) =>
    response(route, status, JSON.stringify(value), {
      'Content-Type': 'application/json', ...headers,
    });

  async function handle(route) {
    const request = route.request();
    const url = new globalThis.URL(request.url());
    const method = request.method();
    const cookie = request.headers().cookie || '';
    const authenticated = cookie.includes('warblade_session=checkpoint-session');
    requests.push([method, url.pathname]);

    if (url.pathname === '/api/performance')
      return response(route, 204);
    if (url.pathname === '/api/me')
      return json(route, 200, { username: authenticated ? USER : null });
    if (url.pathname === '/api/register' || url.pathname === '/api/login') {
      if (method !== 'POST') return json(route, 405, { detail: 'Method not allowed' });
      const credentials = request.postDataJSON();
      if (credentials.username !== USER || credentials.password !== PASSWORD)
        return json(route, 401, { detail: 'Invalid credentials' });
      return json(route, 200, { username: USER }, {
        'Set-Cookie': 'warblade_session=checkpoint-session; Path=/; HttpOnly; SameSite=Lax; Max-Age=2592000',
      });
    }
    if (!authenticated) return json(route, 401, { detail: 'Not authenticated' });

    if (url.pathname === '/api/logout' && method === 'POST')
      return json(route, 200, {}, {
        'Set-Cookie': 'warblade_session=; Path=/; HttpOnly; SameSite=Lax; Max-Age=0',
      });
    if (url.pathname === '/api/saves' && method === 'GET') {
      return json(route, 200, { files: [...saves].map(([path, save]) =>
        ({ path, size: save.data.length, version: save.version })) });
    }
    if (url.pathname.startsWith('/api/saves/')) {
      const path = decodeURIComponent(url.pathname.slice('/api/saves/'.length));
      if (path === 'warblade/warblade_132.his')
        return json(route, 404, { detail: 'Use shared scores' });
      if (method === 'GET') {
        const save = saves.get(path);
        return save ? response(route, 200, save.data, {
          'Content-Type': 'application/octet-stream', ETag: `"${save.version}"`,
        }) : json(route, 404, { detail: 'Save not found' });
      }
      if (method === 'PUT') {
        const current = saves.get(path);
        const match = request.headers()['if-match'];
        if ((!current && match !== '*') || (current && match !== String(current.version)))
          return json(route, 412, { detail: 'Save revision conflict' });
        const data = request.postDataBuffer() || Buffer.alloc(0);
        const save = { data: Buffer.from(data), version: nextSaveVersion++ };
        saves.set(path, save);
        saveWrites.push({ path, data: Buffer.from(data), version: save.version });
        return json(route, 200, { version: save.version }, { ETag: `"${save.version}"` });
      }
      if (method === 'DELETE') {
        const current = saves.get(path);
        if (!current) return json(route, 404, { detail: 'Save not found' });
        if (request.headers()['if-match'] !== String(current.version))
          return json(route, 412, { detail: 'Save revision conflict' });
        saves.delete(path);
        return response(route, 204);
      }
      return json(route, 405, { detail: 'Method not allowed' });
    }
    if (url.pathname === '/api/hiscores' && method === 'GET')
      return response(route, 200, compressedBoard, {
        'Content-Type': 'application/octet-stream', ETag: `"${boardVersion}"`,
      });
    if (url.pathname === '/api/hiscores' && method === 'POST') {
      // This isolated fixture never publishes or persists scores.
      boardVersion++;
      return response(route, 200, compressedBoard, {
        'Content-Type': 'application/octet-stream', ETag: `"${boardVersion}"`,
      });
    }
    return json(route, 404, { detail: 'Unexpected API request' });
  }

  return { handle, saves, saveWrites, requests };
}

test('real WASM saves a shop checkpoint and resumes it after remembered-session reload', {
  skip: !GAME_URL,
  timeout: 300000,
}, async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  const mock = newMockApi();
  let page;
  try {
    const context = await browser.newContext({ viewport: { width: 800, height: 600 } });
    page = await context.newPage();
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.route('**/api/**', mock.handle);
    await page.goto(GAME_URL);
    await page.waitForFunction(() => gameStarted && Module._WebLoginReady() === 1,
      null, { timeout: 120000 });
    await page.locator('#loading').waitFor({ state: 'hidden' });

    assert.equal(await page.evaluate(({ name, password }) => Module.ccall(
      'WebQueueCredentials', 'number', ['string', 'string', 'number'],
      [name, password, 1]), { name: USER, password: PASSWORD }), 1);
    await page.waitForFunction(() => Module._WebAccountStatus() === 1,
      null, { timeout: 60000 });
    assert.equal(await page.evaluate(() => Module.accountName), USER);
    console.log('native account activated');
    await page.waitForFunction(() => FS.analyzePath('/save/warblade/profiles/profile000.acc').exists,
      null, { timeout: 30000 });

    const canvas = page.locator('#canvas');
    await canvas.focus();
    await nativeKey(page, 'Escape'); // close the initial profile panel
    await page.evaluate(() => {
      const sample = Module.onPerformanceFrame;
      Module.onPerformanceFrame = (phase, ...args) => { window.nativePhase = phase; return sample(phase, ...args); };
    });
    for (let attempt = 0; attempt < 20; attempt++) {
      await nativeKey(page, 'Escape');
      await nativeKey(page, 'F1');
      try { await page.waitForFunction(() => nativePhase === 1, null, { timeout: 1000 }); break; }
      catch (error) { if (attempt === 19) throw error; }
    }
    console.log('native game entered play');
    await page.waitForTimeout(4000); // let the native respawn/get-ready sequence settle
    await nativeType(page, 'galaga');
    await nativeKey(page, '9');      // stay alive while the test waits for warp/shop
    await nativeKey(page, '2');      // add money so the level routes through the shop
    await nativeKey(page, '3');      // existing warp/skip-level cheat
    console.log('warp cheat sent; awaiting real shop checkpoint');
    if (process.env.WARBLADE_CHECKPOINT_EVIDENCE) await page.screenshot({ path: process.env.WARBLADE_CHECKPOINT_EVIDENCE });

    await page.waitForFunction(() => FS.analyzePath('/save/warblade/profiles/profile000.svg').exists,
      null, { timeout: 120000 });
    await page.evaluate(() => persistSaves());
    assert.equal(await page.locator('#save-alert').isHidden(), true,
      await page.locator('#save-alert').textContent());
    assert.ok(mock.saves.has(PROFILE_SAVE), 'the real shop checkpoint reached the mock account API');
    assert.ok(mock.saves.has(PROFILE_ACCOUNT), 'the authenticated native profile reached the mock API');

    console.log('native shop checkpoint uploaded');
    const firstCheckpoint = Buffer.from(mock.saves.get(PROFILE_SAVE).data);
    const firstRaw = zlib.inflateSync(firstCheckpoint);
    assert.equal(firstRaw.length, SAVEFILE_SIZE, 'snapshot is a complete native savefile');
    assert.equal(firstRaw.subarray(0, 3).toString('ascii'), 'SDY');
    assert.equal(firstRaw.readInt32LE(SAVEFILE_MODE_OFFSET), 0, 'snapshot is one-player mode');
    const firstSaveId = firstRaw.readBigInt64LE(SAVE_ID_OFFSET);

    const loginCountBeforeReload = mock.requests.filter(([method, path]) =>
      method === 'POST' && (path === '/api/login' || path === '/api/register')).length;
    await page.reload();
    await page.waitForFunction(() => gameStarted && Module._WebGameReady() && Module._WebAccountStatus() === 1,
      null, { timeout: 120000 });
    assert.equal(await page.evaluate(() => Module.accountName), USER,
      'the existing HttpOnly session restores the disposable mock account');
    assert.equal(mock.requests.filter(([method, path]) =>
      method === 'POST' && (path === '/api/login' || path === '/api/register')).length,
    loginCountBeforeReload, 'reload restores the remembered session without another login');
    await page.waitForFunction(() => FS.analyzePath('/save/warblade/profiles/profile000.svg').exists,
      null, { timeout: 30000 });
    console.log('remembered session and checkpoint restored');
    const reloadedCheckpoint = await page.evaluate((path) =>
      [...FS.readFile(path)], '/save/warblade/profiles/profile000.svg');
    assert.deepEqual(Buffer.from(reloadedCheckpoint), firstCheckpoint,
      'reload downloaded the exact shop checkpoint from the mock account');

    await page.locator('#canvas').focus();
    await nativeKey(page, 'Escape'); // leave any attract-mode demo
    await page.waitForTimeout(1000);
    await nativeKey(page, 'a'); // open MY PROFILE
    await page.waitForTimeout(1200);
    if (process.env.WARBLADE_CHECKPOINT_EVIDENCE)
      await page.screenshot({ path: process.env.WARBLADE_CHECKPOINT_EVIDENCE });

    // ProfileWindow uses logical 800x600 coordinates, 550px wide and 366px tall for this
    // fresh low-rank account. CONTINUE GAME is at local (25, h-75).
    const bounds = await page.locator('#canvas').boundingBox();
    assert.ok(bounds, 'game canvas is visible');
    const logicalX = 125 + 25 + 60;
    const logicalY = (600 - 366) / 2 + (366 - 75) + 7;
    const scaleX = bounds.width / 800;
    const scaleY = bounds.height / 600;
    await page.mouse.move(bounds.x + logicalX * scaleX, bounds.y + logicalY * scaleY);
    await page.mouse.down();
    await page.waitForTimeout(500);
    await page.mouse.up();

    // LoadSuspended consumes the old file and the resumed shop immediately makes a fresh
    // checkpoint. Poll virtual FS so the assertion covers the native resume path itself.
    await page.waitForFunction((oldBytes) => {
      const path = '/save/warblade/profiles/profile000.svg';
      if (!FS.analyzePath(path).exists) return false;
      const current = FS.readFile(path);
      if (current.length !== oldBytes.length) return true;
      for (let i = 0; i < current.length; i++)
        if (current[i] !== oldBytes[i]) return true;
      return false;
    }, [...firstCheckpoint], { timeout: 60000, polling: 100 });
    console.log('native checkpoint resumed and shop re-saved');
    const resumedCheckpointBytes = await page.evaluate((path) =>
      [...FS.readFile(path)], '/save/warblade/profiles/profile000.svg');
    const resumedRaw = zlib.inflateSync(Buffer.from(resumedCheckpointBytes));
    assert.equal(resumedRaw.length, SAVEFILE_SIZE);
    assert.equal(resumedRaw.subarray(0, 3).toString('ascii'), 'SDY');
    assert.notEqual(resumedRaw.readBigInt64LE(SAVE_ID_OFFSET), firstSaveId,
      'the resumed shop wrote a fresh native checkpoint');
    await page.evaluate(() => persistSaves());
    assert.equal(await page.locator('#save-alert').isHidden(), true,
      await page.locator('#save-alert').textContent());
    assert.ok(mock.saveWrites.filter(write => write.path === PROFILE_SAVE).length >= 2,
      'both the original and resumed shop checkpoints were uploaded');

    // Sign-out is available from My Profile at the title, not from the shop.
    await page.reload();
    await page.waitForFunction(() => gameStarted && Module._WebGameReady && Module._WebGameReady() && Module._WebAccountStatus() === 1, null, { timeout: 120000 });
    await page.locator("#canvas").focus();
    await nativeKey(page, "Escape");
    // Sign-out must clear the same remembered cookie used by the reload path.
    const navigation = page.waitForNavigation();
    await page.evaluate(() => Module._WebQueueSignOut());
    await navigation;
    assert.equal((await context.cookies()).some(cookie => cookie.name === 'warblade_session'), false);
    await page.waitForFunction(() => gameStarted && Module._WebAccountStatus() === 0,
      null, { timeout: 120000 });
    assert.ok(mock.requests.some(([method, path]) => method === 'POST' && path === '/api/logout'));
    assert.deepEqual(errors, []);
    await context.close();
  } catch (error) {
    if (page && process.env.WARBLADE_CHECKPOINT_EVIDENCE) await page.screenshot({ path: process.env.WARBLADE_CHECKPOINT_EVIDENCE });
    console.log('mock API calls', mock.requests);
    if (page) console.log('native state', await page.evaluate(() => ({name:Module.accountName,error:Module.authError,status:Module._WebAccountStatus(),login:Module._WebLoginReady(),files:saveFiles()})));
    throw error;
  } finally {
    await browser.close();
  }
});
