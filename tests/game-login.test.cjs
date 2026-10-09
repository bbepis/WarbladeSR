// Real game checks: isolated API state required; production assets are read-only.
'use strict';
const {test} = require('node:test');
const assert = require('node:assert/strict');
const {chromium} = require('playwright');
const URL = process.env.WARBLADE_GAME_URL;
const secret = 'Serverpass12345!_';

async function openGame(browser) {
  const context = await browser.newContext();
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.goto(URL);
  assert.equal(await page.locator('#auth').count(), 0);
  assert.equal(await page.locator('#play').count(), 0);
  await page.waitForFunction(() => gameStarted, null, {timeout:120000});
  await page.waitForFunction(() => Module._WebLoginReady() === 1, null, {timeout:90000});
  await page.locator('#loading').waitFor({state:'hidden'});
  assert.equal(await page.evaluate(() => Module._WebAccountStatus()), 0);
  return {context,page,errors};
}

// Native input polls held keys once per game frame. Keep presses and releases
// long enough for the loop to observe both, as a physical keyboard does.
async function nativeKey(page, key) {
  const shifted = key === key.toUpperCase() && /^[A-Z]$/.test(key);
  if (shifted) await page.keyboard.down('Shift');
  await page.keyboard.down(key);
  await page.waitForTimeout(100);
  await page.keyboard.up(key);
  if (shifted) await page.keyboard.up('Shift');
  await page.waitForTimeout(100);
}
async function nativeType(page, value) {
  for (const key of value) await nativeKey(page, key);
}

async function signIn(page, name, create) {
  assert.equal(await page.evaluate(({name,create,secret}) => Module.ccall('WebQueueCredentials', 'number',
    ['string','string','number'], [name,secret,create ? 1:0]), {name,create,secret}), 1);
  await page.waitForFunction(() => Module._WebAccountStatus() === 1, null, {timeout:60000});
}

test('native login/create, one server profile, reload persistence and account isolation', {skip:!URL,timeout:300000}, async () => {
  const browser = await chromium.launch({args:['--no-sandbox']});
  try {
    const alice = await openGame(browser);
    // Exercise native field focus, typing and Enter, including visible validation.
    await alice.page.evaluate(() => Module._WebOpenLogin(1));
    await alice.page.waitForTimeout(300);
    await alice.page.locator('#canvas').focus();
    await nativeType(alice.page, 'webtester');
    await nativeKey(alice.page, 'Tab');
    await nativeType(alice.page, 'short');
    await nativeKey(alice.page, 'Enter');
    await alice.page.waitForFunction(() => Module.authError.includes('8-256'), null,{timeout:10000});
    await alice.page.waitForFunction(() => Module._WebLoginReady() === 1);
    await nativeKey(alice.page, 'Tab');
    await nativeType(alice.page, secret);
    await nativeKey(alice.page, 'Enter');
    await alice.page.waitForFunction(() => Module._WebAccountStatus() === 1, null,{timeout:60000});
    assert.equal(await alice.page.evaluate(() => Module.accountName), 'webtester');
    // The welcome sound must not invalidate the authenticated account.
    await alice.page.waitForTimeout(1500);
    assert.equal(await alice.page.evaluate(() => Module._WebAccountStatus()), 1);
    await nativeKey(alice.page, 'F1');
    await alice.page.waitForTimeout(1500);
    assert.equal(await alice.page.evaluate(() => Module._WebAccountStatus()), 1);
    assert.equal(await alice.page.evaluate(() => Module._WebLoginReady()), 0);
    await alice.page.evaluate(() => { Module._WebSaveSettings(); });
    await alice.page.evaluate(() => persistSaves());
    assert.equal(await alice.page.locator('#save-alert').isHidden(), true);
    const before = await alice.page.evaluate(async () => {
      const files = (await (await accountFetch('/api/saves')).json()).files;
      return {profiles:files.filter(f => /^warblade\/profiles\/profile\d+\.acc$/.test(f.path)).length,
        data:[...FS.readFile('/save/warblade/profiles/profile000.acc')]};
    });
    assert.equal(before.profiles, 1);
    assert.equal((await alice.context.cookies()).length, 0);
    assert.deepEqual(await alice.page.evaluate(async () => ({db:await indexedDB.databases(),
      local:localStorage.length,session:sessionStorage.length})), {db:[],local:0,session:0});
    const raw = require('node:zlib').inflateSync(Buffer.from(before.data));
    assert.equal(raw.subarray(7,37).toString().split('\0')[0], 'webtester');
    assert.equal(raw.subarray(37,53).every(x=>x===0), true, 'no separate profile password');
    // A marker save proves server persistence across page-memory loss.
    await alice.page.evaluate(() => { FS.writeFile('/save/warblade/persistence-check.txt','persisted'); });
    await alice.page.evaluate(() => persistSaves());
    await alice.page.reload();
    await alice.page.waitForFunction(() => gameStarted, null, {timeout:120000});
    await alice.page.waitForFunction(() => Module._WebLoginReady() === 1,null,{timeout:90000});
    assert.equal(await alice.page.evaluate(() => Module._WebAccountStatus()), 0);
    await signIn(alice.page,'webtester',false);
    assert.equal(await alice.page.evaluate(() => FS.readFile('/save/warblade/persistence-check.txt',{encoding:'utf8'})), 'persisted');
    const bob = await openGame(browser);
    await signIn(bob.page,'otherplayer',true);
    assert.equal(await bob.page.evaluate(async () => (await accountFetch('/api/saves/warblade/persistence-check.txt')).status), 404);
    // Native signout uses its queued main-loop bridge, then reloads to fresh login.
    await Promise.all([alice.page.waitForEvent('framenavigated'),
      alice.page.evaluate(() => Module._WebQueueSignOut())]);
    await alice.page.waitForFunction(() => gameStarted, null, {timeout:120000});
    assert.equal(await alice.page.evaluate(() => sessionToken), '');
    assert.deepEqual(alice.errors, []);
    assert.deepEqual(bob.errors, []);
  } finally { await browser.close(); }
});
