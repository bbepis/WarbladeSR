// Native completed-frame telemetry: deterministic browser checks and an opt-in WASM smoke.
'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const fs = require('node:fs');
const path = require('node:path');
const fields = ['version', 'device', 'browser', 'phase', 'frames', 'duration_ms', 'fps',
  'mean_ms', 'p95_ms', 'max_ms', 'over_33_pct', 'target_fps'].sort();

async function performancePage(browser) {
  const context = await browser.newContext({ viewport: { width: 800, height: 600 } });
  // Ambient cookies must not accompany anonymous performance summaries.
  await context.addCookies([{ name: 'private_session', value: 'do-not-send', url: 'http://warblade.test/' }]);
  const page = await context.newPage();
  const errors = [], reports = [];
  page.on('pageerror', error => errors.push(error.message));
  await page.route('http://warblade.test/**', route => {
    const request = route.request(), url = new URL(request.url());
    if (url.pathname === '/api/performance') {
      reports.push({ headers: request.headers(), body: request.postDataJSON() });
      return route.fulfill({ status: 204, body: '' });
    }
    if (url.pathname === '/loading-title.jpg') return route.fulfill({ status: 404, body: '' });
    if (url.pathname === '/warblade.js') return route.fulfill({ body: '' });
    assert.ok(!url.pathname.startsWith('/api/'), 'sampling must not contact account APIs');
    const file = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
    return route.fulfill({ contentType: file.endsWith('.css') ? 'text/css' :
      file.endsWith('.js') ? 'text/javascript' : 'text/html',
      body: fs.readFileSync(path.join(__dirname, '../web', file)) });
  });
  await page.goto('http://warblade.test/');
  await page.evaluate(() => { document.getElementById('loading').hidden = true;
    sessionToken = 'private-token-that-must-not-be-sent'; accountName = 'PrivatePlayer'; });
  return { context, page, errors, reports };
}
function checkReport(report) {
  assert.deepEqual(Object.keys(report.body).sort(), fields);
  assert.equal(report.headers.authorization, undefined);
  assert.equal(report.headers.cookie, undefined);
  assert.ok(['menu', 'play', 'paused'].includes(report.body.phase));
  assert.ok(!JSON.stringify(report.body).includes('Private'));
}

test('FPS follows completed native frames at 30 FPS; aggregates once per minute without identity or storage', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, reports, errors } = await performancePage(browser);
    await page.waitForTimeout(1100);
    assert.equal(await page.locator('#performance').isVisible(), false,
      'browser animation frames alone must not create a game FPS reading');
    await page.evaluate(() => {
      Module.onPerformanceFrame(1, 60, 0);
      for (let i = 1; i <= 31; i++) Module.onPerformanceFrame(1, 60, i * 1000 / 30);
    });
    assert.equal(await page.locator('#performance').textContent(), '30 FPS');
    assert.equal(await page.locator('#performance').isVisible(), true);
    assert.equal(reports.length, 0);
    await page.evaluate(() => {
      for (let i = 32; i <= 1801; i++) Module.onPerformanceFrame(1, 60, i * 1000 / 30);
    });
    await page.waitForFunction(() => !performanceSending);
    assert.equal(reports.length, 1, '1800 completed frames produce one summary');
    checkReport(reports[0]);
    assert.equal(reports[0].body.fps, 30);
    assert.equal(reports[0].body.mean_ms, 33.33);
    assert.equal(reports[0].body.target_fps, 60);
    assert.equal(reports[0].body.phase, 'play');
    assert.equal(reports[0].body.over_33_pct, 0);
    assert.ok(reports[0].body.p95_ms >= reports[0].body.mean_ms);
    assert.deepEqual(await page.evaluate(async () => ({ local: localStorage.length,
      session: sessionStorage.length, databases: await indexedDB.databases() })),
    { local: 0, session: 0, databases: [] });
    assert.deepEqual(errors, []);
  } finally { await browser.close(); }
});

test('phase summaries retain slow frame tails without mixing menu and gameplay', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, reports } = await performancePage(browser);
    await page.evaluate(() => {
      let now = 0;
      Module.onPerformanceFrame(0, 60, now);
      for (let i = 0; i < 100; i++) Module.onPerformanceFrame(0, 60, now += 16);
      Module.onPerformanceFrame(1, 60, now += 16);
      for (let i = 0; i < 90; i++) Module.onPerformanceFrame(1, 60, now += 20);
      for (let i = 0; i < 10; i++) Module.onPerformanceFrame(1, 60, now += 100);
      Module.onPerformanceFrame(2, 60, now += 16);
      for (let i = 0; i < 100; i++) Module.onPerformanceFrame(2, 60, now += 16);
      // This is a completed frame in a different phase, so its transition
      // interval is omitted while the actual reporting clock reaches one minute.
      Module.onPerformanceFrame(0, 60, 60000);
    });
    await page.waitForFunction(() => !performanceSending);
    assert.equal(reports.length, 3, 'at most one summary per phase in the minute');
    reports.forEach(checkReport);
    const play = reports.find(report => report.body.phase === 'play').body;
    assert.equal(play.frames, 100);
    assert.equal(play.duration_ms, 2800);
    assert.equal(play.fps, 35.71);
    assert.equal(play.mean_ms, 28);
    assert.equal(play.p95_ms, 100);
    assert.equal(play.max_ms, 100);
    assert.equal(play.over_33_pct, 10);
    assert.equal(reports.find(report => report.body.phase === 'menu').body.mean_ms, 16);
  } finally { await browser.close(); }
});

