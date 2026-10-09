'use strict';
const {test} = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const {chromium} = require('playwright');

test('title loading view starts the game once without a click and keeps recovery in the game frame', async () => {
  const browser = await chromium.launch({args:['--no-sandbox']});
  try {
    const page = await browser.newPage();
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    let releaseData;
    const dataReady = new Promise(resolve => {releaseData = resolve;});
    await page.route('http://warblade.test/**', async route => {
      const url = new URL(route.request().url());
      if (url.pathname === '/api/me') return route.fulfill({ json: { username: null } });
      if (url.pathname === '/loading-title.jpg') return route.fulfill({status:404,body:''});
      if (url.pathname === '/data/') return route.fulfill({contentType:'application/json',
        body:JSON.stringify([{name:'warblade.pac',type:'file',size:1}])});
      if (url.pathname === '/data/warblade.pac') {
        await dataReady;
        return route.fulfill({body:'x'});
      }
      if (url.pathname === '/warblade.js') return route.fulfill({contentType:'application/javascript',body:`
        var FS = {stat:()=>({}),mkdir:()=>{},writeFile:()=>{},chdir:()=>{}};
        Module.startCount=0; Module.nativeReady=false;
        Module._WebGameReady=()=>Module.nativeReady ? 1 : 0;
        Module._WebLoginReady=()=>0;
        Module.callMain=()=>{Module.startCount++;};
        Module.onRuntimeInitialized();
      `});
      const file = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
      return route.fulfill({contentType:file.endsWith('.js')?'application/javascript':file.endsWith('.css')?'text/css':'text/html',
        body:fs.readFileSync(path.join(__dirname,'../web',file))});
    });
    await page.goto('http://warblade.test/');
    assert.equal(await page.locator('#play, #panel, #intro').count(),0);
    assert.equal(await page.locator('#loading').isVisible(),true);
    assert.equal(await page.locator('#progress').isVisible(),true);
    assert.equal(await page.evaluate(()=>Module.startCount),0);
    releaseData();
    await page.waitForFunction(()=>Module.startCount===1);
    assert.equal(await page.locator('#loading').isVisible(),true,'keep artwork over native initialization');
    await page.evaluate(()=>{Module.nativeReady=true;});
    await page.locator('#loading').waitFor({state:'hidden'});
    await page.evaluate(()=>showReady());
    assert.equal(await page.evaluate(()=>Module.startCount),1,'no duplicate game loop or save timers');
    assert.equal(await page.evaluate(()=>document.activeElement.id),'canvas');
    await page.evaluate(()=>Module.onAbort('test failure'));
    assert.equal(await page.locator('#loading').isVisible(),true);
    assert.equal(await page.locator('#retry').isVisible(),true);
    assert.equal(await page.locator('#progress').isHidden(),true);
    assert.deepEqual(errors,[]);
  } finally {await browser.close();}
});
