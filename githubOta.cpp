#include "githubOta.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <Update.h>
#include <ArduinoJson.h>

#include "farmManager.h"
#include "mining.h"
#include "drivers/storage/storage.h"
#include "drivers/devices/device.h"

extern TSettings Settings;

static String g_lastCheckedVer = "";
static String g_latestBinUrl = "";
static String g_latestNotes = "";
static bool g_updateAvailable = false;
static unsigned long g_lastCheckTime = 0;
static bool g_otaInProgress = false;

String otaGetHardwareKey() {
  String chip = "";
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  chip = "esp32c3";
#elif defined(CONFIG_IDF_TARGET_ESP32)
  chip = "esp32";
#else
  chip = "esp32c3";
#endif

  String variant = "";
#if (CURRENT_BUILD_PROFILE == BUILD_PROFILE_HEADLESS) || defined(CUSTOM_ESP32_C3_NO_DISPLAY) || defined(NO_DISPLAY)
  variant = "headless";
#elif (CURRENT_BUILD_PROFILE == BUILD_PROFILE_BUTONLU) || defined(ENABLE_CAP_BUTTONS)
  variant = "butonlu";
#else
  variant = "oled";
#endif

  return chip + "_" + variant;
}

String otaGetHardwareTitle() {
  String title = "";
#if defined(CONFIG_IDF_TARGET_ESP32C3)
  title = "ESP32-C3 Supermini";
#elif defined(CONFIG_IDF_TARGET_ESP32)
  title = "ESP32 DevKit v1 Dual-Core";
#else
  title = "ESP32";
#endif

#if (CURRENT_BUILD_PROFILE == BUILD_PROFILE_HEADLESS) || defined(CUSTOM_ESP32_C3_NO_DISPLAY) || defined(NO_DISPLAY)
  title += " (Ekransiz / LED)";
#elif (CURRENT_BUILD_PROFILE == BUILD_PROFILE_BUTONLU) || defined(ENABLE_CAP_BUTTONS)
  title += " (0.96 OLED + Buton)";
#else
  title += " (0.96 OLED)";
#endif

  return title;
}

// Versiyon karsilastirma (v2.2.0 vs v2.1.0 vb.)
static bool isVersionNewer(const String& newV, const String& currentV) {
  String n = newV;
  String c = currentV;
  n.replace("v", ""); n.replace("V", ""); n.trim();
  c.replace("v", ""); c.replace("V", ""); c.trim();
  int nMajor = 0, nMinor = 0, nPatch = 0;
  int cMajor = 0, cMinor = 0, cPatch = 0;
  sscanf(n.c_str(), "%d.%d.%d", &nMajor, &nMinor, &nPatch);
  sscanf(c.c_str(), "%d.%d.%d", &cMajor, &cMinor, &cPatch);
  if (nMajor > cMajor) return true;
  if (nMajor == cMajor && nMinor > cMinor) return true;
  if (nMajor == cMajor && nMinor == cMinor && nPatch > cPatch) return true;
  return false;
}

