#ifndef _SETUP_PORTAL_H_
#define _SETUP_PORTAL_H_

#include <Arduino.h>

// Simsiyah CSS enjeksiyonu (WiFiManager'in /wifisave, /info, /erase, /restart sayfalari icin)
static const char WM_DARK_HEAD[] PROGMEM =
"<meta name='theme-color' content='#000000'>"
"<style>"
"html,body{background:#000000!important;color:#f3f4f6!important;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif!important;margin:0;padding:16px!important;}"
".wrap{max-width:440px!important;width:100%!important;margin:0 auto!important;background:#0d0d12!important;border:1px solid #22222c!important;border-radius:16px!important;padding:20px!important;box-sizing:border-box!important;}"
"h1,h2,h3,h4,dt,label{color:#ffffff!important;}"
"dd,p,small,em{color:#9ca3af!important;}"
"a{color:#f7931a!important;}"
"input,select{background:#14141c!important;color:#ffffff!important;border:1px solid #2a2a36!important;border-radius:10px!important;padding:12px!important;font-size:15px!important;width:100%!important;box-sizing:border-box!important;}"
"input:focus{outline:none!important;border-color:#f7931a!important;}"
"button,input[type='button'],input[type='submit']{background:#f7931a!important;color:#000000!important;font-weight:700!important;border:none!important;border-radius:12px!important;padding:12px 16px!important;font-size:15px!important;line-height:1.4!important;}"
"button.D{background:#dc2626!important;color:#ffffff!important;}"
".msg{background:#14141c!important;color:#f3f4f6!important;border:1px solid #2a2a36!important;border-left:4px solid #f7931a!important;border-radius:10px!important;}"
".msg.S{border-left-color:#22c55e!important;}"
".msg.D{border-left-color:#ef4444!important;}"
"hr{border:0!important;border-top:1px solid #22222c!important;margin:16px 0!important;}"
"</style>";

