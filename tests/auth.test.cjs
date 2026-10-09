// Browser bridge checks with isolated API routes; never uses production accounts.
'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const fs = require('node:fs');
const path = require('node:path');

test('native login bridge validates and authenticates without storing game data in the browser', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const context = await browser.newContext();
    const page = await context.newPage();
    let status = 409, calls = 0, networkDown = false;
    await page.route('http://warblade.test/**', async route => {
      const url = new URL(route.request().url());
      if (url.pathname.startsWith('/api/')) {
        calls++;
        if (networkDown) return route.abort('failed');
        await new Promise(resolve => setTimeout(resolve, 30));
        return route.fulfill({ status, contentType: 'application/json',
          body: status === 200 ? JSON.stringify({username: 'TestPlayer'}) : '{}' });
      }
      if (url.pathname === '/loading-title.jpg') return route.fulfill({status:404, body:''});
      if (url.pathname === '/warblade.js') return route.fulfill({ body: '' });
      const file = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
      return route.fulfill({ contentType: file.endsWith('.css') ? 'text/css' : file.endsWith('.js') ? 'text/javascript' : 'text/html',
        body: fs.readFileSync(path.join(__dirname, '../web', file)) });
    });
    await page.goto('http://warblade.test/');
    assert.equal(await page.locator('#auth').count(), 0, 'no outer login form');
    assert.equal(await page.locator('#logout').count(), 0, 'signout belongs to native profile UI');
    await page.evaluate(() => {
      FS = { readdir: () => ['.', '..'], stat: () => ({}), mkdir: () => {} };
      loadSaves = async () => { accountReady = true; };
      loadScores = async () => {};
    });
    assert.equal(await page.evaluate(() => Module.authenticateGame('x','short',true)), false);
    assert.match(await page.evaluate(() => Module.authError), /Username: 3-32/);
    assert.equal(await page.evaluate(() => Module.authenticateGame('TestPlayer','1234567',true)), false);
    assert.match(await page.evaluate(() => Module.authError), /Password must have 8-256/);
    assert.equal(calls, 0);
    // Eight characters reaches authentication rather than local validation.
    assert.equal(await page.evaluate(() => Module.authenticateGame('TestPlayer','12345678',true)), false);
    assert.match(await page.evaluate(() => Module.authError), /already taken/);
    assert.equal(calls, 1);
    for (const [code, expected] of [[409,/already taken/], [400,/requirements/],
      [429,/15 minutes/], [403,/unavailable/], [500,/try again shortly/], [401,/Incorrect/]]) {
      status = code;
      assert.equal(await page.evaluate(() => Module.authenticateGame('TestPlayer','test fixture password',true)), false);
      assert.match(await page.evaluate(() => Module.authError), expected);
    }
    networkDown = true;
    assert.equal(await page.evaluate(() => Module.authenticateGame('TestPlayer','test fixture password',false)), false);
    assert.match(await page.evaluate(() => Module.authError), /connection/);
    networkDown = false; status = 200;
    const before = calls;
    assert.deepEqual(await page.evaluate(async () => Promise.all([
      Module.authenticateGame('TestPlayer','test fixture password',true),
      Module.authenticateGame('TestPlayer','test fixture password',true),
    ])), [true,false]);
    assert.equal(calls, before + 1);
    assert.equal(await page.evaluate(() => Module.accountName), 'TestPlayer');
    assert.equal((await context.cookies()).length, 0);
    assert.deepEqual(await page.evaluate(async () => ({databases:await indexedDB.databases(),
      local:localStorage.length, session:sessionStorage.length})), {databases:[],local:0,session:0});
    await page.reload();
    assert.equal(await page.evaluate(() => accountReady), false);
    assert.equal(await page.evaluate(() => Module.accountName), '');
  } finally { await browser.close(); }
});
