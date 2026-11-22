#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <mutex>

#include "farmManager.h"
#include "farmDashboard.h"
#include "githubOta.h"
#include "mining.h"
#include "monitor.h"
#include "wManager.h"
#include "drivers/displays/display.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"

extern TSettings Settings;
extern nvMemory nvMem;
extern monitor_data mMonitor;
extern void reset_configuration();

// ============================================================================
// ESP-NOW Zero-Touch Provisioning Paket Yapisi
// ============================================================================
#define FARM_ESPNOW_MAGIC 0x3DB71F01UL

static const uint8_t ESPNOW_XOR_KEY[16] = {
  0x3D, 0xB7, 0x1F, 0xA9, 0x52, 0xC4, 0x7E, 0x19,
  0x8B, 0x41, 0xD3, 0x6A, 0xF0, 0x28, 0x95, 0xE7
};

#pragma pack(push, 1)
struct FarmEspNowReq {
  uint32_t magic;
  uint8_t type;       // 1 = provisioning request
  uint8_t seq;
  uint8_t reqMac[6];
};

struct FarmEspNowResp {
  uint32_t magic;
  uint8_t type;       // 2 = provisioning response
  uint8_t crc8;
  char ssid[33];
  char wifiPass[65];
  char poolUrl[48];
  uint16_t poolPort;
  char poolPass[16];
  char wallet[66];
  int8_t timezone;
  uint8_t saveStats;
  uint8_t masterIp[4];
};
#pragma pack(pop)

static_assert(sizeof(FarmEspNowResp) <= 250, "FarmEspNowResp ESP-NOW 250 byte limitini asiyor");

static uint8_t calcCrc8(const uint8_t* data, size_t len) {
  uint8_t crc = 0x5A;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x80) crc = (crc << 1) ^ 0x07;
      else crc <<= 1;
    }
  }
  return crc;
}

static void xorRespPayload(FarmEspNowResp& resp) {
  uint8_t* raw = reinterpret_cast<uint8_t*>(&resp);
  const size_t startOff = 6; // magic (4) + type (1) + crc8 (1) haric
  for (size_t i = startOff; i < sizeof(FarmEspNowResp); i++) {
    raw[i] ^= ESPNOW_XOR_KEY[(i - startOff) % sizeof(ESPNOW_XOR_KEY)];
  }
}

// ============================================================================
// Farm Veri Yapilari ve Global Durum
// ============================================================================
struct FarmNode {
  bool used = false;
  bool isMaster = false;
  String mac;
  String ip;
  String name;
  String variant;
  double hashrate = 0.0;
  uint32_t shares = 0;
  uint32_t valids = 0;
  double bestDiff = 0.0;
  int temp = 0;
  uint32_t ping = 0;
  int rssi = 0;
  uint64_t uptime = 0;
  int state = 0;
  bool paused = false;
  bool screenOn = true;
  bool identifying = false;
  uint32_t cfgVer = 1;
  uint32_t lastSeenMs = 0;

  // Bekleyen komut kuyrugu (HTTP fallback)
  uint32_t pendingCmdId = 0;
  String pendingCmd;
  String pendingArg;
};

struct PendingConfigApply {
  bool hasUpdate = false;
  String pool;
  int port = 3333;
  String poolPass;
  String wallet;
  int tz = 3;
  bool saveStats = false;
  uint32_t cfgVer = 1;
};

static std::mutex g_farmMutex;
static FarmNode g_farmNodes[MAX_FARM_NODES];
static volatile FarmRole g_role = FARM_ROLE_NONE;
static IPAddress g_masterIp(0, 0, 0, 0);
static WebServer* g_farmServer = nullptr;
static WiFiUDP g_discUdp;
static WiFiUDP g_cmdUdp;
static bool g_discUdpActive = false;
static bool g_cmdUdpActive = false;
static bool g_espNowMasterActive = false;
static TaskHandle_t g_farmTaskHandle = nullptr;

static String g_myMac;
static uint32_t g_nextCmdId = 1;
static uint32_t g_lastExecCmdId = 0;
static uint32_t g_identifyUntilMs = 0;
static uint32_t g_lastBeaconMs = 0;
static volatile bool g_syncNowFlag = false;
static volatile bool g_pendingBecomeMaster = false;
static IPAddress g_pendingNewMasterIp(0, 0, 0, 0);
static bool g_pendingSwitchMaster = false;
static PendingConfigApply g_pendingCfg;

struct QueuedLocalCmd {
  bool pending = false;
  uint32_t cmdId = 0;
  String cmd;
  String arg;
};
static QueuedLocalCmd g_queuedLocalCmd;

static uint32_t g_pendingRestartAt = 0;
static uint32_t g_pendingResetAt = 0;

// ESP-NOW ISR tamponlari (asla callback icinde NVS/WiFi cagrisi yapma!)
static volatile bool g_provReqFlag = false;
static uint8_t g_provReqMac[6] = {0};

static volatile bool g_provRespReceived = false;
static FarmEspNowResp g_provRespBuf;

// ============================================================================
// Yardimci Fonksiyonlar
// ============================================================================
static void sanitizeDeviceName(const char* raw, char* out, size_t outSize, const String& macStr) {
  size_t idx = 0;
  if (raw) {
    for (size_t i = 0; raw[i] != '\0' && idx + 1 < outSize && idx < 20; i++) {
      char c = raw[i];
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_') {
        out[idx++] = c;
      }
    }
  }
  out[idx] = '\0';
  if (idx == 0) {
    String clean = macStr;
    clean.replace(":", "");
    String tail = (clean.length() >= 4) ? clean.substring(clean.length() - 4) : String("0001");
    snprintf(out, outSize, "Miner-%s", tail.c_str());
  }
}

String farmGetMacString() {
  if (g_myMac.length() == 0) {
    uint8_t m[6] = {0};
    WiFi.macAddress(m);
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
    g_myMac = String(buf);
  }
  return g_myMac;
}

static String farmGetShortMac() {
  String m = farmGetMacString();
  m.replace(":", "");
  if (m.length() >= 4) return m.substring(m.length() - 4);
  return String("0001");
}

void farmEnsureDeviceName(TSettings* cfg) {
  if (!cfg) return;
  String macStr = farmGetMacString();
  char clean[24];
  sanitizeDeviceName(cfg->DeviceName, clean, sizeof(clean), macStr);
  strlcpy(cfg->DeviceName, clean, sizeof(cfg->DeviceName));
}

