// Real browser inputs over the native login window, with no account/API writes.
// Touch emulation checks trusted focus and editing; it cannot display an OS keyboard.
'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const fs = require('node:fs');
const path = require('node:path');

async function loginPage(browser, mobile) {
  const context = await browser.newContext({ viewport: { width: 800, height: 600 },
    isMobile: mobile, hasTouch: mobile });
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.route('http://warblade.test/**', route => {
    const url = new URL(route.request().url());
    if (url.pathname === '/loading-title.jpg') return route.fulfill({ status: 404, body: '' });
    if (url.pathname === '/warblade.js') return route.fulfill({ body: '' });
    assert.ok(!url.pathname.startsWith('/api/'), 'editing must not contact the account server');
    const file = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
    return route.fulfill({ contentType: file.endsWith('.css') ? 'text/css' :
      file.endsWith('.js') ? 'text/javascript' : 'text/html',
      body: fs.readFileSync(path.join(__dirname, '../web', file)) });
  });
  await page.goto('http://warblade.test/');
  await page.evaluate(() => {
    runtimeReady = true;
    document.getElementById('loading').hidden = true;
    window.nativeLogin = { name: '', password: '', focus: 0, calls: [], submits: 0 };
    Module._WebLoginReady = () => 1;
    Module.ccall = (name, result, types, values = []) => {
      nativeLogin.calls.push({ name, values });
      if (name === 'WebSetLoginField') {
        nativeLogin[values[0] === 0 ? 'name' : 'password'] = values[1];
        nativeLogin.focus = values[0];
        return 1;
      }
      if (name === 'WebSubmitLogin') { nativeLogin.submits++; return 1; }
      // The old key bridge would duplicate characters typed into a real input.
      if (name === 'WebInsertLoginText') return 1;
      return 1;
    };
    window.updateLoginWindow = () => Module.syncLoginInputs({ x: 200, y: 150,
      width: 400, name: nativeLogin.name, password: nativeLogin.password,
      focus: nativeLogin.focus, screenW: 800, screenH: 600 });
    updateLoginWindow();
  });
  return { context, page, errors };
}

test('touch login fields receive trusted focus, edit whole values and submit once', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, errors } = await loginPage(browser, true);
    const name = page.locator('#login-name');
    const password = page.locator('#login-password');
    assert.equal(await name.isVisible(), true);
    assert.equal(await password.isVisible(), true);
    for (const [input, type, max, label] of [[name, 'text', '32', 'Player name'],
      [password, 'password', '256', 'Password']]) {
      assert.equal(await input.getAttribute('type'), type);
      assert.equal(await input.getAttribute('maxlength'), max);
      assert.equal(await input.getAttribute('aria-label'), label);
      assert.equal(await input.getAttribute('autocomplete'), 'off');
      assert.equal(await input.getAttribute('autocapitalize'), 'none');
    }
    const canvas = await page.locator('#canvas').boundingBox();
    const bounds = await name.boundingBox();
    assert.ok(Math.abs(bounds.x - (canvas.x + canvas.width * 200 / 800)) < 2);
    assert.ok(Math.abs(bounds.y - (canvas.y + canvas.height * 150 / 600)) < 2);
    assert.ok(Math.abs(bounds.width - canvas.width * 400 / 800) < 2);
    const passwordBounds = await password.boundingBox();
    assert.ok(Math.abs(passwordBounds.y - bounds.y - canvas.height * 25 / 600) < 2);

    await page.evaluate(() => {
      document.getElementById('login-name').addEventListener('focus', event => {
        window.trustedNameFocus = event.isTrusted;
      });
    });
    await page.touchscreen.tap(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
    assert.equal(await page.evaluate(() => document.activeElement.id), 'login-name');
    assert.equal(await page.evaluate(() => trustedNameFocus), true);
    await page.keyboard.type('RkJ._-');
    assert.equal(await name.inputValue(), 'RkJ._-');
    assert.equal(await page.evaluate(() => nativeLogin.name), 'RkJ._-');
    await page.evaluate(() => updateLoginWindow());
    assert.equal(await name.inputValue(), 'RkJ._-');
    await page.setViewportSize({ width: 640, height: 480 });
    await page.evaluate(() => new Promise(resolve => requestAnimationFrame(resolve)));
    const resizedCanvas = await page.locator('#canvas').boundingBox();
    const resizedName = await name.boundingBox();
    assert.ok(Math.abs(resizedName.x - (resizedCanvas.x + resizedCanvas.width * 200 / 800)) < 2);
    assert.ok(Math.abs(resizedName.y - (resizedCanvas.y + resizedCanvas.height * 150 / 600)) < 2);
    assert.equal(await page.evaluate(() => document.activeElement.id), 'login-name');
    assert.equal(await name.inputValue(), 'RkJ._-');
    await name.press('Enter');
    assert.equal(await page.evaluate(() => document.activeElement.id), 'login-password');
    await page.keyboard.type('MiXeD!12');
    assert.equal(await page.evaluate(() => nativeLogin.password), 'MiXeD!12');

    // InputEvent is the path used by paste, autofill and virtual keyboards;
    // these edits need whole-value replacement rather than appending characters.
    await password.evaluate(input => {
      input.value = 'PaStEd!34';
      input.dispatchEvent(new InputEvent('input', { bubbles: true,
        inputType: 'insertFromPaste', data: 'PaStEd!34' }));
    });
    assert.equal(await page.evaluate(() => nativeLogin.password), 'PaStEd!34');
    await password.press('ControlOrMeta+A');
    await page.keyboard.type('New#5678');
    assert.equal(await page.evaluate(() => nativeLogin.password), 'New#5678');
    await password.press('Backspace');
    assert.equal(await page.evaluate(() => nativeLogin.password), 'New#567');
    await password.press('Enter');
    assert.equal(await page.evaluate(() => nativeLogin.submits), 1);
    assert.equal(await page.evaluate(() => nativeLogin.calls.some(call =>
      call.name === 'WebInsertLoginText')), false, 'DOM text must bypass the legacy key bridge');

    await page.evaluate(() => Module.syncLoginInputs(null));
    assert.equal(await name.isVisible(), false);
    assert.equal(await password.isVisible(), false);
    assert.equal(await name.inputValue(), '');
    assert.equal(await password.inputValue(), '');
    assert.notEqual(await page.evaluate(() => document.activeElement.id), 'login-password');
    assert.deepEqual(await page.evaluate(() => ({ local: localStorage.length,
      session: sessionStorage.length })), { local: 0, session: 0 });
    assert.deepEqual(errors, [], 'no runtime errors while opening or resizing login');
  } finally { await browser.close(); }
});