bool otaCheckUpdate(String& outNewVer, String& outBinUrl, String& outNotes, bool& outHasUpdate) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] WiFi bagli degil, guncelleme kontrol edilemiyor.");
    outHasUpdate = false;
    return false;
  }

  Serial.println("[OTA] GitHub versiyonu sorgulaniyor: " GITHUB_OTA_MANIFEST_URL);
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(10000);

  HTTPClient https;
  if (!https.begin(client, GITHUB_OTA_MANIFEST_URL)) {
    Serial.println("[OTA] HTTPS baglantisi baslatilamadi!");
    outHasUpdate = false;
    return false;
  }

  https.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[OTA] HTTP Hatasi: %d\n", httpCode);
    https.end();
    outHasUpdate = false;
    return false;
  }

  String payload = https.getString();
  https.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[OTA] JSON parse hatasi: %s\n", err.c_str());
    outHasUpdate = false;
    return false;
  }

  String newVer = doc["version"] | "";
  String notes = doc["release_notes"] | "";
  String hwKey = otaGetHardwareKey();
  String binUrl = "";

  if (doc["firmware"].is<JsonObject>()) {
    binUrl = doc["firmware"][hwKey] | "";
    if (binUrl.length() == 0) {
      // Fallback
      binUrl = doc["firmware"]["default"] | "";
    }
  }

  if (binUrl.length() > 0 && !binUrl.startsWith("http")) {
    binUrl = "https://raw.githubusercontent.com/" GITHUB_OTA_DEFAULT_REPO "/" GITHUB_OTA_BRANCH "/" + binUrl;
  }

  outNewVer = newVer;
  outBinUrl = binUrl;
  outNotes = notes;

  String currentVerStr = "2.2.0";
  outHasUpdate = isVersionNewer(newVer, currentVerStr);

  g_lastCheckedVer = newVer;
  g_latestBinUrl = binUrl;
  g_latestNotes = notes;
  g_updateAvailable = outHasUpdate;
  g_lastCheckTime = millis();

  Serial.printf("[OTA] Kontrol Sonucu: Donanim=%s | Mevcut=%s | GitHub=%s | Guncelleme Var Mi=%s\n",
                hwKey.c_str(), currentVerStr.c_str(), newVer.c_str(), outHasUpdate ? "EVET" : "HAYIR");

  return true;
}

bool otaPerformGithubUpdate(const String& binUrl) {
  if (binUrl.length() == 0 || WiFi.status() != WL_CONNECTED) {
    return false;
  }

  g_otaInProgress = true;
  is_mining_paused_flag = true;
  Serial.printf("[OTA] Guncelleme baslatiliyor: %s\n", binUrl.c_str());

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(25000);

#if defined(LED_PIN) && defined(ACTIVE_LED)
  httpUpdate.setLedPin(LED_PIN, ACTIVE_LED);
#endif
  httpUpdate.rebootOnUpdate(false); // Once mesaji basalim sonra kendimiz restart ederiz

  t_httpUpdate_return ret = httpUpdate.update(client, binUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[OTA] Guncelleme Basarisiz! Hata (%d): %s\n",
                    httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      g_otaInProgress = false;
      is_mining_paused_flag = false;
      return false;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] Cihaz zaten guncel.");
      g_otaInProgress = false;
      is_mining_paused_flag = false;
      return false;

    case HTTP_UPDATE_OK:
      Serial.println("[OTA] Guncelleme Basarili! Yeniden baslatiliyor...");
      delay(1000);
      ESP.restart();
      return true;
  }

  g_otaInProgress = false;
  is_mining_paused_flag = false;
  return false;
}