const char* farmGetDeviceName() {
  if (Settings.DeviceName[0] == '\0') {
    farmEnsureDeviceName(&Settings);
  }
  return Settings.DeviceName;
}

FarmRole farmGetRole() {
  return g_role;
}

int farmGetOnlineCount() {
  if (g_role == FARM_ROLE_NONE) return 0;
  if (g_role == FARM_ROLE_WORKER) return 1;
  std::lock_guard<std::mutex> lock(g_farmMutex);
  int cnt = 1; // Master kendisi online
  uint32_t now = millis();
  for (int i = 0; i < MAX_FARM_NODES; i++) {
    if (g_farmNodes[i].used && !g_farmNodes[i].isMaster) {
      if ((now - g_farmNodes[i].lastSeenMs) < 25000UL) {
        cnt++;
      }
    }
  }
  return cnt;
}

bool farmIsIdentifying() {
  return (g_identifyUntilMs != 0 && (int32_t)(g_identifyUntilMs - millis()) > 0);
}

// ============================================================================
// ESP-NOW Callback & Master/Worker Mantigi
// ============================================================================
static void onEspNowRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (!info || !data || len < 4) return;
  uint32_t magic = 0;
  memcpy(&magic, data, 4);
  if (magic != FARM_ESPNOW_MAGIC) return;

  if (len == (int)sizeof(FarmEspNowReq) && data[4] == 1) {
    if (g_role == FARM_ROLE_MASTER && !g_provReqFlag) {
      memcpy(g_provReqMac, info->src_addr, 6);
      g_provReqFlag = true;
    }
  } else if (len == (int)sizeof(FarmEspNowResp) && data[4] == 2) {
    if (!g_provRespReceived) {
      memcpy(&g_provRespBuf, data, sizeof(FarmEspNowResp));
      g_provRespReceived = true;
    }
  }
}

static void startMasterEspNow() {
  if (g_espNowMasterActive) return;
  if (esp_now_init() == ESP_OK) {
    esp_now_register_recv_cb(onEspNowRecv);
    g_espNowMasterActive = true;
    Serial.println("[FARM] Master ESP-NOW Oto-Kurulum dinleyicisi baslatildi");
  }
}

static void stopMasterEspNow() {
  if (!g_espNowMasterActive) return;
  esp_now_unregister_recv_cb();
  esp_now_deinit();
  g_espNowMasterActive = false;
}

static void processMasterEspNowRequest() {
  if (!g_provReqFlag) return;
  uint8_t destMac[6];
  memcpy(destMac, g_provReqMac, 6);
  g_provReqFlag = false;

  if (!Settings.farmAutoJoin || WiFi.status() != WL_CONNECTED) {
    return;
  }

  wifi_config_t conf;
  memset(&conf, 0, sizeof(conf));
  if (esp_wifi_get_config(WIFI_IF_STA, &conf) != ESP_OK) {
    return;
  }
  if (conf.sta.ssid[0] == '\0') {
    return;
  }

  FarmEspNowResp resp;
  memset(&resp, 0, sizeof(resp));
  resp.magic = FARM_ESPNOW_MAGIC;
  resp.type = 2;
  resp.crc8 = 0;
  strlcpy(resp.ssid, reinterpret_cast<const char*>(conf.sta.ssid), sizeof(resp.ssid));
  strlcpy(resp.wifiPass, reinterpret_cast<const char*>(conf.sta.password), sizeof(resp.wifiPass));
  strlcpy(resp.poolUrl, Settings.PoolAddress.c_str(), sizeof(resp.poolUrl));
  resp.poolPort = (uint16_t)Settings.PoolPort;
  strlcpy(resp.poolPass, Settings.PoolPassword, sizeof(resp.poolPass));
  strlcpy(resp.wallet, Settings.BtcWallet, sizeof(resp.wallet));
  resp.timezone = (int8_t)Settings.Timezone;
  resp.saveStats = Settings.saveStats ? 1 : 0;
  IPAddress ip = WiFi.localIP();
  for (int i = 0; i < 4; i++) resp.masterIp[i] = ip[i];

  resp.crc8 = calcCrc8(reinterpret_cast<const uint8_t*>(&resp), sizeof(resp));
  xorRespPayload(resp);

  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, destMac, 6);
  peer.channel = 0; // Master'in mevcut STA kanalini kullan
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;

  esp_now_del_peer(destMac);
  if (esp_now_add_peer(&peer) == ESP_OK) {
    esp_now_send(destMac, reinterpret_cast<const uint8_t*>(&resp), sizeof(resp));
    delay(25);
    esp_now_send(destMac, reinterpret_cast<const uint8_t*>(&resp), sizeof(resp));
    delay(20);
    esp_now_del_peer(destMac);
    Serial.printf("[FARM] ESP-NOW ile yeni cihaza WiFi/Farm bilgisi gonderildi (%02X:%02X:%02X:%02X:%02X:%02X)\n",
                  destMac[0], destMac[1], destMac[2], destMac[3], destMac[4], destMac[5]);
  }
}