test('hidden time and the resume interval are discarded, with a fresh report minute', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, reports } = await performancePage(browser);
    await page.evaluate(() => {
      Module.onPerformanceFrame(1, 60, 0);
      for (let i = 1; i <= 100; i++) Module.onPerformanceFrame(1, 60, i * 20);
      Object.defineProperty(document, 'visibilityState', { configurable: true, get: () => 'hidden' });
      document.dispatchEvent(new Event('visibilitychange'));
      Module.onPerformanceFrame(1, 60, 90000);
    });
    assert.equal(await page.locator('#performance').isVisible(), false);
    assert.equal(reports.length, 0);
    await page.evaluate(() => {
      Object.defineProperty(document, 'visibilityState', { configurable: true, get: () => 'visible' });
      document.dispatchEvent(new Event('visibilitychange'));
      Module.onPerformanceFrame(1, 60, 100000);
      for (let i = 1; i <= 3001; i++) Module.onPerformanceFrame(1, 60, 100000 + i * 20);
    });
    await page.waitForFunction(() => !performanceSending);
    assert.equal(reports.length, 1);
    assert.equal(reports[0].body.fps, 50);
    assert.equal(reports[0].body.max_ms, 20);
    assert.equal(reports[0].body.over_33_pct, 0);
  } finally { await browser.close(); }
});

test('a stalled or rejected telemetry request never overlaps, retries or interrupts frame sampling', async () => {
  const browser = await chromium.launch({ args: ['--no-sandbox'] });
  try {
    const { page, errors } = await performancePage(browser);
    await page.evaluate(() => {
      window.reportAttempts = [];
      window.fetch = (url, options) => {
        reportAttempts.push({ url, credentials: options.credentials, headers: options.headers });
        return new Promise((resolve, reject) => { window.rejectReport = reject; });
      };
      Module.onPerformanceFrame(1, 60, 0);
      for (let i = 1; i <= 3001; i++) Module.onPerformanceFrame(1, 60, i * 20);
      for (let i = 3002; i <= 6001; i++) Module.onPerformanceFrame(1, 60, i * 20);
    });
    assert.equal(await page.evaluate(() => reportAttempts.length), 1);
    assert.equal(await page.locator('#performance').textContent(), '50 FPS');
    assert.equal(await page.evaluate(() => reportAttempts[0].credentials), 'omit');
    await page.evaluate(() => rejectReport(new Error('offline')));
    await page.waitForFunction(() => !performanceSending);
    await page.waitForTimeout(50);
    assert.equal(await page.evaluate(() => reportAttempts.length), 1, 'failed batch has no retry');
    await page.evaluate(() => {
      for (let i = 6002; i <= 6101; i++) Module.onPerformanceFrame(1, 60, i * 20);
    });
    assert.equal(await page.evaluate(() => reportAttempts.length), 1, 'no per-frame requests');
    assert.deepEqual(errors, []);
  } finally { await browser.close(); }
});

test('real mobile guest game updates native FPS and sends a bounded anonymous gameplay summary',
  { skip: !process.env.WARBLADE_GAME_URL, timeout: 210000 }, async () => {
    const browser = await chromium.launch({ args: ['--no-sandbox'] });
    try {
      const context = await browser.newContext({ viewport: { width: 375, height: 812 },
        isMobile: true, hasTouch: true });
      const page = await context.newPage();
      const reports = [], accountWrites = [], errors = [];
      page.on('pageerror', error => errors.push(error.message));
      await page.route('**/api/**', route => {
        const request = route.request();
        if (new URL(request.url()).pathname === '/api/performance') {
          reports.push({ headers: request.headers(), body: request.postDataJSON() });
          return route.fulfill({ status: 204, body: '' });
        }
        if (request.method() !== 'GET') accountWrites.push(request.url());
        return route.abort();
      });
      await page.goto(process.env.WARBLADE_GAME_URL);
      await page.waitForFunction(() => Module._WebLoginReady && Module._WebLoginReady() === 1,
        null, { timeout: 120000 });
      await page.locator('#loading').waitFor({ state: 'hidden' });
      await page.waitForTimeout(1500);
      const guest = page.locator('#login-guest');
      await guest.waitFor({ state: 'visible' });
      const bounds = await guest.boundingBox();
      await page.touchscreen.tap(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
      await page.waitForFunction(() => Module._WebIsGuest() === 1);
      await page.waitForFunction(() => /^\d+ FPS$/.test(document.getElementById('performance').textContent) &&
        !document.getElementById('performance').hidden);
      // Wait for a genuine minute of native frames; do not inject timestamps.
      await page.waitForFunction(() => performanceReportAt !== null, null, { timeout: 5000 });
      const deadline = Date.now() + 75000;
      while (!reports.some(report => report.body.phase === 'play') && Date.now() < deadline)
        await page.waitForTimeout(500);
      assert.ok(reports.some(report => report.body.phase === 'play'), 'native gameplay must produce a minute summary');
      assert.ok(reports.length <= 3, 'one bounded batch in the first minute');
      reports.forEach(checkReport);
      const play = reports.find(report => report.body.phase === 'play').body;
      assert.equal(play.device, 'mobile');
      assert.ok(play.fps > 0 && play.fps <= 300);
      assert.ok(play.frames > 0);
      assert.deepEqual(accountWrites, []);
      assert.deepEqual(await context.cookies(), []);
      assert.deepEqual(await page.evaluate(async () => ({ local: localStorage.length,
        session: sessionStorage.length, databases: await indexedDB.databases() })),
      { local: 0, session: 0, databases: [] });
      assert.deepEqual(errors, []);
    } finally { await browser.close(); }
  });
