// Actual SDL output, rather than a stand-in canvas: density must improve the
// picture without moving the native login or breaking trusted mobile taps.
'use strict';
const { test } = require('node:test');
const assert = require('node:assert/strict');
const { chromium } = require('playwright');
const path = require('node:path');

for (const density of [1, 3]) test(`mobile scaling at DPR ${density} preserves display density and input after rotation`,
  { skip: !process.env.WARBLADE_GAME_URL, timeout: 180000 }, async () => {
    const browser = await chromium.launch({ args: ['--no-sandbox'] });
    try {
      const context = await browser.newContext({ viewport: { width: 375, height: 812 },
        deviceScaleFactor: density, isMobile: true, hasTouch: true });
      const page = await context.newPage();
      // Observe the real SDL presentation sampler without changing GL state or
      // drawing a substitute scene. Only the final 800x600 canvas copy is relevant.
      await page.addInitScript(() => {
        const getContext = HTMLCanvasElement.prototype.getContext;
        HTMLCanvasElement.prototype.getContext = function (...args) {
          const gl = getContext.apply(this, args);
          if (this.id !== 'canvas' || !/^webgl/.test(args[0]) || !gl || gl.scalingObserved)
            return gl;
          gl.scalingObserved = true;
          const textures = new Map(), units = new Map(), shaders = new Map(), programs = new Map();
          let active = gl.TEXTURE0, framebuffer = null, program = null;
          const wrap = (name, observe) => {
            const call = gl[name];
            gl[name] = function (...values) {
              observe(values);
              return call.apply(this, values);
            };
          };
          wrap('activeTexture', values => { active = values[0]; });
          wrap('bindTexture', values => {
            if (values[0] === gl.TEXTURE_2D) {
              units.set(active, values[1]);
              if (values[1] && !textures.has(values[1])) textures.set(values[1], {});
            }
          });
          wrap('bindFramebuffer', values => {
            if (values[0] === gl.FRAMEBUFFER || values[0] === gl.DRAW_FRAMEBUFFER)
              framebuffer = values[1];
          });
          wrap('texImage2D', values => {
            const texture = textures.get(units.get(active));
            if (texture && values.length === 9) {
              texture.width = values[3]; texture.height = values[4];
            }
          });
          wrap('texParameteri', values => {
            const texture = textures.get(units.get(active));
            if (texture && values[1] === gl.TEXTURE_MIN_FILTER) texture.min = values[2];
            if (texture && values[1] === gl.TEXTURE_MAG_FILTER) texture.mag = values[2];
          });
          wrap('shaderSource', values => { shaders.set(values[0], values[1]); });
          wrap('attachShader', values => {
            if (!programs.has(values[0])) programs.set(values[0], new Set());
            programs.get(values[0]).add(values[1]);
          });
          wrap('useProgram', values => { program = values[0]; });
          const observeDraw = () => {
            if (framebuffer !== null) return;
            for (const texture of units.values()) {
              const state = textures.get(texture);
              if (state?.width === 800 && state.height === 600)
                window.presentationSampler = { min: state.min, mag: state.mag, linear: gl.LINEAR,
                  pixelart: [...(programs.get(program) || [])].some(shader =>
                    (shaders.get(shader) || '').includes('GetPixelArtSample')) };
            }
          };
          for (const name of ['drawArrays', 'drawElements']) wrap(name, observeDraw);
          return gl;
        };
      });
      const errors = [], accountRequests = [];
      page.on('pageerror', error => errors.push(error.message));
      await page.route('**/api/**', route => {
        if (new URL(route.request().url()).pathname === '/api/performance')
          return route.fulfill({ status: 204 });
        accountRequests.push(route.request().url());
        return route.abort();
      });
      await page.goto(process.env.WARBLADE_GAME_URL);
      await page.waitForFunction(() => gameStarted && Module._WebLoginReady() === 1,
        null, { timeout: 120000 });
      await page.locator('#loading').waitFor({ state: 'hidden' });
      await page.evaluate(() => {
        const sync = Module.syncLoginInputs;
        Module.syncLoginInputs = state => { window.lastNativeLogin = state; return sync(state); };
      });
      await page.waitForTimeout(1500); // allow the native window's entrance to finish

      const checkDisplay = async () => {
        const aligned = await page.waitForFunction(density => {
          const canvas = document.getElementById('canvas');
          const rect = canvas.getBoundingClientRect();
          const state = window.lastNativeLogin;
          const password = document.getElementById('login-password').getBoundingClientRect();
          const guest = document.getElementById('login-guest').getBoundingClientRect();
          // SDL density and browser input overlays settle in separate frame
          // callbacks after rotation. Read them together and await actual
          // alignment, rather than comparing boxes captured in different frames.
          if (!state || state.screenW !== 800 || state.screenH !== 600 ||
            Math.abs(canvas.width - rect.width * density) > 2 ||
            Math.abs(canvas.height - rect.height * density) > 2 ||
            Math.abs(password.x - (rect.x + state.x * rect.width / 800)) >= 2 ||
            Math.abs(password.y - (rect.y + (state.y + 25) * rect.height / 600)) >= 2 ||
            Math.abs(guest.x - (rect.x + (state.x - 120) * rect.width / 800)) >= 2 ||
            Math.abs(guest.y - (rect.y + (state.y + 192) * rect.height / 600)) >= 2)
            return false;
          return { canvas: rect.toJSON(), password: password.toJSON(), guest: guest.toJSON(), state,
            rendering: getComputedStyle(canvas).imageRendering };
        }, density, { timeout: 10000 });
        const { canvas, password, guest, state, rendering } = await aligned.jsonValue();
        await aligned.dispose();
        assert.ok(Math.abs(canvas.width / canvas.height - 4 / 3) < 0.01);
        assert.equal(rendering, 'auto');
        assert.equal(state.screenW, 800, 'game and input keep original logical resolution');
        assert.equal(state.screenH, 600);
        assert.ok(Math.abs(password.x - (canvas.x + state.x * canvas.width / 800)) < 2);
        assert.ok(Math.abs(password.y - (canvas.y + (state.y + 25) * canvas.height / 600)) < 2);
        assert.ok(Math.abs(guest.x - (canvas.x + (state.x - 120) * canvas.width / 800)) < 2);
        assert.ok(Math.abs(guest.y - (canvas.y + (state.y + 192) * canvas.height / 600)) < 2);
        return password;
      };
      let password = await checkDisplay();
      if (density === 1) {
        await page.waitForFunction(() => window.presentationSampler &&
          presentationSampler.min === presentationSampler.linear &&
          presentationSampler.mag === presentationSampler.linear &&
          !presentationSampler.pixelart, null, { timeout: 10000 });
      }
      if (process.env.WARBLADE_SCALING_EVIDENCE)
        await page.screenshot({ path: path.join(process.env.WARBLADE_SCALING_EVIDENCE,
          `mobile-login-portrait-dpr${density}.png`) });
      await page.touchscreen.tap(password.x + password.width / 2, password.y + password.height / 2);
      assert.equal(await page.evaluate(() => document.activeElement.id), 'login-password');
      await page.keyboard.type('MiXeD#123');
      await page.waitForFunction(() => lastNativeLogin.password === 'MiXeD#123');
      await page.setViewportSize({ width: 812, height: 375 });
      password = await checkDisplay();
      await page.touchscreen.tap(password.x + password.width / 2, password.y + password.height / 2);
      assert.equal(await page.evaluate(() => document.activeElement.id), 'login-password');
      await page.keyboard.press('Backspace');
      await page.waitForFunction(() => lastNativeLogin.password === 'MiXeD#12');
      await page.locator('#performance').waitFor({ state: 'visible', timeout: 15000 });
      assert.match(await page.locator('#performance').textContent(), /^\d+ FPS$/);
      if (process.env.WARBLADE_SCALING_EVIDENCE)
        await page.screenshot({ path: path.join(process.env.WARBLADE_SCALING_EVIDENCE,
          `mobile-login-dpr${density}.png`) });
      const guest = await page.locator('#login-guest').boundingBox();
      await page.touchscreen.tap(guest.x + guest.width / 2, guest.y + guest.height / 2);
      await page.waitForFunction(() => Module._WebIsGuest() === 1, null, { timeout: 15000 });
      assert.equal(await page.evaluate(() => Module._WebCanPlay()), 1);
      assert.deepEqual(accountRequests, [], 'scaling/input checks need no account or saved progress');
      assert.deepEqual(errors, []);
    } finally { await browser.close(); }
  });