bool farmTryEspNowProvision(TSettings* cfg, String& outSsid, String& outPass, IPAddress& outMasterIp) {
  if (!cfg) return false;

  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.setSleep(false);
  WiFi.disconnect(false, false);
  delay(80);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[FARM] ESP-NOW baslatilamadi");
    return false;
  }

  g_provRespReceived = false;
  esp_now_register_recv_cb(onEspNowRecv);

  static const uint8_t bcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t bcastPeer;
  memset(&bcastPeer, 0, sizeof(bcastPeer));
  memcpy(bcastPeer.peer_addr, bcastMac, 6);
  bcastPeer.channel = 0;
  bcastPeer.ifidx = WIFI_IF_STA;
  bcastPeer.encrypt = false;
  if (esp_now_add_peer(&bcastPeer) != ESP_OK) {
    esp_now_unregister_recv_cb();
    esp_now_deinit();
    return false;
  }

  FarmEspNowReq req;
  req.magic = FARM_ESPNOW_MAGIC;
  req.type = 1;
  req.seq = 0;
  WiFi.macAddress(req.reqMac);

  bool success = false;
  for (uint8_t ch = 1; ch <= 13; ch++) {
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    delay(10);
    req.seq = ch;
    esp_now_send(bcastMac, reinterpret_cast<const uint8_t*>(&req), sizeof(req));
    for (int t = 0; t < 6 && !g_provRespReceived; t++) delay(10);

    if (!g_provRespReceived) {
      esp_now_send(bcastMac, reinterpret_cast<const uint8_t*>(&req), sizeof(req));
      for (int t = 0; t < 11 && !g_provRespReceived; t++) delay(10);
    }

    if (g_provRespReceived) {
      FarmEspNowResp copy = g_provRespBuf;
      xorRespPayload(copy);
      uint8_t recvCrc = copy.crc8;
      copy.crc8 = 0;
      uint8_t expected = calcCrc8(reinterpret_cast<const uint8_t*>(&copy), sizeof(copy));
      if (copy.magic == FARM_ESPNOW_MAGIC && copy.type == 2 && recvCrc == expected && copy.ssid[0] != '\0') {
        outSsid = String(copy.ssid);
        outPass = String(copy.wifiPass);
        outMasterIp = IPAddress(copy.masterIp[0], copy.masterIp[1], copy.masterIp[2], copy.masterIp[3]);

        if (copy.poolUrl[0] != '\0') {
          cfg->PoolAddress = String(copy.poolUrl);
        }
        if (copy.poolPort > 0) {
          cfg->PoolPort = copy.poolPort;
        }
        strlcpy(cfg->PoolPassword, copy.poolPass, sizeof(cfg->PoolPassword));
        strlcpy(cfg->BtcWallet, copy.wallet, sizeof(cfg->BtcWallet));
        cfg->Timezone = copy.timezone;
        cfg->saveStats = (copy.saveStats != 0);

        Serial.printf("[FARM] Kanal %d uzerinden Master (%s) tarafindan otomatik ayar alindi! SSID: %s\n",
                      ch, outMasterIp.toString().c_str(), outSsid.c_str());
        success = true;
        break;
      } else {
        // Gecersiz veya CRC hatali paket, aramaya devam
        g_provRespReceived = false;
      }
    }
  }

  esp_now_unregister_recv_cb();
  esp_now_deinit();
  return success;
}

// ============================================================================
// Yerel Komut Yurutme (Hem Master Hem Worker)
// ============================================================================
static void executeLocalCmd(const String& cmd, const String& arg) {
  if (cmd == "pause") {
    is_mining_paused_flag = true;
    Serial.println("[FARM-CMD] Madencilik duraklatildi");
  } else if (cmd == "resume") {
    is_mining_paused_flag = false;
    Serial.println("[FARM-CMD] Madencilik devam ediyor");
  } else if (cmd == "toggle_screen") {
    alternateScreenState();
  } else if (cmd == "next_screen") {
    switchToNextScreen();
  } else if (cmd == "identify") {
    g_identifyUntilMs = millis() + 5000UL;
    Serial.println("[FARM-CMD] Cihaz tanitiliyor (5sn)");
  } else if (cmd == "rename") {
    char clean[24];
    sanitizeDeviceName(arg.c_str(), clean, sizeof(clean), farmGetMacString());
    strlcpy(Settings.DeviceName, clean, sizeof(Settings.DeviceName));
    nvMem.saveConfig(&Settings);
    Serial.printf("[FARM-CMD] Yeni cihaz adi: %s\n", Settings.DeviceName);
  } else if (cmd == "restart") {
    g_pendingRestartAt = millis() + 600UL;
    Serial.println("[FARM-CMD] Cihaz yeniden baslatilacak...");
  } else if (cmd == "reset") {
    g_pendingResetAt = millis() + 600UL;
    Serial.println("[FARM-CMD] Cihaz fabrika ayarlarina dondurulecek...");
  } else if (cmd == "ota_update") {
    Serial.println("[FARM-CMD] GitHub OTA guncellemesi tetiklendi...");
    xTaskCreate([](void* p) {
      String nv, bu, nt;
      bool hu = false;
      if (otaCheckUpdate(nv, bu, nt, hu) && bu.length() > 0) {
        otaPerformGithubUpdate(bu);
      }
      vTaskDelete(NULL);
    }, "ota_worker_task", 8192, NULL, 5, NULL);
  }
}

// ============================================================================
// Ag Uzerinde Master Kesfi (HTTP Ping + UDP Broadcast + mDNS)
// ============================================================================
static bool probeMasterHttp(IPAddress ip) {
  if (ip == IPAddress(0, 0, 0, 0) || ip == WiFi.localIP()) return false;
  WiFiClient wc;
  HTTPClient http;
  http.setConnectTimeout(1200);
  http.setTimeout(1200);
  String url = String("http://") + ip.toString() + "/api/farm/ping";
  if (!http.begin(wc, url)) return false;
  int code = http.GET();
  bool ok = false;
  if (code == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    if (!deserializeJson(doc, payload)) {
      const char* r = doc["role"] | "";
      if (strcmp(r, "master") == 0) {
        ok = true;
      }
    }
  }
  http.end();
  return ok;
}

static IPAddress discoverMasterOnLan(IPAddress hintIp, bool initTempMdns) {
  if (hintIp != IPAddress(0, 0, 0, 0) && hintIp != WiFi.localIP()) {
    if (probeMasterHttp(hintIp)) {
      Serial.printf("[FARM] Master hint IP ile dogrulandi: %s\n", hintIp.toString().c_str());
      return hintIp;
    }
  }

  // 1) UDP Broadcast ile "3DBRINT_WHO_IS_MASTER" sor (mDNS calismayan modemlerde bile aninda bulur)
  WiFiUDP udp;
  if (udp.begin(0)) {
    for (int attempt = 0; attempt < 3; attempt++) {
      udp.beginPacket(IPAddress(255, 255, 255, 255), FARM_UDP_DISCOVERY_PORT);
      udp.print("3DBRINT_WHO_IS_MASTER");
      udp.endPacket();

      uint32_t waitStart = millis();
      while (millis() - waitStart < 300) {
        int pkt = udp.parsePacket();
        if (pkt > 0) {
          char buf[128];
          int n = udp.read(buf, sizeof(buf) - 1);
          if (n > 0) {
            buf[n] = '\0';
            String msg(buf);
            if (msg.startsWith("3DBRINT_MASTER|")) {
              int p1 = msg.indexOf('|');
              int p2 = msg.indexOf('|', p1 + 1);
              String ipStr = (p2 > p1) ? msg.substring(p1 + 1, p2) : msg.substring(p1 + 1);
              IPAddress cand;
              if (cand.fromString(ipStr) && cand != WiFi.localIP() && probeMasterHttp(cand)) {
                udp.stop();
                Serial.printf("[FARM] UDP ile Master bulundu: %s\n", cand.toString().c_str());
                return cand;
              }
            }
          }
        }
        delay(10);
      }
    }
    udp.stop();
  }

  // 2) mDNS ile "3dbrint.local" sorgula
  if (initTempMdns) {
    MDNS.end();
    String host = "3dbrint-" + farmGetShortMac();
    MDNS.begin(host.c_str());
  }
  IPAddress mdnsIp = MDNS.queryHost("3dbrint", 1500);
  if (mdnsIp != IPAddress(0, 0, 0, 0) && mdnsIp != WiFi.localIP()) {
    if (probeMasterHttp(mdnsIp)) {
      Serial.printf("[FARM] mDNS ile Master bulundu: %s\n", mdnsIp.toString().c_str());
      return mdnsIp;
    }
  }

  return IPAddress(0, 0, 0, 0);
}