void otaRegisterRoutes(WebServer* server) {
  if (!server) return;

  // OTA Durum Sorgulama
  server->on("/api/ota/status", HTTP_GET, [server]() {
    JsonDocument doc;
    doc["ok"] = true;
    doc["currentVer"] = "2.2.0";
    doc["hwKey"] = otaGetHardwareKey();
    doc["hwTitle"] = otaGetHardwareTitle();
    doc["latestVer"] = g_lastCheckedVer;
    doc["latestNotes"] = g_latestNotes;
    doc["hasUpdate"] = g_updateAvailable;
    doc["binUrl"] = g_latestBinUrl;
    doc["inProgress"] = g_otaInProgress;

    String out;
    serializeJson(doc, out);
    server->sendHeader(F("Cache-Control"), F("no-store"));
    server->send(200, F("application/json; charset=utf-8"), out);
  });

  // GitHub'dan Yeni Surum Sorgula (GET veya POST)
  server->on("/api/ota/check", HTTP_ANY, [server]() {
    String newVer, binUrl, notes;
    bool hasUpdate = false;
    bool ok = otaCheckUpdate(newVer, binUrl, notes, hasUpdate);

    JsonDocument doc;
    doc["ok"] = ok;
    doc["currentVer"] = "2.2.0";
    doc["hwKey"] = otaGetHardwareKey();
    doc["hwTitle"] = otaGetHardwareTitle();
    doc["latestVer"] = newVer;
    doc["latestNotes"] = notes;
    doc["hasUpdate"] = hasUpdate;
    doc["binUrl"] = binUrl;

    String out;
    serializeJson(doc, out);
    server->sendHeader(F("Cache-Control"), F("no-store"));
    server->send(200, F("application/json; charset=utf-8"), out);
  });

  // Bu Cihaz Icin GitHub OTA Baslat
  server->on("/api/ota/start", HTTP_POST, [server]() {
    if (g_otaInProgress) {
      server->send(400, F("application/json"), F("{\"ok\":false,\"msg\":\"Guncelleme zaten suruyor\"}"));
      return;
    }

    String url = g_latestBinUrl;
    if (server->hasArg("url")) {
      url = server->arg("url");
    }

    if (url.length() == 0) {
      String newVer, notes;
      bool hasUpdate;
      otaCheckUpdate(newVer, url, notes, hasUpdate);
    }

    if (url.length() == 0) {
      server->send(400, F("application/json"), F("{\"ok\":false,\"msg\":\"Firmware URL bulunamadi\"}"));
      return;
    }

    server->send(200, F("application/json"), F("{\"ok\":true,\"msg\":\"Guncelleme baslatildi, kart yeniden baslayacak...\"}"));

    // Async baslatabilmek icin kisa gecikme ile task aciyoruz
    xTaskCreate([](void* p) {
      String* pUrl = (String*)p;
      delay(500);
      otaPerformGithubUpdate(*pUrl);
      delete pUrl;
      vTaskDelete(NULL);
    }, "ota_task", 8192, new String(url), 5, NULL);
  });

  // Manuel Tarayicidan .bin Dosyasi Yukleme Formu (GET /update)
  server->on("/update", HTTP_GET, [server]() {
    String html = F("<!DOCTYPE html><html><head><meta charset='utf-8'><title>Manuel OTA Guncelleme</title>"
                    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                    "<style>body{background:#0a0a0f;color:#eee;font-family:sans-serif;padding:24px;text-align:center;}"
                    ".box{max-width:440px;margin:30px auto;background:#15151e;padding:24px;border-radius:12px;border:1px solid #2a2a38;}"
                    "input[type=file]{margin:16px 0;padding:10px;background:#0d0d14;border:1px solid #333;border-radius:8px;color:#fff;width:100%;}"
                    "input[type=submit]{background:#f7931a;color:#000;border:0;padding:12px 24px;font-weight:bold;border-radius:8px;cursor:pointer;width:100%;font-size:15px;}"
                    "a{color:#f7931a;text-decoration:none;display:inline-block;margin-top:14px;}</style></head>"
                    "<body><div class='box'><h2>Manuel Firmware Yukleme</h2>"
                    "<p style='font-size:13px;color:#aaa;margin-top:8px;'>Derlenmis <b>.bin</b> dosyasini secip yukleyin.</p>"
                    "<form method='POST' action='/update' enctype='multipart/form-data'>"
                    "<input type='file' name='update' accept='.bin' required>"
                    "<input type='submit' value='Firmware Yukle ve Yeniden Baslat'>"
                    "</form><a href='/'>&larr; Farm Paneline Don</a></div></body></html>");
    server->send(200, F("text/html; charset=utf-8"), html);
  });

  // Manuel Tarayicidan .bin Dosyasi Yukleme (Web OTA POST)
  server->on("/update", HTTP_POST, [server]() {
    server->sendHeader("Connection", "close");
    server->send(200, "text/plain", (Update.hasError()) ? "Guncelleme Basarisiz!" : "Guncelleme Basarili! Yeniden baslatiliyor...");
    delay(1000);
    ESP.restart();
  }, [server]() {
    HTTPUpload& upload = server->upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[WEB OTA] Dosya yukleniyor: %s\n", upload.filename.c_str());
      is_mining_paused_flag = true;
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("[WEB OTA] Basarili: %u bayt yuklendi.\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
      is_mining_paused_flag = false;
    }
  });
}

void otaCheckPeriodic() {
  if (WiFi.status() != WL_CONNECTED || g_otaInProgress) return;
  // Her 12 saatte bir otomatik sorgula
  if (millis() - g_lastCheckTime > 43200000UL || g_lastCheckTime == 0) {
    String nv, bu, nt;
    bool hu;
    otaCheckUpdate(nv, bu, nt, hu);
  }
}