// Ozel Simsiyah Turkce Kurulum Portali HTML (Offline jsQR destekli)
static const char SETUP_PAGE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no">
<meta name="theme-color" content="#000000">
<title>NerdMiner C3 Kurulum</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent;}
html,body{background:#000000;color:#f3f4f6;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;min-height:100vh;padding:14px 12px 32px;}
.container{max-width:440px;margin:0 auto;}
.hdr{display:flex;align-items:center;justify-content:space-between;padding:6px 4px 16px;}
.brand{display:flex;align-items:center;gap:10px;}
.btc-ico{width:34px;height:34px;border-radius:10px;background:#f7931a;color:#000;display:flex;align-items:center;justify-content:center;font-weight:900;font-size:20px;}
.brand h1{font-size:18px;font-weight:700;color:#fff;letter-spacing:-0.2px;}
.brand span{display:block;font-size:11px;color:#9ca3af;margin-top:1px;}
.badge{font-size:11px;font-weight:600;color:#f7931a;background:rgba(247,147,26,0.12);border:1px solid rgba(247,147,26,0.3);padding:4px 9px;border-radius:999px;}
.card{background:#0d0d12;border:1px solid #22222c;border-radius:16px;padding:16px;margin-bottom:14px;}
.card-title{font-size:14px;font-weight:700;color:#fff;margin-bottom:12px;display:flex;align-items:center;justify-content:space-between;}
.card-title small{font-size:11px;font-weight:500;color:#9ca3af;}
.qr-grid{display:grid;grid-template-columns:1fr auto;gap:8px;margin-bottom:8px;}
.btn-qr{display:flex;align-items:center;justify-content:center;gap:8px;background:#f7931a;color:#000;font-weight:700;font-size:15px;padding:13px 14px;border-radius:12px;cursor:pointer;border:none;text-align:center;user-select:none;}
.btn-sec{display:flex;align-items:center;justify-content:center;gap:6px;background:#14141c;color:#f3f4f6;border:1px solid #2a2a36;font-weight:600;font-size:13px;padding:12px 13px;border-radius:12px;cursor:pointer;user-select:none;}
.btn-sec:active,.btn-qr:active,.btn-submit:active{opacity:0.8;}
.sub-actions{display:flex;align-items:center;justify-content:space-between;gap:8px;margin-bottom:10px;}
.btn-link{background:none;border:none;color:#9ca3af;font-size:12px;cursor:pointer;padding:4px 0;text-decoration:underline;}
.hint{font-size:11px;color:#6b7280;text-align:right;}
.hidden-file{position:absolute;width:1px;height:1px;opacity:0;overflow:hidden;pointer-events:none;}
.status{display:none;padding:10px 12px;border-radius:10px;font-size:13px;font-weight:600;margin-bottom:10px;line-height:1.4;}
.status.ok{display:block;background:rgba(34,197,94,0.14);border:1px solid #22c55e;color:#4ade80;}
.status.err{display:block;background:rgba(239,68,68,0.14);border:1px solid #ef4444;color:#f87171;}
.status.info{display:block;background:rgba(247,147,26,0.12);border:1px solid rgba(247,147,26,0.4);color:#fbbf24;}
.paste-box{display:none;margin-bottom:10px;}
.paste-row{display:flex;gap:6px;}
.scan-list{display:none;max-height:185px;overflow-y:auto;border:1px solid #22222c;border-radius:10px;margin-bottom:12px;background:#07070a;}
.net-item{display:flex;align-items:center;justify-content:space-between;padding:10px 12px;border-bottom:1px solid #181820;cursor:pointer;font-size:13px;}
.net-item:last-child{border-bottom:none;}
.net-item:active{background:#181822;}
.net-name{color:#fff;font-weight:600;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;max-width:72%;}
.net-meta{color:#9ca3af;font-size:12px;flex-shrink:0;}
.fld{margin-bottom:12px;}
.fld:last-child{margin-bottom:0;}
.fld label{display:block;font-size:12px;font-weight:600;color:#9ca3af;margin-bottom:6px;}
.inp-wrap{position:relative;display:flex;align-items:center;gap:6px;}
input[type=text],input[type=password],input[type=number]{width:100%;background:#14141c;color:#fff;border:1px solid #2a2a36;border-radius:10px;padding:12px 13px;font-size:15px;outline:none;transition:border-color 0.2s,box-shadow 0.2s;}
input:focus{border-color:#f7931a;}
input.qr-ok{border-color:#22c55e!important;box-shadow:0 0 0 2px rgba(34,197,94,0.25);}
.btn-inline{position:absolute;right:8px;background:#1e1e28;border:1px solid #2e2e3c;color:#d1d5db;font-size:11px;font-weight:600;padding:6px 10px;border-radius:7px;cursor:pointer;}
.btn-cam-mini{flex-shrink:0;display:flex;align-items:center;justify-content:center;background:#14141c;border:1px solid #2a2a36;color:#f7931a;border-radius:10px;padding:11px 13px;font-size:16px;cursor:pointer;}
details.card summary{list-style:none;cursor:pointer;font-size:14px;font-weight:700;color:#d1d5db;display:flex;align-items:center;justify-content:space-between;}
details.card summary::-webkit-details-marker{display:none;}
details.card[open] summary{margin-bottom:14px;color:#fff;}
.chk-row{display:flex;align-items:center;gap:10px;margin-top:4px;cursor:pointer;}
.chk-row input{width:18px;height:18px;accent-color:#f7931a;}
.chk-row span{font-size:13px;color:#d1d5db;}
.btn-submit{width:100%;background:#f7931a;color:#000;border:none;border-radius:14px;padding:15px;font-size:16px;font-weight:800;cursor:pointer;margin-top:4px;box-shadow:0 4px 18px rgba(247,147,26,0.22);}
.footer{display:flex;justify-content:center;gap:18px;margin-top:18px;}
.footer a{color:#6b7280;font-size:12px;text-decoration:none;}
.footer a:hover{color:#f7931a;}
</style>
<script defer src="/qr.js"></script>
</head>
<body>
<div class="container">
  <div class="hdr">
    <div class="brand">
      <div class="btc-ico">&#8383;</div>
      <div>
        <h1>NerdMiner C3</h1>
        <span>Solo Bitcoin Madenci</span>
      </div>
    </div>
    <div class="badge">ESP32-C3</div>
  </div>

  <form method="POST" action="/wifisave" onsubmit="return onFormSubmit()">
    <div class="card">
      <div class="card-title">
        <span>1. WiFi Ba&#287;lant&#305;s&#305;</span>
        <small>Modem QR ile h&#305;zl&#305; doldur</small>
      </div>

      <input id="qr-cam" class="hidden-file" type="file" accept="image/*" capture="environment" onchange="handleQrFile(this,'wifi')">
      <input id="qr-gal" class="hidden-file" type="file" accept="image/*" onchange="handleQrFile(this,'wifi')">
      <input id="qr-btc" class="hidden-file" type="file" accept="image/*" onchange="handleQrFile(this,'btc')">

      <div class="qr-grid">
        <label for="qr-cam" class="btn-qr">&#128247; Modem WiFi QR Okut</label>
        <label for="qr-gal" class="btn-sec">&#128444;&#65039; Galeri</label>
      </div>

      <div class="sub-actions">
        <button type="button" class="btn-link" onclick="togglePaste()">&#128203; Metin Yap&#305;&#351;t&#305;r</button>
        <span class="hint">Kamera a&#231;&#305;lmazsa taray&#305;c&#305;da 192.168.4.1 a&#231;&#305;n</span>
      </div>

      <div id="paste-box" class="paste-box">
        <div class="paste-row">
          <input id="paste-inp" type="text" placeholder="WIFI:T:WPA;S:AgAdi;P:Sifre;;">
          <button type="button" class="btn-sec" onclick="applyPastedText()">Uygula</button>
        </div>
      </div>

      <div id="qr-status" class="status"></div>

      <button type="button" id="btn-scan" class="btn-sec" style="width:100%;margin-bottom:12px;" onclick="scanWifi()">&#128225; &#199;evredeki A&#287;lar&#305; Tara</button>
      <div id="scan-list" class="scan-list"></div>

      <div class="fld">
        <label for="s">WiFi Ad&#305; (SSID)</label>
        <input id="s" name="s" type="text" maxlength="32" autocorrect="off" autocapitalize="none" value="{{SSID}}" required placeholder="WiFi ad&#305;n&#305; girin">
      </div>

      <div class="fld">
        <label for="p">WiFi &#350;ifresi</label>
        <div class="inp-wrap">
          <input id="p" name="p" type="password" maxlength="64" placeholder="WiFi &#351;ifresi (&#351;ifresizse bo&#351; b&#305;rak&#305;n)" style="padding-right:68px;">
          <button type="button" class="btn-inline" onclick="togglePass(this)">G&#246;ster</button>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">
        <span>2. Bitcoin C&#252;zdan&#305;</span>
        <small>&#214;d&#252;l adresi</small>
      </div>
      <div class="fld">
        <label for="btcAddress">BTC Adresiniz</label>
        <div class="inp-wrap">
          <input id="btcAddress" name="btcAddress" type="text" maxlength="80" value="{{BTC}}" placeholder="bc1q... veya 1...">
          <label for="qr-btc" class="btn-cam-mini" title="C&#252;zdan QR Okut">&#128247;</label>
        </div>
      </div>
    </div>

    <details class="card">
      <summary>
        <span>3. Geli&#351;mi&#351; Ayarlar (Havuz &amp; Saat)</span>
        <span style="font-size:12px;color:#f7931a;">A&#231; / Kapat &#9662;</span>
      </summary>
      <div class="fld">
        <label for="Poolurl">Havuz Adresi (Pool URL)</label>
        <input id="Poolurl" name="Poolurl" type="text" maxlength="80" value="{{POOL}}" placeholder="pool.nerdminer.io">
      </div>
      <div class="fld">
        <label for="Poolport">Havuz Portu</label>
        <input id="Poolport" name="Poolport" type="number" maxlength="7" value="{{PORT}}" placeholder="3333">
      </div>
      <div class="fld">
        <label for="Poolpass">Havuz &#350;ifresi (Opsiyonel)</label>
        <input id="Poolpass" name="Poolpass" type="text" maxlength="80" value="{{PPASS}}" placeholder="x">
      </div>
      <div class="fld">
        <label for="TimeZone">Saat Dilimi (UTC Fark&#305;)</label>
        <input id="TimeZone" name="TimeZone" type="number" maxlength="3" value="{{TZ}}" placeholder="3">
      </div>
      <label class="chk-row" for="SaveStatsToNVS">
        <input id="SaveStatsToNVS" name="SaveStatsToNVS" type="checkbox" value="T" {{STATS_CHK}}>
        <span>&#304;statistikleri haf&#305;zaya kaydet</span>
      </label>
    </details>

    <button type="submit" id="btn-save" class="btn-submit">Kaydet ve Madencili&#287;i Ba&#351;lat</button>
  </form>

  <div class="footer">
    <a href="/info">Cihaz Bilgisi</a>
    <a href="/erase" onclick="return confirm('Kay&#305;tl&#305; WiFi ayarlar&#305; silinsin mi?')">Ayarlar&#305; S&#305;f&#305;rla</a>
    <a href="/restart">Yeniden Ba&#351;lat</a>
  </div>
</div>

<script>
function showStatus(msg, cls) {
  var el = document.getElementById('qr-status');
  el.textContent = msg;
  el.className = 'status ' + cls;
}

function flashField(el) {
  el.classList.remove('qr-ok');
  void el.offsetWidth;
  el.classList.add('qr-ok');
  setTimeout(function() { el.classList.remove('qr-ok'); }, 2500);
}

function togglePass(btn) {
  var p = document.getElementById('p');
  if (p.type === 'password') {
    p.type = 'text';
    btn.textContent = 'Gizle';
  } else {
    p.type = 'password';
    btn.textContent = 'G\u00f6ster';
  }
}

function togglePaste() {
  var box = document.getElementById('paste-box');
  box.style.display = (box.style.display === 'block') ? 'none' : 'block';
  if (box.style.display === 'block') {
    document.getElementById('paste-inp').focus();
  }
}

function parseWifiQR(raw) {
  if (!raw || typeof raw !== 'string') return null;
  var s = raw.trim();
  if (!/^WIFI:/i.test(s)) return null;
  var body = s.slice(5);
  var res = { S: '', P: '', T: '' };
  var i = 0;
  while (i < body.length) {
    if (body[i] === ';') { i++; continue; }
    var colon = body.indexOf(':', i);
    if (colon === -1) break;
    var key = body.slice(i, colon).toUpperCase();
    i = colon + 1;
    var val = '';
    while (i < body.length) {
      var ch = body[i];
      if (ch === '\\' && i + 1 < body.length) {
        val += body[i + 1];
        i += 2;
      } else if (ch === ';') {
        i++;
        break;
      } else {
        val += ch;
        i++;
      }
    }
    if (key === 'S' || key === 'P' || key === 'T') {
      res[key] = val;
    }
  }
  if (!res.S) return null;
  if (res.T && res.T.toUpperCase() === 'NOPASS') res.P = '';
  return { ssid: res.S, pass: res.P, type: res.T };
}

function parseBtcQR(raw) {
  if (!raw || typeof raw !== 'string') return null;
  var s = raw.trim();
  if (/^bitcoin:/i.test(s)) {
    s = s.slice(8);
    var q = s.indexOf('?');
    if (q !== -1) s = s.slice(0, q);
    s = s.trim();
  }
  if (/^(bc1[a-zA-HJ-NP-Z0-9]{25,87}|[13][a-km-zA-HJ-NP-Z1-9]{25,34})$/.test(s)) {
    return s;
  }
  return null;
}

function applyDecodedText(raw, mode) {
  var wifi = parseWifiQR(raw);
  if (wifi) {
    var sEl = document.getElementById('s');
    var pEl = document.getElementById('p');
    sEl.value = wifi.ssid;
    pEl.value = wifi.pass;
    flashField(sEl);
    flashField(pEl);
    showStatus('\u2705 WiFi okundu: ' + wifi.ssid, 'ok');
    return true;
  }
  var btc = parseBtcQR(raw);
  if (btc) {
    var bEl = document.getElementById('btcAddress');
    bEl.value = btc;
    flashField(bEl);
    showStatus('\u2705 BTC c\u00fczdan\u0131 okundu!', 'ok');
    return true;
  }
  if (mode === 'btc') {
    showStatus('\u274c Ge\u00e7erli Bitcoin QR kodu alg\u0131lanamad\u0131.', 'err');
  } else {
    showStatus('\u274c Bu QR bir WiFi QR kodu de\u011fil (' + raw.slice(0, 28) + ').', 'err');
  }
  return false;
}

function applyPastedText() {
  var val = document.getElementById('paste-inp').value;
  applyDecodedText(val, 'wifi');
}

function ensureJsQr() {
  return new Promise(function(resolve, reject) {
    if (typeof window.jsQR === 'function') { resolve(window.jsQR); return; }
    var existing = document.querySelector('script[src="/qr.js"]');
    if (existing) {
      var waited = 0;
      var t = setInterval(function() {
        waited += 50;
        if (typeof window.jsQR === 'function') {
          clearInterval(t);
          resolve(window.jsQR);
        } else if (waited >= 3000) {
          clearInterval(t);
          loadScriptFresh(resolve, reject);
        }
      }, 50);
    } else {
      loadScriptFresh(resolve, reject);
    }
  });
}

function loadScriptFresh(resolve, reject) {
  var sc = document.createElement('script');
  sc.src = '/qr.js?t=' + Date.now();
  sc.onload = function() {
    if (typeof window.jsQR === 'function') resolve(window.jsQR);
    else reject(new Error('jsQR load failed'));
  };
  sc.onerror = function() { reject(new Error('qr.js error')); };
  document.head.appendChild(sc);
}

function decodePass(img, maxSide, cropRatio, canvas, ctx) {
  var iw = img.naturalWidth || img.width;
  var ih = img.naturalHeight || img.height;
  var sw = Math.round(iw * cropRatio);
  var sh = Math.round(ih * cropRatio);
  var sx = Math.round((iw - sw) / 2);
  var sy = Math.round((ih - sh) / 2);
  var scale = Math.min(1, maxSide / Math.max(sw, sh));
  var dw = Math.max(1, Math.round(sw * scale));
  var dh = Math.max(1, Math.round(sh * scale));
  canvas.width = dw;
  canvas.height = dh;
  ctx.drawImage(img, sx, sy, sw, sh, 0, 0, dw, dh);
  var imgData = ctx.getImageData(0, 0, dw, dh);
  return window.jsQR(imgData.data, dw, dh, { inversionAttempts: 'attemptBoth' });
}

function handleQrFile(inp, mode) {
  var file = inp.files && inp.files[0];
  if (!file) return;
  showStatus('\u23f3 QR kod taran\u0131yor...', 'info');
  var objUrl = URL.createObjectURL(file);
  var img = new Image();
  img.onload = async function() {
    try {
      var decoded = null;
      if (typeof window.BarcodeDetector === 'function') {
        try {
          var det = new window.BarcodeDetector({ formats: ['qr_code'] });
          var hits = await det.detect(img);
          if (hits && hits.length > 0 && hits[0].rawValue) {
            decoded = hits[0].rawValue;
          }
        } catch (e) {}
      }
      if (!decoded) {
        await ensureJsQr();
        var canvas = document.createElement('canvas');
        var ctx = canvas.getContext('2d', { willReadFrequently: true });
        var passes = [
          [1024, 1.0],
          [1024, 0.6],
          [1600, 1.0]
        ];
        for (var k = 0; k < passes.length; k++) {
          var hit = decodePass(img, passes[k][0], passes[k][1], canvas, ctx);
          if (hit && hit.data) {
            decoded = hit.data;
            break;
          }
        }
      }
      if (decoded) {
        applyDecodedText(decoded, mode);
      } else {
        showStatus('\u274c QR okunamad\u0131. I\u015f\u0131\u011f\u0131 art\u0131r\u0131p QR koda yak\u0131nla\u015farak tekrar deneyin.', 'err');
      }
    } catch (err) {
      showStatus('\u274c QR okuma hatas\u0131. A\u011f\u0131 manuel se\u00e7ebilir veya metin yap\u0131\u015ft\u0131rabilirsiniz.', 'err');
    } finally {
      URL.revokeObjectURL(objUrl);
      inp.value = '';
    }
  };
  img.onerror = function() {
    URL.revokeObjectURL(objUrl);
    inp.value = '';
    showStatus('\u274c G\u00f6rsel a\u00e7\u0131lamad\u0131.', 'err');
  };
  img.src = objUrl;
}

async function scanWifi() {
  var btn = document.getElementById('btn-scan');
  var list = document.getElementById('scan-list');
  btn.disabled = true;
  btn.textContent = '\u23f3 A\u011flar taran\u0131yor...';
  try {
    var r = await fetch('/scan');
    var nets = await r.json();
    list.innerHTML = '';
    if (!nets || nets.length === 0) {
      list.innerHTML = '<div class="net-item"><span class="net-name">A\u011f bulunamad\u0131, tekrar deneyin</span></div>';
    } else {
      nets.forEach(function(n) {
        var row = document.createElement('div');
        row.className = 'net-item';
        var nm = document.createElement('span');
        nm.className = 'net-name';
        nm.textContent = n.s;
        var mt = document.createElement('span');
        mt.className = 'net-meta';
        mt.textContent = (n.l ? '\ud83d\udd12 ' : '') + n.r + '%';
        row.appendChild(nm);
        row.appendChild(mt);
        row.onclick = function() {
          var sEl = document.getElementById('s');
          var pEl = document.getElementById('p');
          sEl.value = n.s;
          flashField(sEl);
          if (n.l) pEl.focus();
          list.style.display = 'none';
        };
        list.appendChild(row);
      });
    }
    list.style.display = 'block';
  } catch (e) {
    showStatus('\u274c A\u011f taramas\u0131 ba\u015far\u0131s\u0131z oldu.', 'err');
  } finally {
    btn.disabled = false;
    btn.textContent = '\ud83d\udce1 \u00c7evredeki A\u011flar\u0131 Tara';
  }
}

function onFormSubmit() {
  var btn = document.getElementById('btn-save');
  btn.textContent = 'Kaydediliyor...';
  return true;
}
</script>
</body>
</html>
)rawliteral";

#endif // _SETUP_PORTAL_H_