// ============================================================================
// Master HTTP & UDP Isleyicileri
// ============================================================================
static void fillSelfNode(FarmNode& selfNode) {
  selfNode.used = true;
  selfNode.isMaster = true;
  selfNode.mac = farmGetMacString();
  selfNode.ip = WiFi.localIP().toString();
  selfNode.name = farmGetDeviceName();
  selfNode.variant = FARM_VARIANT_NAME;
  selfNode.hashrate = is_mining_paused_flag ? 0.0 : g_lastHashrateKHs;
  selfNode.shares = shares;
  selfNode.valids = valids;
  selfNode.bestDiff = best_diff;
  selfNode.temp = (int)temperatureRead();
  selfNode.ping = pool_ping_ms;
  selfNode.rssi = WiFi.RSSI();
  selfNode.uptime = upTime;
  selfNode.state = (int)mMonitor.NerdStatus;
  selfNode.paused = is_mining_paused_flag;
  selfNode.screenOn = getDisplayPowerState();
  selfNode.identifying = farmIsIdentifying();
  selfNode.cfgVer = Settings.farmConfigVer;
  selfNode.lastSeenMs = millis();
}

static void sendUdpCmdToWorker(const String& ipStr, const String& macStr, const String& cmd, const String& arg, uint32_t cmdId) {
  IPAddress dst;
  if (!dst.fromString(ipStr) || dst == IPAddress(0, 0, 0, 0)) return;
  if (!g_discUdpActive) return;
  g_discUdp.beginPacket(dst, FARM_UDP_CMD_PORT);
  g_discUdp.printf("3DBRINT_CMD|%s|%s|%s|%lu", macStr.c_str(), cmd.c_str(), arg.c_str(), (unsigned long)cmdId);
  g_discUdp.endPacket();
}

static void broadcastUdpSync() {
  if (!g_discUdpActive) return;
  g_discUdp.beginPacket(IPAddress(255, 255, 255, 255), FARM_UDP_CMD_PORT);
  g_discUdp.print("3DBRINT_SYNC");
  g_discUdp.endPacket();
}

static void sendMasterBeacon() {
  if (!g_discUdpActive || WiFi.status() != WL_CONNECTED) return;
  g_discUdp.beginPacket(IPAddress(255, 255, 255, 255), FARM_UDP_DISCOVERY_PORT);
  g_discUdp.printf("3DBRINT_MASTER_BEACON|%s|%s", WiFi.localIP().toString().c_str(), farmGetMacString().c_str());
  g_discUdp.endPacket();
  g_lastBeaconMs = millis();
}

static void handleApiPing() {
  if (!g_farmServer) return;
  JsonDocument doc;
  doc["ok"] = true;
  doc["role"] = (g_role == FARM_ROLE_MASTER) ? "master" : "worker";
  doc["mac"] = farmGetMacString();
  doc["ip"] = WiFi.localIP().toString();
  doc["name"] = farmGetDeviceName();
  doc["variant"] = FARM_VARIANT_NAME;
  doc["version"] = FARM_FW_VERSION;
  doc["configVer"] = Settings.farmConfigVer;
  doc["autoJoin"] = Settings.farmAutoJoin;
  String out;
  serializeJson(doc, out);
  g_farmServer->sendHeader(F("Cache-Control"), F("no-store"));
  g_farmServer->send(200, F("application/json; charset=utf-8"), out);
}

static void handleMasterRoot() {
  if (!g_farmServer) return;
  g_farmServer->sendHeader(F("Cache-Control"), F("no-cache"));
  g_farmServer->send_P(200, "text/html; charset=utf-8", FARM_DASHBOARD_HTML);
}

static void handleWorkerRoot() {
  if (!g_farmServer) return;
  String html = FPSTR(FARM_WORKER_HTML);
  html.replace(F("{{MASTER_IP}}"), g_masterIp.toString());
  html.replace(F("{{DEV_NAME}}"), String(farmGetDeviceName()));
  g_farmServer->sendHeader(F("Cache-Control"), F("no-cache"));
  g_farmServer->send(200, F("text/html; charset=utf-8"), html);
}

