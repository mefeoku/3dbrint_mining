#define ESP_DRD_USE_SPIFFS true

// Include Libraries
//#include ".h"

#include <WiFi.h>
#include <ESPmDNS.h>

#include <WiFiManager.h>

#include "wManager.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"
#include "mining.h"
#include "timeconst.h"

#include <ArduinoJson.h>
#include <vector>

#include "setupPortal.h"
#include "jsqr_gz.h"
#include "farmManager.h"


// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
TSettings Settings;

// Define WiFiManager Object
WiFiManager wm;
extern monitor_data mMonitor;

nvMemory nvMem;

extern SDCard SDCrd;

static String g_setupApName = DEFAULT_SSID;

const char* getSetupApName() {
    return g_setupApName.c_str();
}

static String htmlEscape(const String& s) {
    String out;
    out.reserve(s.length() + 8);
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (c == '&') out += F("&amp;");
        else if (c == '"') out += F("&quot;");
        else if (c == '<') out += F("&lt;");
        else if (c == '>') out += F("&gt;");
        else out += c;
    }
    return out;
}

static bool handleCaptiveRedirect() {
    if (!wm.getConfigPortalActive() || !wm.server) return false;
    String apIp = WiFi.softAPIP().toString();
    String host = wm.server->hostHeader();
    if (host.length() > 0 && host != apIp) {
        wm.server->sendHeader(F("Location"), String(F("http://")) + apIp + F("/"), true);
        wm.server->send(302, F("text/plain"), F(""));
        wm.server->client().stop();
        return true;
    }
    return false;
}

static void handleCustomSetupPage() {
    if (handleCaptiveRedirect()) return;
    if (!wm.server) return;

    String html = FPSTR(SETUP_PAGE_HTML);
    String savedSsid = wm.getWiFiSSID(true);
    String btcVal = (strcmp(Settings.BtcWallet, DEFAULT_WALLETID) == 0) ? String("") : String(Settings.BtcWallet);

    html.replace(F("{{SSID}}"), htmlEscape(savedSsid));
    html.replace(F("{{BTC}}"), htmlEscape(btcVal));
    html.replace(F("{{POOL}}"), htmlEscape(Settings.PoolAddress));
    html.replace(F("{{PORT}}"), String(Settings.PoolPort));
    html.replace(F("{{PPASS}}"), htmlEscape(String(Settings.PoolPassword)));
    html.replace(F("{{TZ}}"), String(Settings.Timezone));
    html.replace(F("{{STATS_CHK}}"), Settings.saveStats ? F("checked") : F(""));

    wm.server->sendHeader(F("Cache-Control"), F("no-cache, no-store, must-revalidate"));
    wm.server->send(200, F("text/html; charset=utf-8"), html);
}

static void handleWifiScanJson() {
    if (!wm.server) return;
    int n = WiFi.scanNetworks(false, false);

    struct NetEntry {
        String ssid;
        int rssi;
        bool locked;
    };
    std::vector<NetEntry> nets;

    for (int i = 0; i < n; i++) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() == 0) continue;
        int rssi = WiFi.RSSI(i);
        bool locked = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        bool found = false;
        for (auto& existing : nets) {
            if (existing.ssid == ssid) {
                found = true;
                if (rssi > existing.rssi) {
                    existing.rssi = rssi;
                    existing.locked = locked;
                }
                break;
            }
        }
        if (!found) {
            nets.push_back({ssid, rssi, locked});
        }
    }
    WiFi.scanDelete();

    for (size_t i = 0; i < nets.size(); i++) {
        for (size_t j = i + 1; j < nets.size(); j++) {
            if (nets[j].rssi > nets[i].rssi) {
                std::swap(nets[i], nets[j]);
            }
        }
    }

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    size_t limit = nets.size() > 15 ? 15 : nets.size();
    for (size_t i = 0; i < limit; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["s"] = nets[i].ssid;
        obj["r"] = wm.getRSSIasQuality(nets[i].rssi);
        obj["l"] = nets[i].locked ? 1 : 0;
    }

    String out;
    serializeJson(arr, out);
    wm.server->sendHeader(F("Cache-Control"), F("no-cache"));
    wm.server->send(200, F("application/json; charset=utf-8"), out);
}

static void handleJsQr() {
    if (!wm.server) return;
    wm.server->sendHeader(F("Content-Encoding"), F("gzip"));
    wm.server->sendHeader(F("Cache-Control"), F("public, max-age=86400"));
    wm.server->send_P(200, "application/javascript", reinterpret_cast<const char*>(JSQR_GZ), JSQR_GZ_LEN);
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
    shouldSaveConfig = true;    
    //wm.setConfigPortalBlocking(false);
}

/* void saveParamsCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
    nvMem.saveConfig(&Settings);
} */