test('desktop retains native keyboard input without visible touch login fields', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, errors } = await loginPage(browser, false);
    assert.equal(await page.locator('#login-name').isVisible(), false);
    assert.equal(await page.locator('#login-password').isVisible(), false);
    await page.locator('#canvas').focus();
    await page.keyboard.type('RkJ');
    assert.deepEqual(await page.evaluate(() => nativeLogin.calls.filter(call =>
      call.name === 'WebInsertLoginText').map(call => call.values[0])), ['R', 'k', 'J']);
    assert.equal(await page.evaluate(() => nativeLogin.submits), 0);
    assert.deepEqual(errors, [], 'no desktop runtime errors');
  } finally { await browser.close(); }
});

test('real mobile game accepts DOM credentials and stays signed in when starting',
  { skip: !process.env.WARBLADE_GAME_URL }, async () => {
    const browser = await chromium.launch({ args: ['--no-sandbox'] });
    try {
      const context = await browser.newContext({ viewport: { width: 375, height: 812 },
        isMobile: true, hasTouch: true });
      const page = await context.newPage();
      const errors = [];
      const accountWrites = [];
      page.on('pageerror', error => errors.push(error.message));
      // This exercises the actual game and its in-memory profile with fixture
      // credentials. No account or save may be written to the server.
      await page.route('**/api/**', route => {
        if (route.request().method() !== 'GET') {
          accountWrites.push(route.request().method());
          return route.abort();
        }
        return route.continue();
      });
      await page.goto(process.env.WARBLADE_GAME_URL);
      await page.waitForFunction(() => window.Module && Module._WebLoginReady &&
        Module._WebLoginReady() === 1, undefined, { timeout: 120000 });
      const name = page.locator('#login-name');
      const password = page.locator('#login-password');
      await name.waitFor({ state: 'visible', timeout: 10000 });
      // Native windows slide into view. A visible DOM field can still sit
      // outside the canvas during that animation and cannot yet be tapped.
      await page.waitForFunction(() => {
        const input = document.getElementById('login-name').getBoundingClientRect();
        const canvas = document.getElementById('canvas').getBoundingClientRect();
        return input.width > 0 && input.left >= canvas.left &&
          input.right <= canvas.right && input.top >= canvas.top &&
          input.bottom <= canvas.bottom;
      }, undefined, { timeout: 10000 });
      await page.evaluate(() => {
        window.fixtureCredentials = [];
        Module.authenticateGame = async (name, password, create) => {
          fixtureCredentials.push({ name, password, create });
          Module.accountName = name;
          Module.authError = '';
          return true;
        };
      });
      const nameBounds = await name.boundingBox();
      await page.touchscreen.tap(nameBounds.x + nameBounds.width / 2,
        nameBounds.y + nameBounds.height / 2);
      assert.equal(await page.evaluate(() => document.activeElement.id), 'login-name');
      await page.keyboard.type('mobilecheck');
      await name.press('Enter');
      assert.equal(await page.evaluate(() => document.activeElement.id), 'login-password');
      await page.keyboard.type('Eight123!_');
      assert.equal(await password.inputValue(), 'Eight123!_');
      await password.press('Backspace');
      assert.equal(await password.inputValue(), 'Eight123!');
      await page.keyboard.type('_');
      await password.press('Enter');
      await page.waitForFunction(() => Module._WebAccountStatus() === 1,
        undefined, { timeout: 15000 });
      assert.deepEqual(await page.evaluate(() => fixtureCredentials.map(credentials =>
        ({ name: credentials.name, password: credentials.password }))),
      [{ name: 'mobilecheck', password: 'Eight123!_' }]);
      assert.equal(await password.isVisible(), false);
      assert.equal(await password.inputValue(), '');
      assert.equal(await page.evaluate(() => accountReady), false,
        'fixture session must never enable production save synchronization');
      await page.keyboard.down('F1');
      await page.waitForTimeout(150);
      await page.keyboard.up('F1');
      await page.waitForTimeout(1000);
      assert.equal(await page.evaluate(() => Module._WebAccountStatus()), 1);
      assert.equal(await page.evaluate(() => Module._WebLoginReady()), 0);
      assert.equal(await name.isVisible(), false);
      assert.equal(await page.evaluate(() => fixtureCredentials.length), 1);
      assert.deepEqual(accountWrites, [], 'no account or progress writes');
      assert.deepEqual(errors, [], 'no runtime errors in actual mobile game');
    } finally { await browser.close(); }
  });