static void handleMasterStatus() {
  if (!g_farmServer) return;
  JsonDocument doc;
  doc["fwVersion"] = FARM_FW_VERSION;

  JsonObject cfg = doc["farmConfig"].to<JsonObject>();
  cfg["pool"] = Settings.PoolAddress;
  cfg["port"] = Settings.PoolPort;
  cfg["poolPass"] = Settings.PoolPassword;
  cfg["wallet"] = Settings.BtcWallet;
  cfg["tz"] = Settings.Timezone;
  cfg["saveStats"] = Settings.saveStats;
  cfg["autoJoin"] = Settings.farmAutoJoin;
  cfg["configVer"] = Settings.farmConfigVer;

  JsonArray arr = doc["nodes"].to<JsonArray>();

  double totalHr = 0.0;
  uint64_t totalSh = 0;
  uint64_t totalVal = 0;
  double maxDiff = 0.0;
  int onlineCnt = 0;
  int totalCnt = 0;

  FarmNode selfNode;
  fillSelfNode(selfNode);

  auto appendNode = [&](const FarmNode& n, bool online) {
    JsonObject obj = arr.add<JsonObject>();
    obj["isMaster"] = n.isMaster;
    obj["online"] = online;
    obj["mac"] = n.mac;
    obj["ip"] = n.ip;
    obj["name"] = n.name;
    obj["variant"] = n.variant;
    obj["hashrate"] = online ? n.hashrate : 0.0;
    obj["shares"] = n.shares;
    obj["valids"] = n.valids;
    obj["bestDiff"] = n.bestDiff;
    obj["temp"] = n.temp;
    obj["ping"] = n.ping;
    obj["rssi"] = n.rssi;
    obj["uptime"] = n.uptime;
    obj["state"] = n.state;
    obj["paused"] = n.paused;
    obj["screenOn"] = n.screenOn;
    obj["identifying"] = n.identifying;
    obj["cfgVer"] = n.cfgVer;

    totalCnt++;
    if (online) {
      onlineCnt++;
      totalHr += n.hashrate;
    }
    totalSh += n.shares;
    totalVal += n.valids;
    if (n.bestDiff > maxDiff) maxDiff = n.bestDiff;
  };

  appendNode(selfNode, true);

  {
    std::lock_guard<std::mutex> lock(g_farmMutex);
    uint32_t now = millis();
    for (int i = 0; i < MAX_FARM_NODES; i++) {
      if (g_farmNodes[i].used && !g_farmNodes[i].isMaster) {
        bool isOnline = ((now - g_farmNodes[i].lastSeenMs) < 25000UL);
        appendNode(g_farmNodes[i], isOnline);
      }
    }
  }

  JsonObject sum = doc["summary"].to<JsonObject>();
  sum["totalHashrate"] = totalHr;
  sum["onlineCount"] = onlineCnt;
  sum["totalCount"] = totalCnt;
  sum["totalShares"] = totalSh;
  sum["totalValids"] = totalVal;
  sum["bestDiff"] = maxDiff;

  String out;
  serializeJson(doc, out);
  g_farmServer->sendHeader(F("Cache-Control"), F("no-store"));
  g_farmServer->send(200, F("application/json; charset=utf-8"), out);
}

static void handleMasterReport() {
  if (!g_farmServer) return;
  String body = g_farmServer->arg("plain");
  JsonDocument inDoc;
  if (deserializeJson(inDoc, body)) {
    g_farmServer->send(400, F("application/json"), F("{\"ok\":false}"));
    return;
  }

  String mac = inDoc["mac"] | "";
  if (mac.length() == 0) {
    g_farmServer->send(400, F("application/json"), F("{\"ok\":false}"));
    return;
  }

  uint32_t ackCmdId = inDoc["ackCmdId"] | 0;
  uint32_t outCmdId = 0;
  String outCmd;
  String outArg;

  {
    std::lock_guard<std::mutex> lock(g_farmMutex);
    int slot = -1;
    int emptySlot = -1;
    int oldestSlot = 0;
    uint32_t maxAge = 0;
    uint32_t now = millis();

    for (int i = 0; i < MAX_FARM_NODES; i++) {
      if (g_farmNodes[i].used) {
        if (g_farmNodes[i].mac == mac) {
          slot = i;
          break;
        }
        uint32_t age = now - g_farmNodes[i].lastSeenMs;
        if (age >= maxAge) {
          maxAge = age;
          oldestSlot = i;
        }
      } else if (emptySlot == -1) {
        emptySlot = i;
      }
    }

    if (slot == -1) {
      slot = (emptySlot != -1) ? emptySlot : oldestSlot;
      g_farmNodes[slot] = FarmNode();
      g_farmNodes[slot].used = true;
      g_farmNodes[slot].mac = mac;
    }

    FarmNode& n = g_farmNodes[slot];
    n.isMaster = false;
    n.ip = inDoc["ip"] | g_farmServer->client().remoteIP().toString();
    n.name = inDoc["name"] | mac;
    n.variant = inDoc["variant"] | "ESP32-C3";
    n.hashrate = inDoc["hashrate"] | 0.0;
    n.shares = inDoc["shares"] | 0;
    n.valids = inDoc["valids"] | 0;
    n.bestDiff = inDoc["bestDiff"] | 0.0;
    n.temp = inDoc["temp"] | 0;
    n.ping = inDoc["ping"] | 0;
    n.rssi = inDoc["rssi"] | 0;
    n.uptime = inDoc["uptime"] | 0;
    n.state = inDoc["state"] | 0;
    n.paused = inDoc["paused"] | false;
    n.screenOn = inDoc["screenOn"] | true;
    n.identifying = inDoc["identifying"] | false;
    n.cfgVer = inDoc["cfgVer"] | 1;
    n.lastSeenMs = millis();

    if (ackCmdId != 0 && ackCmdId == n.pendingCmdId) {
      n.pendingCmdId = 0;
      n.pendingCmd = "";
      n.pendingArg = "";
    }
    if (n.pendingCmdId != 0) {
      outCmdId = n.pendingCmdId;
      outCmd = n.pendingCmd;
      outArg = n.pendingArg;
    }
  }

  JsonDocument resp;
  resp["ok"] = true;
  resp["configVer"] = Settings.farmConfigVer;
  resp["pool"] = Settings.PoolAddress;
  resp["port"] = Settings.PoolPort;
  resp["poolPass"] = Settings.PoolPassword;
  resp["wallet"] = Settings.BtcWallet;
  resp["tz"] = Settings.Timezone;
  resp["saveStats"] = Settings.saveStats;

  if (outCmdId != 0) {
    resp["cmdId"] = outCmdId;
    resp["cmd"] = outCmd;
    resp["arg"] = outArg;
  }

  String out;
  serializeJson(resp, out);
  g_farmServer->sendHeader(F("Cache-Control"), F("no-store"));
  g_farmServer->send(200, F("application/json; charset=utf-8"), out);
}