void configModeCallback(WiFiManager* myWiFiManager)
// Called when config mode launched
{
    Serial.println("Entered Configuration Mode");
    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    // Eger onceden kaydedilmis bir WiFi varsa ve baglanamadiysa kullaniciya bildir:
    if (WiFi.SSID().length() > 0) {
        drawStatusScreen("BAGLANTI HATASI", "WiFi Baglanamadi!", "Kurulum Modu...");
        delay(2500);
    }

    drawSetupScreen();
    Serial.print("Config SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());

    Serial.print("Config IP Address: ");
    Serial.println(WiFi.softAPIP());
}

void reset_configuration()
{
    Serial.println("Erasing Config, restarting");
    nvMem.deleteConfig();
    resetStat();
    wm.resetSettings();
    ESP.restart();
}

void init_WifiManager()
{
    g_setupApName = DEFAULT_SSID;
    const char* apName = g_setupApName.c_str();

    //Init pin 15 to eneble 5V external power (LilyGo bug)
#ifdef PIN_ENABLE5V
    pinMode(PIN_ENABLE5V, OUTPUT);
    digitalWrite(PIN_ENABLE5V, HIGH);
#endif

    // Change to true when testing to force configuration every time we run
    bool forceConfig = false;

#if defined(PIN_BUTTON_2)
    // Check if button2 is pressed to enter configMode with actual configuration
    if (!digitalRead(PIN_BUTTON_2)) {
        Serial.println(F("Button pressed to force start config mode"));
        forceConfig = true;
        wm.setBreakAfterConfig(true); //Set to detect config edition and save
    }
#endif
    // Explicitly set WiFi mode & limit TX power for ESP32-C3 Supermini stability
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(WIFI_POWER_8_5dBm);
    WiFi.setSleep(false);

    Serial.println("[BOOT] Loading config...");
    bool hasConfig = nvMem.loadConfig(&Settings);
    if (!hasConfig)
    {
        //No config file on internal flash.
        if (SDCrd.loadConfigFile(&Settings))
        {
            //Config file on SD card.
            SDCrd.SD2nvMemory(&nvMem, &Settings); // reboot on success.          
        }
        else
        {
            forceConfig = true;
        }
    }
    farmEnsureDeviceName(&Settings);
    
    // Free the memory from SDCard class 
    SDCrd.terminate();
    
    // Reset settings (only for development)
    //wm.resetSettings();

    // Set dark theme & custom portal look
    wm.setTitle("3dbrint Miner");
    wm.setHostname("3dbrint");
    wm.setDarkMode(true);
    wm.setCustomHeadElement(WM_DARK_HEAD);
    wm.setAPClientCheck(true);

    wm.setWebServerCallback([]() {
        if (!wm.server) return;
        wm.server->on("/", HTTP_GET, handleCustomSetupPage);
        wm.server->on("/wifi", HTTP_GET, handleCustomSetupPage);
        wm.server->on("/0wifi", HTTP_GET, handleCustomSetupPage);
        wm.server->on("/scan", HTTP_GET, handleWifiScanJson);
        wm.server->on("/qr.js", HTTP_GET, handleJsQr);
    });

    // Set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveConfigCallback);

    // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wm.setAPCallback(configModeCallback);    

    //Advanced settings
    wm.setConfigPortalBlocking(false); //Hacemos que el portal no bloquee el firmware
    wm.setConnectTimeout(40); // how long to try to connect for before continuing
    wm.setConfigPortalTimeout(300); // auto close configportal after n seconds
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap
    //wm.setTimeout(120);
    //wm.setConfigPortalTimeout(120); //seconds

    // Custom elements

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter pool_text_box("Poolurl", "Pool url", Settings.PoolAddress.c_str(), 80);

    // Need to convert numerical input to string to display the default value.
    char convertedValue[6];
    sprintf(convertedValue, "%d", Settings.PoolPort);

    // Text box (Number) - 7 characters maximum
    WiFiManagerParameter port_text_box_num("Poolport", "Pool port", convertedValue, 7);

    // Text box (String) - 80 characters maximum
    //WiFiManagerParameter password_text_box("Poolpassword", "Pool password (Optional)", Settings.PoolPassword, 80);

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter addr_text_box("btcAddress", "Your BTC address", Settings.BtcWallet, 80);

  // Text box (Number) - 2 characters maximum
  char charZone[6];
  sprintf(charZone, "%d", Settings.Timezone);
  WiFiManagerParameter time_text_box_num("TimeZone", "TimeZone fromUTC (-12/+12)", charZone, 3);

  WiFiManagerParameter features_html("<hr><br><label style=\"font-weight: bold;margin-bottom: 25px;display: inline-block;\">Features</label>");

  char checkboxParams[24] = "type=\"checkbox\"";
  if (Settings.saveStats)
  {
    strcat(checkboxParams, " checked");
  }
  WiFiManagerParameter save_stats_to_nvs("SaveStatsToNVS", "Save mining statistics to flash memory.", "T", 2, checkboxParams, WFM_LABEL_AFTER);
  // Text box (String) - 80 characters maximum
  WiFiManagerParameter password_text_box("Poolpass", "Pool password", Settings.PoolPassword, 80);

  // Add all defined parameters
  wm.addParameter(&pool_text_box);
  wm.addParameter(&port_text_box_num);
  wm.addParameter(&password_text_box);
  wm.addParameter(&addr_text_box);
  wm.addParameter(&time_text_box_num);
  wm.addParameter(&features_html);
  wm.addParameter(&save_stats_to_nvs);
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  char checkboxParams2[24] = "type=\"checkbox\"";
  if (Settings.invertColors)
  {
    strcat(checkboxParams2, " checked");
  }
  WiFiManagerParameter invertColors("inverColors", "Invert Display Colors (if the colors looks weird)", "T", 2, checkboxParams2, WFM_LABEL_AFTER);
  wm.addParameter(&invertColors);
  #endif
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
    char brightnessConvValue[2];
    sprintf(brightnessConvValue, "%d", Settings.Brightness);
    // Text box (Number) - 3 characters maximum
    WiFiManagerParameter brightness_text_box_num("Brightness", "Screen backlight Duty Cycle (0-255)", brightnessConvValue, 3);
    wm.addParameter(&brightness_text_box_num);
  #endif

    auto waitWifiConnected = [](uint32_t timeoutMs) -> bool {
        uint32_t startMs = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < timeoutMs) {
            doLedStuff(0);
            delay(250);
        }
        return (WiFi.status() == WL_CONNECTED);
    };

    Serial.println("AllDone: ");
    IPAddress provMasterIp(0, 0, 0, 0);
    String savedSsid = wm.getWiFiSSID(true);
    String savedPass = wm.getWiFiPass(true);
    String provSsid;
    String provPass;

    mMonitor.NerdStatus = NM_Connecting;
    doLedStuff(0);

    if (hasConfig && !forceConfig && savedSsid.length() > 0) {
        Serial.printf("[BOOT] Baglaniyor: %s\n", savedSsid.c_str());
        showLoadingProgress("WiFi Baglaniliyor...", 45);
        WiFi.persistent(true);
        WiFi.begin(savedSsid.c_str(), savedPass.c_str());
        if (!waitWifiConnected(14000)) {
            Serial.println("[BOOT] Kayitli WiFi baglanamadi, ESP-NOW ile 3dbrint.local araniyor...");
            showLoadingProgress("Farm Araniyor...", 50);
            if (farmTryEspNowProvision(&Settings, provSsid, provPass, provMasterIp)) {
                showLoadingProgress("Farm Bulundu!", 60);
                nvMem.saveConfig(&Settings);
                WiFi.persistent(true);
                WiFi.begin(provSsid.c_str(), provPass.c_str());
                waitWifiConnected(14000);
            }
        }
    } else {
        Serial.println("[BOOT] Ilk kurulum: ESP-NOW ile aktif 3dbrint.local Farm araniyor...");
        showLoadingProgress("Farm Araniyor...", 40);
        if (farmTryEspNowProvision(&Settings, provSsid, provPass, provMasterIp)) {
            showLoadingProgress("Farm Bulundu!", 55);
            nvMem.saveConfig(&Settings);
            WiFi.persistent(true);
            WiFi.begin(provSsid.c_str(), provPass.c_str());
            waitWifiConnected(14000);
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[BOOT] Master Farm bulunamadi -> Kurulum Portali aciliyor (3dbrint-Miner)...");
        wm.setConfigPortalTimeout(90);
        wm.setConfigPortalBlocking(true);
        drawSetupScreen();
        mMonitor.NerdStatus = NM_waitingConfig;
        doLedStuff(0);
        wm.startConfigPortal(apName, DEFAULT_WIFIPW);

        if (shouldSaveConfig) {
            Settings.PoolAddress = pool_text_box.getValue();
            Settings.PoolPort = atoi(port_text_box_num.getValue());
            strlcpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
            strlcpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
            Settings.Timezone = atoi(time_text_box_num.getValue());
            Settings.saveStats = (strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
            #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
                Settings.Brightness = atoi(brightness_text_box_num.getValue());
            #endif
            nvMem.saveConfig(&Settings);
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[BOOT] WiFi baglantisi kurulamadi, yeniden baslatiliyor (Farm tekrar aranacak)...");
            delay(1000);
            ESP.restart();
        }
    }

    if (wm.getConfigPortalActive()) {
        wm.stopConfigPortal();
    }

    if (WiFi.status() == WL_CONNECTED) {
        showLoadingProgress("WiFi Baglandi!", 70);
        delay(200);
        showLoadingProgress("Farm Kontrolu...", 80);
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        farmInitAfterWifi(provMasterIp);
    }
}

//----------------- MAIN PROCESS WIFI MANAGER --------------
int oldStatus = 0;

void wifiManagerProcess() {

    wm.process(); // avoid delays() in loop when non-blocking and other long running code
    farmLoop();

    int newStatus = WiFi.status();
    if (newStatus != oldStatus) {
        if (newStatus == WL_CONNECTED) {
            Serial.println("CONNECTED - Current ip: " + WiFi.localIP().toString());
        } else {
            Serial.print("[Error] - current status: ");
            Serial.println(newStatus);
        }
        oldStatus = newStatus;
    }
}
