#ifndef _FARM_DASHBOARD_H_
#define _FARM_DASHBOARD_H_

#include <Arduino.h>

static const char FARM_WORKER_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#000000">
<meta http-equiv="refresh" content="2;url=http://{{MASTER_IP}}/">
<title>3dbrint Farm Worker</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;}
body{background:#000000;color:#f3f4f6;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:18px;}
.card{max-width:420px;width:100%;background:#0d0d12;border:1px solid #22222c;border-radius:16px;padding:24px;text-align:center;}
.ico{width:46px;height:46px;border-radius:12px;background:#f7931a;color:#000;display:inline-flex;align-items:center;justify-content:center;font-weight:900;font-size:26px;margin-bottom:14px;}
h1{font-size:19px;margin-bottom:8px;color:#fff;}
p{font-size:13px;color:#9ca3af;line-height:1.5;margin-bottom:18px;}
.btn{display:block;width:100%;background:#f7931a;color:#000;text-decoration:none;font-weight:800;font-size:15px;padding:13px;border-radius:12px;margin-bottom:10px;}
.btn2{display:block;width:100%;background:#14141c;color:#f3f4f6;border:1px solid #2a2a36;text-decoration:none;font-weight:600;font-size:13px;padding:11px;border-radius:12px;}
</style>
</head>
<body>
<div class="card">
  <div class="ico">&#8383;</div>
  <h1>Farm Worker Aktif</h1>
  <p>Bu cihaz (<strong>{{DEV_NAME}}</strong>) tek merkezli <strong>3dbrint Farm</strong> agina bagli olarak calisiyor. Tum cihazlari tek panelden yonetmek icin Master panele yonlendiriliyorsunuz...</p>
  <a class="btn" href="http://{{MASTER_IP}}/">Master Panele Git ({{MASTER_IP}})</a>
  <a class="btn2" href="http://3dbrint.local/">http://3dbrint.local Ac</a>
  <a class="btn2" style="margin-top:8px;border-color:rgba(247,147,26,0.3);color:#f7931a;" href="/update">Manuel Cihaz Guncelleme (Web OTA)</a>
</div>
</body>
</html>)rawliteral";

static const char FARM_DASHBOARD_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<meta name="theme-color" content="#000000">
<title>3dbrint Farm Kontrol</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;-webkit-tap-highlight-color:transparent;}
html,body{background:#000000;color:#f3f4f6;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;min-height:100vh;padding:14px 12px 36px;}
.wrap{max-width:1060px;margin:0 auto;}
.hdr{display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;padding:4px 2px 16px;}
.brand{display:flex;align-items:center;gap:11px;}
.btc-ico{width:38px;height:38px;border-radius:11px;background:#f7931a;color:#000;display:flex;align-items:center;justify-content:center;font-weight:900;font-size:22px;flex-shrink:0;}
.brand h1{font-size:20px;font-weight:800;color:#fff;letter-spacing:-0.3px;}
.brand span{display:block;font-size:12px;color:#9ca3af;margin-top:2px;}
.hdr-right{display:flex;align-items:center;gap:8px;flex-wrap:wrap;}
.badge{font-size:11px;font-weight:700;color:#f7931a;background:rgba(247,147,26,0.12);border:1px solid rgba(247,147,26,0.32);padding:5px 10px;border-radius:999px;}
.live-pill{display:inline-flex;align-items:center;gap:6px;font-size:11px;font-weight:600;color:#4ade80;background:rgba(34,197,94,0.12);border:1px solid rgba(34,197,94,0.3);padding:5px 10px;border-radius:999px;}
.live-pill.err{color:#f87171;background:rgba(239,68,68,0.12);border-color:rgba(239,68,68,0.3);}
.dot{width:7px;height:7px;border-radius:50%;background:currentColor;}
.sum-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(160px,1fr));gap:10px;margin-bottom:14px;}
.sum-card{background:#0d0d12;border:1px solid #22222c;border-radius:14px;padding:14px 15px;}
.sum-lbl{font-size:11px;font-weight:600;color:#9ca3af;text-transform:uppercase;letter-spacing:0.4px;margin-bottom:6px;}
.sum-val{font-size:22px;font-weight:800;color:#fff;}
.sum-val.acc{color:#f7931a;}
.sum-sub{font-size:11px;color:#6b7280;margin-top:4px;}
.card{background:#0d0d12;border:1px solid #22222c;border-radius:15px;padding:15px;margin-bottom:14px;}
.pool-bar{display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;}
.pool-info{font-size:13px;color:#d1d5db;line-height:1.5;}
.pool-info strong{color:#fff;}
.pool-info a{color:#f7931a;text-decoration:none;font-weight:700;}
.pool-info a:hover{text-decoration:underline;}
.actions-row{display:flex;flex-wrap:wrap;gap:8px;margin-top:12px;}
.btn{background:#14141c;color:#f3f4f6;border:1px solid #2a2a36;border-radius:10px;padding:9px 13px;font-size:12px;font-weight:700;cursor:pointer;transition:opacity 0.15s,border-color 0.15s;}
.btn:hover{border-color:#f7931a;}
.btn:active{opacity:0.75;}
.btn-pri{background:#f7931a;color:#000;border-color:#f7931a;}
.btn-ok{background:rgba(34,197,94,0.14);color:#4ade80;border-color:rgba(34,197,94,0.4);}
.btn-warn{background:rgba(245,158,11,0.14);color:#fbbf24;border-color:rgba(245,158,11,0.38);}
.btn-red{background:rgba(239,68,68,0.14);color:#f87171;border-color:rgba(239,68,68,0.38);}
.btn-sm{padding:7px 10px;font-size:11px;border-radius:8px;}
details.cfg summary{list-style:none;cursor:pointer;font-size:14px;font-weight:700;color:#fff;display:flex;align-items:center;justify-content:space-between;}
details.cfg summary::-webkit-details-marker{display:none;}
.cfg-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(190px,1fr));gap:10px;margin-top:14px;}
.fld label{display:block;font-size:11px;font-weight:600;color:#9ca3af;margin-bottom:5px;}
.fld input{width:100%;background:#14141c;color:#fff;border:1px solid #2a2a36;border-radius:9px;padding:10px 11px;font-size:13px;outline:none;}
.fld input:focus{border-color:#f7931a;}
.chk-row{display:flex;align-items:center;gap:8px;margin-top:12px;cursor:pointer;font-size:13px;color:#d1d5db;}
.chk-row input{width:17px;height:17px;accent-color:#f7931a;}
.toast{display:none;padding:10px 13px;border-radius:10px;font-size:13px;font-weight:600;margin-bottom:12px;}
.toast.ok{display:block;background:rgba(34,197,94,0.14);border:1px solid #22c55e;color:#4ade80;}
.toast.err{display:block;background:rgba(239,68,68,0.14);border:1px solid #ef4444;color:#f87171;}
.nodes-hdr{display:flex;align-items:center;justify-content:space-between;margin:6px 2px 10px;}
.nodes-hdr h2{font-size:15px;font-weight:700;color:#fff;}
.nodes-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(310px,1fr));gap:12px;}
.node{background:#0d0d12;border:1px solid #22222c;border-radius:15px;padding:15px;display:flex;flex-direction:column;gap:11px;}
.node.off{opacity:0.6;border-color:#2c1b1b;}
.node.idflash{border-color:#f7931a;box-shadow:0 0 0 2px rgba(247,147,26,0.35);}
.node-top{display:flex;align-items:flex-start;justify-content:space-between;gap:8px;}
.node-name{font-size:15px;font-weight:800;color:#fff;word-break:break-word;}
.badges{display:flex;gap:5px;flex-wrap:wrap;margin-top:5px;}
.tag{font-size:10px;font-weight:700;padding:2px 7px;border-radius:6px;background:#181822;color:#d1d5db;border:1px solid #2a2a36;}
.tag.m{background:rgba(247,147,26,0.16);color:#f7931a;border-color:rgba(247,147,26,0.4);}
.tag.v{background:rgba(59,130,246,0.15);color:#60a5fa;border-color:rgba(59,130,246,0.35);}
.st-pill{display:inline-flex;align-items:center;gap:5px;font-size:11px;font-weight:700;padding:4px 8px;border-radius:999px;flex-shrink:0;}
.st-mine{color:#4ade80;background:rgba(34,197,94,0.13);border:1px solid rgba(34,197,94,0.3);}
.st-pause{color:#fbbf24;background:rgba(245,158,11,0.13);border:1px solid rgba(245,158,11,0.35);}
.st-conn{color:#60a5fa;background:rgba(59,130,246,0.13);border:1px solid rgba(59,130,246,0.35);}
.st-off{color:#f87171;background:rgba(239,68,68,0.13);border:1px solid rgba(239,68,68,0.35);}
.metrics{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;background:#08080c;border:1px solid #1a1a24;border-radius:11px;padding:10px;}
.met-k{font-size:10px;color:#6b7280;margin-bottom:2px;}
.met-v{font-size:13px;font-weight:700;color:#fff;}
.met-v.hi{color:#f7931a;}
.meta-line{font-size:11px;color:#6b7280;display:flex;justify-content:space-between;flex-wrap:wrap;gap:6px;}
.node-btns{display:flex;flex-wrap:wrap;gap:6px;}
.empty{text-align:center;padding:30px;color:#6b7280;font-size:13px;}
</style>
</head>
<body>
<div class="wrap">
  <div class="hdr">
    <div class="brand">
      <div class="btc-ico">&#8383;</div>
      <div>
        <h1>3dbrint Farm Kontrol</h1>
        <span>Tek Merkezli ESP32-C3 Madenci Yonetimi</span>
      </div>
    </div>
    <div class="hdr-right">
      <span class="badge" id="fw-ver">3dbrint Farm v2.1</span>
      <span class="live-pill" id="live-pill"><span class="dot"></span><span id="live-txt">Canli</span></span>
    </div>
  </div>

  <div id="toast" class="toast"></div>

  <div class="sum-grid">
    <div class="sum-card">
      <div class="sum-lbl">Toplam Hashrate</div>
      <div class="sum-val acc" id="sum-hr">0.00 kH/s</div>
      <div class="sum-sub" id="sum-pool">Havuz: --</div>
    </div>
    <div class="sum-card">
      <div class="sum-lbl">Aktif Cihazlar</div>
      <div class="sum-val" id="sum-cnt">0 / 0</div>
      <div class="sum-sub" id="sum-master">Master: 3dbrint.local</div>
    </div>
    <div class="sum-card">
      <div class="sum-lbl">Toplam Shares / Valid</div>
      <div class="sum-val" id="sum-sh">0 / 0</div>
      <div class="sum-sub">Havuz kabul paylari</div>
    </div>
    <div class="sum-card">
      <div class="sum-lbl">Farm Best Diff</div>
      <div class="sum-val" id="sum-bd">0.0</div>
      <div class="sum-sub">En yuksek zorluk rekoru</div>
    </div>
  </div>

  <div class="card">
    <div class="pool-bar">
      <div class="pool-info" id="wallet-bar">
        <div><strong>Havuz:</strong> <span id="lbl-pool">--</span> &nbsp;|&nbsp; <strong>Cuzdan:</strong> <span id="lbl-wallet">--</span></div>
      </div>
      <button type="button" id="btn-autojoin" class="btn btn-ok" onclick="toggleAutoJoin()">Oto-Katilim: ACIK</button>
    </div>

    <div class="actions-row">
      <button type="button" class="btn btn-warn" onclick="sendCmd('ALL','pause')">Tumunu Duraklat</button>
      <button type="button" class="btn btn-ok" onclick="sendCmd('ALL','resume')">Tumunu Baslat</button>
      <button type="button" class="btn" onclick="sendCmd('ALL','toggle_screen')">Ekran/LED Ac-Kapa (Tumu)</button>
      <button type="button" class="btn btn-red" onclick="if(confirm('Tum cihazlar yeniden baslatilsin mi?')) sendCmd('ALL','restart')">Tumunu Yeniden Baslat</button>
      <button type="button" class="btn" onclick="pruneOffline()">Cevrimdisilari Temizle</button>
    </div>
  </div>

  <details class="card cfg" id="cfg-box">
    <summary>
      <span>Havuz &amp; Cuzdan Ayarlari (Tum Farm'a Uygula)</span>
      <span style="font-size:12px;color:#f7931a;">Ac / Kapat &#9662;</span>
    </summary>
    <form onsubmit="return saveFarmConfig(event)">
      <div class="cfg-grid">
        <div class="fld">
          <label for="cfg-pool">Havuz Adresi (Pool URL)</label>
          <input id="cfg-pool" type="text" maxlength="78" placeholder="pool.nerdminer.io" oninput="markDirty()">
        </div>
        <div class="fld">
          <label for="cfg-port">Havuz Portu</label>
          <input id="cfg-port" type="number" min="1" max="65535" placeholder="3333" oninput="markDirty()">
        </div>
        <div class="fld">
          <label for="cfg-pass">Havuz Sifresi</label>
          <input id="cfg-pass" type="text" maxlength="78" placeholder="x" oninput="markDirty()">
        </div>
        <div class="fld">
          <label for="cfg-wallet">BTC Cuzdan Adresi</label>
          <input id="cfg-wallet" type="text" maxlength="78" placeholder="bc1q..." oninput="markDirty()">
        </div>
        <div class="fld">
          <label for="cfg-tz">Saat Dilimi (UTC Farki)</label>
          <input id="cfg-tz" type="number" min="-12" max="12" placeholder="3" oninput="markDirty()">
        </div>
      </div>
      <label class="chk-row" for="cfg-stats">
        <input id="cfg-stats" type="checkbox" onchange="markDirty()">
        <span>Madencilik istatistiklerini flash hafizaya kaydet</span>
      </label>
      <div class="actions-row" style="margin-top:14px;">
        <button type="submit" class="btn btn-pri">Ayarlari Kaydet ve Tum Farm'a Gonder</button>
      </div>
    </form>
  </details>

  <details class="card cfg" id="ota-box">
    <summary>
      <span>GitHub OTA &amp; Otomatik Guncelleme</span>
      <span style="font-size:12px;color:#38bdf8;">Ac / Kapat &#9662;</span>
    </summary>
    <div style="margin-top:14px;">
      <p style="font-size:13px;color:#9ca3af;line-height:1.6;margin-bottom:14px;">
        Cihazlar en son firmware surumunu GitHub reposundan (<strong>mefeoku/NerdMiner_Farm</strong>) otomatik ceker veya manuel olarak <code>.bin</code> dosyasi yukleyebilirsiniz.<br>
        <strong>Mevcut Farm Surumu:</strong> <span style="color:#f7931a;font-weight:bold;">v2.2.0</span>
      </p>
      <div id="ota-status-box" style="display:none;padding:12px;border-radius:10px;background:#14141c;border:1px solid #2a2a36;font-size:13px;margin-bottom:14px;line-height:1.6;"></div>
      <div class="actions-row">
        <button type="button" class="btn" style="border-color:#38bdf8;color:#38bdf8;" onclick="checkOtaUpdate()">GitHub Kontrol Et</button>
        <button type="button" class="btn btn-ok" id="btn-ota-this" style="display:none;" onclick="startThisOta()">Bu Master Cihazi Guncelle</button>
        <button type="button" class="btn btn-pri" onclick="if(confirm('Tum farm cihazlarina GitHub OTA guncellemesi gonderilsin mi?')) sendCmd('ALL','ota_update')">Tum Farm'i Guncelle (GitHub)</button>
        <a class="btn" href="/update" target="_blank">Manuel .BIN Yukle</a>
      </div>
    </div>
  </details>

  <div class="nodes-hdr">
    <h2>Bagli Cihazlar</h2>
    <span style="font-size:12px;color:#9ca3af;" id="nodes-sub">Guncelleniyor...</span>
  </div>
  <div id="nodes-grid" class="nodes-grid">
    <div class="empty">Cihaz verileri yukleniyor...</div>
  </div>
</div>

<script>
var cfgDirty = false;
var cfgLoaded = false;
var currentAutoJoin = true;
var inFlight = false;

function markDirty() { cfgDirty = true; }

function esc(s) {
  if (s === null || s === undefined) return '';
  return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;').replace(/'/g,'&#39;');
}

function showToast(msg, ok) {
  var el = document.getElementById('toast');
  el.textContent = msg;
  el.className = 'toast ' + (ok ? 'ok' : 'err');
  clearTimeout(window._tt);
  window._tt = setTimeout(function(){ el.className = 'toast'; }, 3500);
}

function fmtUp(sec) {
  sec = parseInt(sec || 0, 10);
  var d = Math.floor(sec / 86400);
  var h = Math.floor((sec % 86400) / 3600);
  var m = Math.floor((sec % 3600) / 60);
  var s = sec % 60;
  var pad = function(n){ return (n < 10 ? '0' : '') + n; };
  return (d > 0 ? d + 'g ' : '') + pad(h) + ':' + pad(m) + ':' + pad(s);
}

function fmtDiff(v) {
  var n = Number(v || 0);
  if (n <= 0) return '0.0';
  if (n >= 1e9) return (n / 1e9).toFixed(2) + 'G';
  if (n >= 1e6) return (n / 1e6).toFixed(2) + 'M';
  if (n >= 1e3) return (n / 1e3).toFixed(2) + 'K';
  if (n >= 1) return n.toFixed(2);
  return n.toFixed(4);
}

async function sendCmd(target, cmd, arg) {
  try {
    var r = await fetch('/api/farm/cmd', {
      method: 'POST',
      headers: {'Content-Type':'application/json'},
      body: JSON.stringify({target: target, cmd: cmd, arg: arg || ''})
    });
    var d = await r.json();
    if (d.ok) {
      showToast('Komut gonderildi: ' + cmd + (target === 'ALL' ? ' (Tumu)' : ''), true);
      setTimeout(fetchStatus, 350);
    } else {
      showToast('Hata: ' + (d.error || 'Bilinmeyen'), false);
    }
  } catch (e) {
    showToast('Baglanti hatasi!', false);
  }
}

function renameNode(mac, oldName) {
  var nv = prompt('Cihaz icin yeni isim girin (bosluksuz, max 20 karakter):', oldName || '');
  if (nv === null) return;
  nv = nv.trim();
  if (!nv) return;
  sendCmd(mac, 'rename', nv);
}

async function toggleAutoJoin() {
  var targetState = !currentAutoJoin;
  await sendCmd('ALL', targetState ? 'autojoin_on' : 'autojoin_off');
}

async function pruneOffline() {
  try {
    var r = await fetch('/api/farm/prune', { method: 'POST' });
    var d = await r.json();
    if (d.ok) {
      showToast('Cevrimdisi cihazlar temizlendi (' + (d.removed || 0) + ').', true);
      fetchStatus();
    }
  } catch (e) {
    showToast('Temizleme hatasi!', false);
  }
}

async function saveFarmConfig(ev) {
  ev.preventDefault();
  var body = {
    pool: document.getElementById('cfg-pool').value.trim(),
    port: parseInt(document.getElementById('cfg-port').value || '3333', 10),
    poolPass: document.getElementById('cfg-pass').value,
    wallet: document.getElementById('cfg-wallet').value.trim(),
    tz: parseInt(document.getElementById('cfg-tz').value || '3', 10),
    saveStats: document.getElementById('cfg-stats').checked
  };
  try {
    var r = await fetch('/api/farm/config', {
      method: 'POST',
      headers: {'Content-Type':'application/json'},
      body: JSON.stringify(body)
    });
    var d = await r.json();
    if (d.ok) {
      cfgDirty = false;
      showToast('Ayarlar kaydedildi ve tum cihazlara dagitiliyor!', true);
      fetchStatus();
    } else {
      showToast('Kaydetme hatasi!', false);
    }
  } catch (e) {
    showToast('Baglanti hatasi!', false);
  }
  return false;
}

function renderStatus(data) {
  document.getElementById('fw-ver').textContent = data.fwVersion || '3dbrint Farm';
  var s = data.summary || {};
  document.getElementById('sum-hr').textContent = Number(s.totalHashrate || 0).toFixed(2) + ' kH/s';
  document.getElementById('sum-cnt').textContent = (s.onlineCount || 0) + ' / ' + (s.totalCount || 0);
  document.getElementById('sum-sh').textContent = (s.totalShares || 0) + ' / ' + (s.totalValids || 0);
  document.getElementById('sum-bd').textContent = fmtDiff(s.bestDiff || 0);

  var c = data.farmConfig || {};
  document.getElementById('sum-pool').textContent = 'Havuz: ' + (c.pool || '--') + ':' + (c.port || 3333);
  document.getElementById('lbl-pool').textContent = (c.pool || '--') + ':' + (c.port || 3333);

  var wEl = document.getElementById('lbl-wallet');
  if (c.wallet && c.wallet.length > 0) {
    var shortW = c.wallet.length > 20 ? (c.wallet.slice(0,10) + '...' + c.wallet.slice(-6)) : c.wallet;
    if ((c.pool || '').indexOf('pool.nerdminer.io') !== -1) {
      wEl.innerHTML = '<a target="_blank" href="https://pool.nerdminer.io/#' + encodeURIComponent(c.wallet) + '">' + esc(shortW) + ' &#8599;</a>';
    } else {
      wEl.textContent = shortW;
    }
  } else {
    wEl.textContent = 'Tanimlanmadi!';
  }

  currentAutoJoin = (c.autoJoin !== false);
  var ajBtn = document.getElementById('btn-autojoin');
  ajBtn.textContent = 'Oto-Katilim (Yeni Cihaz): ' + (currentAutoJoin ? 'ACIK' : 'KAPALI');
  ajBtn.className = 'btn ' + (currentAutoJoin ? 'btn-ok' : 'btn-red');

  if (!cfgLoaded || !cfgDirty) {
    document.getElementById('cfg-pool').value = c.pool || 'pool.nerdminer.io';
    document.getElementById('cfg-port').value = c.port || 3333;
    document.getElementById('cfg-pass').value = c.poolPass || 'x';
    document.getElementById('cfg-wallet').value = c.wallet || '';
    document.getElementById('cfg-tz').value = (c.tz !== undefined) ? c.tz : 3;
    document.getElementById('cfg-stats').checked = !!c.saveStats;
    cfgLoaded = true;
  }

  var nodes = data.nodes || [];
  document.getElementById('nodes-sub').textContent = 'Toplam ' + nodes.length + ' kayitli cihaz';
  var grid = document.getElementById('nodes-grid');
  if (!nodes.length) {
    grid.innerHTML = '<div class="empty">Henuz cihaz bulunamadi.</div>';
    return;
  }

  var html = '';
  for (var i = 0; i < nodes.length; i++) {
    var n = nodes[i];
    var stCls = 'st-mine', stTxt = 'Kaziyor';
    if (!n.online) { stCls = 'st-off'; stTxt = 'Cevrimdisi'; }
    else if (n.paused) { stCls = 'st-pause'; stTxt = 'Duraklatildi'; }
    else if (n.state !== 2) { stCls = 'st-conn'; stTxt = 'Baglaniyor'; }

    var roleTag = n.isMaster ? '<span class="tag m">MASTER</span>' : '<span class="tag">WORKER</span>';
    var varTag = '<span class="tag v">' + esc(n.variant || 'ESP32-C3') + '</span>';
    var scrLbl = (n.variant === 'Ekransiz') ? (n.screenOn ? 'LED Acik' : 'LED Kapali') : (n.screenOn ? 'Ekran Acik' : 'Ekran Kapali');
    var nextBtn = (n.variant !== 'Ekransiz') ? '<button class="btn btn-sm" onclick="sendCmd(\'' + esc(n.mac) + '\',\'next_screen\')">Ekran Degistir</button>' : '';

    html += '<div class="node' + (n.online ? '' : ' off') + (n.identifying ? ' idflash' : '') + '">' +
      '<div class="node-top">' +
        '<div>' +
          '<div class="node-name">' + esc(n.name) + '</div>' +
          '<div class="badges">' + roleTag + varTag + '<span class="tag">v.' + esc(n.cfgVer || 1) + '</span></div>' +
        '</div>' +
        '<span class="st-pill ' + stCls + '"><span class="dot"></span>' + stTxt + '</span>' +
      '</div>' +
      '<div class="metrics">' +
        '<div><div class="met-k">Hashrate</div><div class="met-v hi">' + Number(n.hashrate || 0).toFixed(2) + ' kH/s</div></div>' +
        '<div><div class="met-k">Shares / Valid</div><div class="met-v">' + (n.shares || 0) + ' / ' + (n.valids || 0) + '</div></div>' +
        '<div><div class="met-k">Best Diff</div><div class="met-v">' + fmtDiff(n.bestDiff) + '</div></div>' +
        '<div><div class="met-k">Sicaklik</div><div class="met-v">' + (n.temp || 0) + ' &deg;C</div></div>' +
        '<div><div class="met-k">Ping / WiFi</div><div class="met-v">' + (n.ping || 0) + 'ms / ' + (n.rssi || 0) + '</div></div>' +
        '<div><div class="met-k">Uptime</div><div class="met-v">' + fmtUp(n.uptime) + '</div></div>' +
      '</div>' +
      '<div class="meta-line"><span>IP: ' + esc(n.ip) + '</span><span>MAC: ' + esc(n.mac) + '</span></div>' +
      '<div class="node-btns">' +
        (n.paused
          ? '<button class="btn btn-sm btn-ok" onclick="sendCmd(\'' + esc(n.mac) + '\',\'resume\')">Devam Et</button>'
          : '<button class="btn btn-sm btn-warn" onclick="sendCmd(\'' + esc(n.mac) + '\',\'pause\')">Duraklat</button>') +
        '<button class="btn btn-sm" onclick="sendCmd(\'' + esc(n.mac) + '\',\'toggle_screen\')">' + scrLbl + '</button>' +
        nextBtn +
        '<button class="btn btn-sm btn-pri" onclick="sendCmd(\'' + esc(n.mac) + '\',\'identify\')">Bul (5sn)</button>' +
        '<button class="btn btn-sm" onclick="renameNode(\'' + esc(n.mac) + '\',\'' + esc(n.name) + '\')">Ad Degistir</button>' +
        '<button class="btn btn-sm" onclick="if(confirm(\'Cihaz yeniden baslatilsin mi?\')) sendCmd(\'' + esc(n.mac) + '\',\'restart\')">Yeniden Baslat</button>' +
        '<button class="btn btn-sm btn-red" onclick="if(confirm(\'DIKKAT: Bu cihazin WiFi ve cuzdan ayarlari silinecek!\')) sendCmd(\'' + esc(n.mac) + '\',\'reset\')">Sifirla</button>' +
      '</div>' +
    '</div>';
  }
  grid.innerHTML = html;
}

async function fetchStatus() {
  if (inFlight) return;
  inFlight = true;
  try {
    var r = await fetch('/api/farm/status', { cache: 'no-store' });
    if (!r.ok) throw new Error('HTTP ' + r.status);
    var data = await r.json();
    renderStatus(data);
    var p = document.getElementById('live-pill');
    p.className = 'live-pill';
    document.getElementById('live-txt').textContent = 'Canli';
  } catch (e) {
    var p = document.getElementById('live-pill');
    p.className = 'live-pill err';
    document.getElementById('live-txt').textContent = 'Baglanti Bekleniyor';
  } finally {
    inFlight = false;
  }
}

async function checkOtaUpdate() {
  var b = document.getElementById('ota-status-box');
  var btn = document.getElementById('btn-ota-this');
  b.style.display = 'block';
  b.innerHTML = '<span style="color:#9ca3af;">GitHub surumu kontrol ediliyor...</span>';
  try {
    var r = await fetch('/api/ota/check', { method: 'POST' });
    var d = await r.json();
    if (d.hasUpdate) {
      b.innerHTML = '<span style="color:#4ade80;font-weight:bold;">&#9888; Yeni Surum Bulundu: v' + esc(d.latestVer) + '</span><br>' +
                    'Model: ' + esc(d.hwTitle || d.hwKey) + '<br>' +
                    'Notlar: ' + esc(d.latestNotes || '-') + '<br>' +
                    'Dosya URL: <code style="color:#38bdf8;font-size:11px;">' + esc(d.binUrl) + '</code>';
      btn.style.display = 'inline-block';
    } else {
      b.innerHTML = '<span style="color:#4ade80;">&#10004; Yaziliminiz guncel! (v' + esc(d.currentVer) + ' - ' + esc(d.hwTitle) + ')</span>';
      btn.style.display = 'none';
    }
  } catch(e) {
    b.innerHTML = '<span style="color:#f87171;">GitHub kontrol hatasi: ' + esc(e.message) + ' (Internet baglantisini kontrol edin)</span>';
  }
}

async function startThisOta() {
  if (!confirm('Bu cihazin firmware yazilimi GitHub uzerinden guncellenecek ve cihaz yeniden baslayacak. Onayliyor musunuz?')) return;
  var b = document.getElementById('ota-status-box');
  b.innerHTML = '<span style="color:#f7931a;font-weight:bold;">Guncelleme indiriliyor ve flash hafizaya yaziliyor... Lutfen bekleyin, cihaz birazdan yeniden baslayacak!</span>';
  try {
    var r = await fetch('/api/ota/start', { method: 'POST' });
    var res = await r.json();
    b.innerHTML = '<span style="color:#4ade80;">' + esc(res.msg || 'Guncelleme baslatildi') + '</span>';
  } catch(e) {
    b.innerHTML = '<span style="color:#f87171;">Guncelleme baslatma hatasi: ' + esc(e.message) + '</span>';
  }
}

fetchStatus();
setInterval(fetchStatus, 3000);
</script>
</body>
</html>)rawliteral";

#endif // _FARM_DASHBOARD_H_