static void handleMasterCmd() {
  if (!g_farmServer) return;
  JsonDocument doc;
  if (deserializeJson(doc, g_farmServer->arg("plain"))) {
    g_farmServer->send(400, F("application/json"), F("{\"ok\":false,\"error\":\"Gecersiz JSON\"}"));
    return;
  }

  String target = doc["target"] | "";
  String cmd = doc["cmd"] | "";
  String arg = doc["arg"] | "";

  if (cmd.length() == 0) {
    g_farmServer->send(400, F("application/json"), F("{\"ok\":false,\"error\":\"Bos komut\"}"));
    return;
  }

  if (cmd == "autojoin_on" || cmd == "autojoin_off") {
    Settings.farmAutoJoin = (cmd == "autojoin_on");
    nvMem.saveConfig(&Settings);
    g_farmServer->send(200, F("application/json"), F("{\"ok\":true}"));
    return;
  }

  String myMac = farmGetMacString();
  if (target == "ALL" || target == myMac) {
    executeLocalCmd(cmd, arg);
  }

  if (target != myMac) {
    std::lock_guard<std::mutex> lock(g_farmMutex);
    for (int i = 0; i < MAX_FARM_NODES; i++) {
      if (!g_farmNodes[i].used || g_farmNodes[i].isMaster) continue;
      if (target == "ALL" || g_farmNodes[i].mac == target) {
        uint32_t cid = ++g_nextCmdId;
        g_farmNodes[i].pendingCmdId = cid;
        g_farmNodes[i].pendingCmd = cmd;
        g_farmNodes[i].pendingArg = arg;
        if (cmd == "rename") {
          char clean[24];
          sanitizeDeviceName(arg.c_str(), clean, sizeof(clean), g_farmNodes[i].mac);
          g_farmNodes[i].name = String(clean);
        } else if (cmd == "pause") {
          g_farmNodes[i].paused = true;
        } else if (cmd == "resume") {
          g_farmNodes[i].paused = false;
        } else if (cmd == "identify") {
          g_farmNodes[i].identifying = true;
        }
        sendUdpCmdToWorker(g_farmNodes[i].ip, g_farmNodes[i].mac, cmd, arg, cid);
      }
    }
  }

  g_farmServer->sendHeader(F("Cache-Control"), F("no-store"));
  g_farmServer->send(200, F("application/json"), F("{\"ok\":true}"));
}

static void handleMasterConfig() {
  if (!g_farmServer) return;
  JsonDocument doc;
  if (deserializeJson(doc, g_farmServer->arg("plain"))) {
    g_farmServer->send(400, F("application/json"), F("{\"ok\":false}"));
    return;
  }

  String pool = doc["pool"] | Settings.PoolAddress;
  pool.trim();
  if (pool.length() == 0) pool = DEFAULT_POOLURL;
  int port = doc["port"] | Settings.PoolPort;
  if (port <= 0 || port > 65535) port = DEFAULT_POOLPORT;
  String poolPass = doc["poolPass"] | String(Settings.PoolPassword);
  String wallet = doc["wallet"] | String(Settings.BtcWallet);
  wallet.trim();
  int tz = doc["tz"] | Settings.Timezone;
  bool saveSt = doc["saveStats"] | Settings.saveStats;

  Settings.PoolAddress = pool;
  Settings.PoolPort = port;
  strlcpy(Settings.PoolPassword, poolPass.c_str(), sizeof(Settings.PoolPassword));
  strlcpy(Settings.BtcWallet, wallet.c_str(), sizeof(Settings.BtcWallet));
  Settings.Timezone = tz;
  Settings.saveStats = saveSt;
  Settings.farmConfigVer++;

  nvMem.saveConfig(&Settings);
  configTime(3600 * Settings.Timezone, 0, "pool.ntp.org", "time.google.com");
  g_stratumReconnectNeeded = true;
  broadcastUdpSync();

  g_farmServer->sendHeader(F("Cache-Control"), F("no-store"));
  g_farmServer->send(200, F("application/json"), F("{\"ok\":true}"));
}

static void handleMasterPrune() {
  if (!g_farmServer) return;
  int removed = 0;
  {
    std::lock_guard<std::mutex> lock(g_farmMutex);
    uint32_t now = millis();
    for (int i = 0; i < MAX_FARM_NODES; i++) {
      if (g_farmNodes[i].used && !g_farmNodes[i].isMaster) {
        if ((now - g_farmNodes[i].lastSeenMs) >= 25000UL) {
          g_farmNodes[i] = FarmNode();
          removed++;
        }
      }
    }
  }
  JsonDocument resp;
  resp["ok"] = true;
  resp["removed"] = removed;
  String out;
  serializeJson(resp, out);
  g_farmServer->sendHeader(F("Cache-Control"), F("no-store"));
  g_farmServer->send(200, F("application/json"), out);
}

// ============================================================================
// Rol Gecisleri (Master <-> Worker)
// ============================================================================
static void teardownNetworkServices() {
  if (g_farmServer) {
    g_farmServer->stop();
    delete g_farmServer;
    g_farmServer = nullptr;
  }
  if (g_discUdpActive) {
    g_discUdp.stop();
    g_discUdpActive = false;
  }
  if (g_cmdUdpActive) {
    g_cmdUdp.stop();
    g_cmdUdpActive = false;
  }
  stopMasterEspNow();
}

static void farmBecomeMaster() {
  teardownNetworkServices();
  g_role = FARM_ROLE_MASTER;
  g_masterIp = WiFi.localIP();

  MDNS.end();
  if (MDNS.begin("3dbrint")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[FARM] MASTER aktif: http://3dbrint.local");
  }

  g_farmServer = new WebServer(80);
  g_farmServer->on("/", HTTP_GET, handleMasterRoot);
  g_farmServer->on("/api/farm/ping", HTTP_GET, handleApiPing);
  g_farmServer->on("/api/farm/status", HTTP_GET, handleMasterStatus);
  g_farmServer->on("/api/farm/report", HTTP_POST, handleMasterReport);
  g_farmServer->on("/api/farm/cmd", HTTP_POST, handleMasterCmd);
  g_farmServer->on("/api/farm/config", HTTP_POST, handleMasterConfig);
  g_farmServer->on("/api/farm/prune", HTTP_POST, handleMasterPrune);
  g_farmServer->onNotFound([]() {
    if (!g_farmServer) return;
    g_farmServer->sendHeader(F("Location"), F("/"), true);
    g_farmServer->send(302, F("text/plain"), F(""));
  });
  otaRegisterRoutes(g_farmServer);
  g_farmServer->begin();

  if (g_discUdp.begin(FARM_UDP_DISCOVERY_PORT)) {
    g_discUdpActive = true;
  }
  startMasterEspNow();
  sendMasterBeacon();
  Serial.printf("[FARM] Master IP: %s\n", WiFi.localIP().toString().c_str());
}

