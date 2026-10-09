// Browser checks run through the same-origin proxy and real account API.
'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const zlib = require('node:zlib');

const URL = process.env.WARBLADE_STUB_URL || 'http://127.0.0.1:18085';
const PROFILE = 'warblade/profiles/profile000.acc';
const SCORE = '/save/warblade/warblade_132.his';

async function account(browser, name) {
  const context = await browser.newContext();
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto(URL);
  await page.waitForFunction(() => gameStarted);
  await page.evaluate((name) => Module.ccall('WebQueueCredentials', 'number', ['string','string','number'],
    [name, 'long password 123', 1]), name);
  await page.waitForFunction(() => Module._WebAccountStatus() === 1);
  return { context, page, errors };
}

async function play(page, runs) {
  await page.waitForFunction(() => FS.analyzePath('/save/warblade/profiles/profile000.acc').exists);
  assert.equal(await page.evaluate(() => FS.readFile('/save/warblade/profiles/profile000.acc', {encoding:'utf8'})), runs + '\n');
  await page.evaluate(() => persistSaves());
  assert.equal(await page.locator('#save-alert').isHidden(), true);
}

test('server saves and login survive reload without browser game storage', async () => {
  const browser = await chromium.launch();
  try {
    const alice = await account(browser, 'BrowserAlice');
    await play(alice.page, 1);
    const saved = await alice.page.evaluate(async (path) => {
      const response = await accountFetch('/api/saves/' + path);
      return { status: response.status, body: await response.text() };
    }, PROFILE);
    assert.deepEqual(saved, { status: 200, body: '1\n' });
    assert.notEqual(await alice.page.evaluate(() => FS.lookupPath('/save').node.mount.mountpoint), '/save');
    assert.equal(await alice.page.evaluate(async () =>
      (await indexedDB.databases()).some((db) => db.name === '/save')), false);

    await alice.page.reload();
    await alice.page.waitForFunction(() => gameStarted);
    assert.equal((await alice.context.cookies()).some(cookie => cookie.name === 'warblade_session' && cookie.httpOnly), true);
    assert.deepEqual(await alice.page.evaluate(async () => ({
      databases: await indexedDB.databases(), local: localStorage.length, session: sessionStorage.length,
    })), {databases: [], local: 0, session: 0});
    await alice.page.waitForFunction(() => Module._WebAccountStatus() === 1);
    assert.equal(await alice.page.evaluate(() => Module.accountName), 'BrowserAlice');
    await play(alice.page, 2);
    const bob = await account(browser, 'BrowserBob');
    await play(bob.page, 1);
    assert.equal(await bob.page.evaluate(async (path) =>
      (await accountFetch('/api/saves/' + path)).text(), PROFILE), '1\n');
    assert.deepEqual(alice.errors, []);
    assert.deepEqual(bob.errors, []);
    await bob.context.close();
    await alice.context.close();
  } finally { await browser.close(); }
});

function candidate(base, name, score) {
  const raw = zlib.inflateSync(Buffer.from(base));
  raw.fill(0, 0x8, 0x8 + 0x68);
  raw.write(name, 0x8, 'ascii');
  raw.writeBigInt64LE(BigInt(score), 0x8 + 0x28);
  return [...zlib.deflateSync(raw)];
}

test('two stale tabs submit scores without losing either run', async () => {
  const browser = await chromium.launch();
  try {
    const alice = await account(browser, 'ScoreAlice');
    const bob = await account(browser, 'ScoreBob');
    const base = await alice.page.evaluate((path) => [...FS.readFile(path)], SCORE);
    const otherBase = await bob.page.evaluate((path) => [...FS.readFile(path)], SCORE);
    assert.deepEqual(otherBase, base);
    for (const [player, name, score] of [[alice, 'Alice', 1500], [bob, 'Bob', 2000]]) {
      await player.page.evaluate(async ({ path, bytes }) => {
        FS.writeFile(path, new Uint8Array(bytes));
        await persistSaves();
      }, { path: SCORE, bytes: candidate(base, name, score) });
      assert.equal(await player.page.locator('#save-alert').isHidden(), true);
    }
    const board = await bob.page.evaluate(async () =>
      [...new Uint8Array(await (await accountFetch('/api/hiscores')).arrayBuffer())]);
    const raw = zlib.inflateSync(Buffer.from(board));
    assert.equal(raw.toString('ascii', 0x8, 0x8 + 3), 'Bob');
    assert.equal(raw.readBigInt64LE(0x8 + 0x28), 2000n);
    assert.equal(raw.toString('ascii', 0x8 + 0x68, 0x8 + 0x68 + 5), 'Alice');
    assert.equal(raw.readBigInt64LE(0x8 + 0x68 + 0x28), 1500n);
    assert.deepEqual(alice.errors, []);
    assert.deepEqual(bob.errors, []);
    await alice.context.close();
    await bob.context.close();
  } finally { await browser.close(); }
});