static void farmBecomeWorker(IPAddress masterIp) {
  teardownNetworkServices();
  g_role = FARM_ROLE_WORKER;
  g_masterIp = masterIp;

  MDNS.end();
  String host = "3dbrint-" + farmGetShortMac();
  if (MDNS.begin(host.c_str())) {
    MDNS.addService("http", "tcp", 80);
  }

  g_farmServer = new WebServer(80);
  g_farmServer->on("/", HTTP_GET, handleWorkerRoot);
  g_farmServer->on("/api/farm/ping", HTTP_GET, handleApiPing);
  g_farmServer->onNotFound(handleWorkerRoot);
  otaRegisterRoutes(g_farmServer);
  g_farmServer->begin();

  if (g_cmdUdp.begin(FARM_UDP_CMD_PORT)) {
    g_cmdUdpActive = true;
  }

  g_syncNowFlag = true;
  Serial.printf("[FARM] WORKER aktif! Master: %s | Yerel: http://%s.local\n",
                g_masterIp.toString().c_str(), host.c_str());
}

// ============================================================================
// Worker Arka Plan Telemetri & Failover Gorevi
// ============================================================================
static void runFarmSyncTask(void* param) {
  int failCount = 0;
  uint32_t lastSendMs = 0;

  while (true) {
    if (g_role == FARM_ROLE_WORKER && WiFi.status() == WL_CONNECTED && !g_pendingBecomeMaster && !g_pendingSwitchMaster) {
      uint32_t now = millis();
      if (g_syncNowFlag || (now - lastSendMs >= 5000UL)) {
        g_syncNowFlag = false;
        lastSendMs = now;

        JsonDocument doc;
        doc["mac"] = farmGetMacString();
        doc["ip"] = WiFi.localIP().toString();
        doc["name"] = farmGetDeviceName();
        doc["variant"] = FARM_VARIANT_NAME;
        doc["hashrate"] = is_mining_paused_flag ? 0.0 : g_lastHashrateKHs;
        doc["shares"] = shares;
        doc["valids"] = valids;
        doc["bestDiff"] = best_diff;
        doc["temp"] = (int)temperatureRead();
        doc["ping"] = pool_ping_ms;
        doc["rssi"] = WiFi.RSSI();
        doc["uptime"] = upTime;
        doc["state"] = (int)mMonitor.NerdStatus;
        doc["paused"] = is_mining_paused_flag;
        doc["screenOn"] = getDisplayPowerState();
        doc["identifying"] = farmIsIdentifying();
        doc["cfgVer"] = Settings.farmConfigVer;
        doc["ackCmdId"] = g_lastExecCmdId;

        String payload;
        serializeJson(doc, payload);

        WiFiClient wc;
        HTTPClient http;
        http.setConnectTimeout(1500);
        http.setTimeout(1500);
        String url = String("http://") + g_masterIp.toString() + "/api/farm/report";
        bool ok = false;
        if (http.begin(wc, url)) {
          http.addHeader(F("Content-Type"), F("application/json"));
          int code = http.POST(payload);
          if (code == HTTP_CODE_OK) {
            ok = true;
            failCount = 0;
            String respStr = http.getString();
            JsonDocument rDoc;
            if (!deserializeJson(rDoc, respStr)) {
              uint32_t mVer = rDoc["configVer"] | 1;
              const char* mWallet = rDoc["wallet"] | "";
              bool needCfg = (mVer != Settings.farmConfigVer) ||
                             (Settings.BtcWallet[0] == '\0' && mWallet[0] != '\0');
              uint32_t cid = rDoc["cmdId"] | 0;
              const char* cmdStr = rDoc["cmd"] | "";
              const char* argStr = rDoc["arg"] | "";

              if (needCfg || (cid != 0 && cid != g_lastExecCmdId && cmdStr[0] != '\0')) {
                std::lock_guard<std::mutex> lock(g_farmMutex);
                if (needCfg) {
                  g_pendingCfg.hasUpdate = true;
                  g_pendingCfg.pool = rDoc["pool"] | Settings.PoolAddress;
                  g_pendingCfg.port = rDoc["port"] | Settings.PoolPort;
                  g_pendingCfg.poolPass = rDoc["poolPass"] | String(Settings.PoolPassword);
                  g_pendingCfg.wallet = String(mWallet);
                  g_pendingCfg.tz = rDoc["tz"] | Settings.Timezone;
                  g_pendingCfg.saveStats = rDoc["saveStats"] | Settings.saveStats;
                  g_pendingCfg.cfgVer = mVer;
                }
                if (cid != 0 && cid != g_lastExecCmdId && cmdStr[0] != '\0') {
                  g_queuedLocalCmd.pending = true;
                  g_queuedLocalCmd.cmdId = cid;
                  g_queuedLocalCmd.cmd = String(cmdStr);
                  g_queuedLocalCmd.arg = String(argStr);
                }
              }
            }
          }
          http.end();
        }

        if (!ok) {
          failCount++;
          if (failCount >= 6) {
            failCount = 0;
            Serial.println("[FARM] Master'a ulasilamadi, agda yeni Master araniyor...");
            IPAddress newMaster = discoverMasterOnLan(IPAddress(0, 0, 0, 0), false);
            if (newMaster != IPAddress(0, 0, 0, 0)) {
              g_pendingNewMasterIp = newMaster;
              g_pendingSwitchMaster = true;
            } else {
              // Yukselmeden once carpismayi onlemek icin MAC bazli deterministik bekleme
              uint8_t m[6] = {0};
              WiFi.macAddress(m);
              vTaskDelay(((m[5] % 10) * 350 + 200) / portTICK_PERIOD_MS);
              IPAddress recheck = discoverMasterOnLan(IPAddress(0, 0, 0, 0), false);
              if (recheck != IPAddress(0, 0, 0, 0)) {
                g_pendingNewMasterIp = recheck;
                g_pendingSwitchMaster = true;
              } else {
                Serial.println("[FARM] Master bulunamadi -> Bu cihaz MASTER olarak yukseliyor!");
                g_pendingBecomeMaster = true;
              }
            }
          }
        }
      }
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

// ============================================================================
// Ana Baslatma ve Dongu Giris Noktalari
// ============================================================================
void farmInitAfterWifi(IPAddress knownMasterIp) {
  farmEnsureDeviceName(&Settings);

  if (knownMasterIp == IPAddress(0, 0, 0, 0)) {
    // Ayni anda prize takilan cihazlarin yarisini onlemek icin MAC bazli jitter
    uint8_t m[6] = {0};
    WiFi.macAddress(m);
    uint32_t jitterMs = (m[5] % 12) * 180;
    delay(jitterMs);
  }

  IPAddress masterIp = discoverMasterOnLan(knownMasterIp, true);
  if (masterIp != IPAddress(0, 0, 0, 0)) {
    farmBecomeWorker(masterIp);
  } else {
    farmBecomeMaster();
  }

  if (g_farmTaskHandle == nullptr) {
    xTaskCreatePinnedToCore(runFarmSyncTask, "FarmSync", 6144, NULL, 2, &g_farmTaskHandle, 0);
  }
}

void farmLoop() {
  uint32_t now = millis();

  if (g_pendingRestartAt != 0 && (int32_t)(now - g_pendingRestartAt) >= 0) {
    g_pendingRestartAt = 0;
    ESP.restart();
  }
  if (g_pendingResetAt != 0 && (int32_t)(now - g_pendingResetAt) >= 0) {
    g_pendingResetAt = 0;
    reset_configuration();
  }

  if (g_pendingSwitchMaster) {
    g_pendingSwitchMaster = false;
    farmBecomeWorker(g_pendingNewMasterIp);
  } else if (g_pendingBecomeMaster) {
    g_pendingBecomeMaster = false;
    farmBecomeMaster();
  }

  if (g_farmServer) {
    g_farmServer->handleClient();
  }

  otaCheckPeriodic();

  // Worker -> Master senkronizasyon verilerini SPIFFS / ana loop uzerinde guvenle uygula
  PendingConfigApply cfgCopy;
  QueuedLocalCmd cmdCopy;
  {
    std::lock_guard<std::mutex> lock(g_farmMutex);
    if (g_pendingCfg.hasUpdate) {
      cfgCopy = g_pendingCfg;
      g_pendingCfg.hasUpdate = false;
    }
    if (g_queuedLocalCmd.pending) {
      cmdCopy = g_queuedLocalCmd;
      g_queuedLocalCmd.pending = false;
    }
  }

  if (cfgCopy.hasUpdate) {
    Settings.PoolAddress = cfgCopy.pool;
    Settings.PoolPort = cfgCopy.port;
    strlcpy(Settings.PoolPassword, cfgCopy.poolPass.c_str(), sizeof(Settings.PoolPassword));
    strlcpy(Settings.BtcWallet, cfgCopy.wallet.c_str(), sizeof(Settings.BtcWallet));
    Settings.Timezone = cfgCopy.tz;
    Settings.saveStats = cfgCopy.saveStats;
    Settings.farmConfigVer = cfgCopy.cfgVer;
    nvMem.saveConfig(&Settings);
    configTime(3600 * Settings.Timezone, 0, "pool.ntp.org", "time.google.com");
    g_stratumReconnectNeeded = true;
    Serial.printf("[FARM] Ayarlar Master ile esitlendi (v.%lu)\n", (unsigned long)Settings.farmConfigVer);
  }

  if (cmdCopy.pending && cmdCopy.cmdId != g_lastExecCmdId) {
    g_lastExecCmdId = cmdCopy.cmdId;
    executeLocalCmd(cmdCopy.cmd, cmdCopy.arg);
    g_syncNowFlag = true;
  }

  if (g_role == FARM_ROLE_MASTER) {
    processMasterEspNowRequest();

    if (g_discUdpActive) {
      int pkt = g_discUdp.parsePacket();
      if (pkt > 0) {
        char buf[160];
        int n = g_discUdp.read(buf, sizeof(buf) - 1);
        if (n > 0) {
          buf[n] = '\0';
          String msg(buf);
          if (msg == "3DBRINT_WHO_IS_MASTER") {
            g_discUdp.beginPacket(g_discUdp.remoteIP(), g_discUdp.remotePort());
            g_discUdp.printf("3DBRINT_MASTER|%s|%s", WiFi.localIP().toString().c_str(), farmGetMacString().c_str());
            g_discUdp.endPacket();
          } else if (msg.startsWith("3DBRINT_MASTER_BEACON|")) {
            int p1 = msg.indexOf('|');
            int p2 = msg.indexOf('|', p1 + 1);
            if (p2 > p1) {
              String otherIpStr = msg.substring(p1 + 1, p2);
              String otherMac = msg.substring(p2 + 1);
              String myMac = farmGetMacString();
              if (otherMac.length() > 0 && otherMac != myMac && otherMac < myMac) {
                IPAddress otherIp;
                if (otherIp.fromString(otherIpStr) && probeMasterHttp(otherIp)) {
                  Serial.printf("[FARM] Daha dusuk MAC'li Master (%s) algilandi -> Worker moduna geciliyor\n", otherIpStr.c_str());
                  farmBecomeWorker(otherIp);
                  return;
                }
              }
            }
          }
        }
      }
      if (now - g_lastBeaconMs >= 15000UL) {
        sendMasterBeacon();
      }
    }
  } else if (g_role == FARM_ROLE_WORKER) {
    if (g_cmdUdpActive) {
      int pkt = g_cmdUdp.parsePacket();
      if (pkt > 0) {
        char buf[160];
        int n = g_cmdUdp.read(buf, sizeof(buf) - 1);
        if (n > 0) {
          buf[n] = '\0';
          String msg(buf);
          if (msg == "3DBRINT_SYNC") {
            g_syncNowFlag = true;
          } else if (msg.startsWith("3DBRINT_CMD|")) {
            int p1 = msg.indexOf('|');
            int p2 = msg.indexOf('|', p1 + 1);
            int p3 = msg.indexOf('|', p2 + 1);
            int p4 = msg.indexOf('|', p3 + 1);
            if (p4 > p3 && p3 > p2 && p2 > p1) {
              String mac = msg.substring(p1 + 1, p2);
              String cmd = msg.substring(p2 + 1, p3);
              String arg = msg.substring(p3 + 1, p4);
              uint32_t cid = (uint32_t)msg.substring(p4 + 1).toInt();
              if ((mac == farmGetMacString() || mac == "ALL") && cid != 0 && cid != g_lastExecCmdId) {
                g_lastExecCmdId = cid;
                executeLocalCmd(cmd, arg);
                g_syncNowFlag = true;
              }
            }
          }
        }
      }
    }
  }
}
