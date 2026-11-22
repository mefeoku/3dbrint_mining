#include "drivers/displays/displayDriver.h"

#ifdef OLED_SSD1306_128X64_DISPLAY

#include <U8g2lib.h>
#include <WiFi.h>
#include <qrcode.h>
#include "monitor.h"
#include "wManager.h"
#include "mining.h"
#include "farmManager.h"
#include "drivers/storage/storage.h"

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Özel Bitcoin Logosu (16x16)
static const unsigned char btc_logo_16x16[] U8X8_PROGMEM = {
  0x00, 0x00, 0xe0, 0x07, 0x18, 0x18, 0x24, 0x24, 
  0x24, 0x27, 0x24, 0x27, 0x24, 0x24, 0x24, 0x27, 
  0x24, 0x27, 0x24, 0x27, 0x24, 0x24, 0x24, 0x27, 
  0x24, 0x24, 0x18, 0x18, 0xe0, 0x07, 0x00, 0x00
};

bool oled_power_state = true;
extern volatile bool is_mining_paused_flag;

bool getDisplayPowerState(void) {
  return oled_power_state;
}

void ssd1306_clearScreen(void) {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

void serialPrint_ssd1306(unsigned long mElapsed) {
  mining_data data = getMiningData(mElapsed);
  Serial.printf(">>> [BTC Solo] Hashrate: %s KH/s | Shares: %s | Best Diff: %s | Temp: %s C\n",
                data.currentHashRate.c_str(), data.completedShares.c_str(), data.bestDiff.c_str(), data.temp.c_str());
}

void ssd1306Display_Init(void) {
  Serial.println("SSD1306 128x64 display driver initialized (Pin 8 SDA, Pin 9 SCL)");
  Wire.begin(SDA_PIN, SCL_PIN, 400000);
  u8g2.begin();
  u8g2.setFontMode(1);
  u8g2.setBitmapMode(1);
  ssd1306_clearScreen();
}

void drawStatusScreen(const char* title, const char* line1, const char* line2) {
  u8g2.clearBuffer();
  u8g2.drawRBox(0, 0, 128, 13, 2);
  u8g2.setFontMode(0);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setDrawColor(0);
  int tw = u8g2.getStrWidth(title);
  int tx = (tw < 124) ? (128 - tw) / 2 : 2;
  u8g2.drawStr(tx, 10, title);
  u8g2.setDrawColor(1);
  u8g2.setFontMode(1);

  u8g2.drawRFrame(0, 16, 128, 48, 3);
  u8g2.setFont(u8g2_font_6x10_tr);
  if (line1 && line1[0]) {
    int w1 = u8g2.getStrWidth(line1);
    u8g2.drawStr((w1 < 120) ? (128 - w1) / 2 : 4, 34, line1);
  }
  u8g2.setFont(u8g2_font_5x7_tr);
  if (line2 && line2[0]) {
    int w2 = u8g2.getStrWidth(line2);
    u8g2.drawStr((w2 < 120) ? (128 - w2) / 2 : 4, 50, line2);
  }
  u8g2.sendBuffer();
}

void ssd1306Display_AlternateScreenState(void) {
  oled_power_state = !oled_power_state;
  if (oled_power_state) {
    u8g2.setPowerSave(0);
  } else {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    u8g2.setPowerSave(1);
  }
}

void ssd1306Display_AlternateRotation(void) {}

extern uint32_t pool_ping_ms;
extern TSettings Settings;

static bool ssd1306_handleIdentify(void) {
  static bool wasIdentifying = false;
  if (farmIsIdentifying()) {
    wasIdentifying = true;
    if (!oled_power_state) u8g2.setPowerSave(0);
    bool flashInvert = ((millis() / 250) % 2) == 0;
    u8g2.clearBuffer();
    if (flashInvert) {
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, 0, 128, 64);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
      u8g2.drawRFrame(0, 0, 128, 64, 3);
    }
    u8g2.setFont(u8g2_font_7x14_tr);
    const char* title = "BURADAYIM!";
    int tw = u8g2.getStrWidth(title);
    u8g2.drawStr((128 - tw) / 2, 22, title);

    u8g2.setFont(u8g2_font_6x10_tr);
    const char* devName = farmGetDeviceName();
    int nw = u8g2.getStrWidth(devName);
    u8g2.drawStr((nw < 124) ? (128 - nw) / 2 : 2, 40, devName);

    String ipStr = WiFi.localIP().toString();
    u8g2.setFont(u8g2_font_5x7_tr);
    int iw = u8g2.getStrWidth(ipStr.c_str());
    u8g2.drawStr((128 - iw) / 2, 55, ipStr.c_str());

    u8g2.setDrawColor(1);
    u8g2.sendBuffer();
    return true;
  } else if (wasIdentifying) {
    wasIdentifying = false;
    if (!oled_power_state) {
      ssd1306_clearScreen();
      u8g2.setPowerSave(1);
    }
  }
  return false;
}

// ==========================================
// EKRAN 1: Cyber Bitcoin Solo Miner HUD
// ==========================================
void ssd1306Display_Screen1(unsigned long mElapsed) {
  if (ssd1306_handleIdentify()) return;
  if (!oled_power_state) return;
  mining_data data = getMiningData(mElapsed);

  u8g2.clearBuffer();

  // 1. ÜST HEADER BAR (WiFi kopma / Duraklatma / Farm / Solo rozeti)
  if (WiFi.status() != WL_CONNECTED) {
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setFontMode(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(0);
    u8g2.drawStr(12, 9, "! BAGLANTI KESILDI !");
    u8g2.setDrawColor(1);
    u8g2.setFontMode(1);
  } else if (is_mining_paused_flag) {
    u8g2.drawRBox(0, 0, 128, 12, 2);
    u8g2.setFontMode(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(0);
    u8g2.drawStr(16, 9, "|| DURAKLATILDI ||");
    u8g2.setDrawColor(1);
    u8g2.setFontMode(1);
  } else {
    String badge = "SOLO";
    FarmRole role = farmGetRole();
    int onlineCnt = farmGetOnlineCount();
    if (role == FARM_ROLE_MASTER && onlineCnt > 1) {
      badge = "F:" + String(onlineCnt);
    } else if (role == FARM_ROLE_WORKER) {
      badge = "FARM";
    }
    u8g2.drawRBox(0, 0, 34, 11, 2);
    u8g2.setFontMode(0);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.setDrawColor(0);
    int bw = u8g2.getStrWidth(badge.c_str());
    u8g2.drawStr((34 - bw) / 2, 8, badge.c_str());
    u8g2.setDrawColor(1);
    u8g2.setFontMode(1);

    // Başlık & Logo
    u8g2.drawXBMP(38, 0, 16, 16, btc_logo_16x16);
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(56, 9, "BITCOIN");

    // Stratum Ping (ms)
    u8g2.setFont(u8g2_font_4x6_tr);
    String pingStr = (pool_ping_ms > 0) ? String(pool_ping_ms) + "ms" : "--ms";
    int pw = u8g2.getStrWidth(pingStr.c_str());
    u8g2.drawStr((126 - pw), 9, pingStr.c_str());
  }

  // Üst çizgi
  u8g2.drawHLine(0, 13, 128);

  // 2. HASHRATE (Büyük ve Havalı)
  u8g2.setFont(u8g2_font_logisoso20_tn);
  u8g2.setCursor(0, 37);
  u8g2.print(data.currentHashRate);

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(72, 26, "kH/s");

  // En İyi Zorluk (Best Diff)
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(72, 37);
  String diffText = (data.bestDiff == "0.0000" || data.bestDiff == "0.0") ? "0.0" : data.bestDiff;
  u8g2.print("Diff:" + diffText);

  // 3. ALT ÇERÇEVE: SHARES, UPTIME & SICAKLIK
  u8g2.drawRFrame(0, 43, 128, 21, 2);

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(4, 52);
  u8g2.print("Shares: " + data.completedShares);

  u8g2.setCursor(4, 61);
  u8g2.print("Valid: " + data.valids);

  u8g2.setCursor(68, 52);
  u8g2.print("Up: " + data.timeMining);

  u8g2.setCursor(68, 61);
  u8g2.print("T: " + data.temp + "C");

  u8g2.sendBuffer();
  serialPrint_ssd1306(mElapsed);
}

// ==========================================
// EKRAN 2: Blok & Ağ İstatistikleri Kartı
// ==========================================
void ssd1306Display_Screen2(unsigned long mElapsed) {
  if (ssd1306_handleIdentify()) return;
  if (!oled_power_state) return;
  coin_data coin = getCoinData(mElapsed);

  u8g2.clearBuffer();

  // Header
  u8g2.drawRBox(0, 0, 128, 12, 2);
  u8g2.setFontMode(0);
  u8g2.setFont(u8g2_font_6x10_tr);
  u8g2.setDrawColor(0);
  u8g2.drawStr(20, 9, "BTC AG DURUMU");
  u8g2.setDrawColor(1);
  u8g2.setFontMode(1);

  u8g2.setFont(u8g2_font_5x7_tr);
  
  u8g2.setCursor(4, 25);
  u8g2.print("Blok   : " + coin.blockHeight);

  u8g2.setCursor(4, 36);
  u8g2.print("Zorluk : " + coin.netwrokDifficulty);

  u8g2.setCursor(4, 47);
  u8g2.print("Ag Hizi: " + coin.globalHashRate + " EH/s");

  u8g2.drawHLine(0, 51, 128);

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(4, 61);
  u8g2.print("Havuz  : " + Settings.PoolAddress);

  u8g2.sendBuffer();
  serialPrint_ssd1306(mElapsed);
}

// ==========================================
// EKRAN 3: Büyük Estetik Canlı Saat & Tarih
// ==========================================
void ssd1306Display_Screen3(unsigned long mElapsed) {
  if (ssd1306_handleIdentify()) return;
  if (!oled_power_state) return;
  clock_data cData = getClockData(mElapsed);
  clock_data_t ct = getClockData_t(mElapsed);

  char liveTime[16];
  sprintf(liveTime, "%02lu:%02lu:%02lu", ct.currentHours, ct.currentMinutes, ct.currentSeconds);

  u8g2.clearBuffer();

  // Şık dış çerçeve
  u8g2.drawRFrame(0, 0, 128, 64, 3);

  // Büyük Canlı Saat (Merkezde)
  u8g2.setFont(u8g2_font_logisoso20_tr);
  int tw = u8g2.getStrWidth(liveTime);
  u8g2.setCursor((tw < 124) ? (128 - tw) / 2 : 2, 34);
  u8g2.print(liveTime);

  // İnce estetik ayırıcı çizgi
  u8g2.drawHLine(14, 40, 100);

  // Büyük Tarih (Merkezde alt satır)
  u8g2.setFont(u8g2_font_7x14_tr);
  int dw = u8g2.getStrWidth(cData.currentDate.c_str());
  u8g2.setCursor((dw < 124) ? (128 - dw) / 2 : 2, 57);
  u8g2.print(cData.currentDate);

  u8g2.sendBuffer();
}

// Yükleme Ekranı (Dinamik Aşamalı Boot HUD)
void ssd1306_showLoadingProgress(const char* statusText, int progressPercent) {
  if (!oled_power_state) return;
  u8g2.clearBuffer();

  // 1. Bitcoin Logo (Merkez üst)
  u8g2.drawXBMP(56, 2, 16, 16, btc_logo_16x16);

  // 2. Başlık
  u8g2.setFont(u8g2_font_6x10_tr);
  int bw = u8g2.getStrWidth("3DBRINT MINER");
  u8g2.drawStr((128 - bw) / 2, 27, "3DBRINT MINER");

  // 3. Durum Metni (Ne yaptığını yazan satır)
  u8g2.setFont(u8g2_font_5x7_tr);
  if (statusText && statusText[0]) {
    int sw = u8g2.getStrWidth(statusText);
    u8g2.drawStr((sw < 124) ? (128 - sw) / 2 : 2, 41, statusText);
  }

  // 4. İlerleme Çubuğu (Progress Bar)
  u8g2.drawRFrame(14, 48, 100, 9, 2);
  if (progressPercent > 0) {
    if (progressPercent > 100) progressPercent = 100;
    int fill = (progressPercent * 96) / 100;
    u8g2.drawBox(16, 50, fill, 5);
  }

  u8g2.sendBuffer();
}

void ssd1306Display_LoadingScreen(void) {
  ssd1306_showLoadingProgress("Sistem Baslatiliyor...", 15);
}

static String escapeWifiQrField(const char* src) {
  String out;
  if (!src) return out;
  for (size_t i = 0; src[i] != '\0'; i++) {
    char c = src[i];
    if (c == '\\' || c == ';' || c == ',' || c == '"' || c == ':') {
      out += '\\';
    }
    out += c;
  }
  return out;
}

static void ssd1306_drawQrCallback(esp_qrcode_handle_t qrcode) {
  int size = esp_qrcode_get_size(qrcode);
  int scale = (size <= 30) ? 2 : 1;
  int qrPx = size * scale;
  int ox = (64 - qrPx) / 2;
  int oy = (64 - qrPx) / 2;

  // Sol yari: yuksek kontrastli beyaz quiet zone uzerine siyah QR modulleri
  u8g2.setDrawColor(1);
  u8g2.drawBox(0, 0, 64, 64);

  u8g2.setDrawColor(0);
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      if (esp_qrcode_get_module(qrcode, x, y)) {
        u8g2.drawBox(ox + x * scale, oy + y * scale, scale, scale);
      }
    }
  }
  u8g2.setDrawColor(1);
}

// Kurulum Ekranı (Solda Taranabilir WiFi QR Kodu, Sağda Bilgi)
void ssd1306Display_SetupScreen(void) {
  const char* apName = getSetupApName();
  if (!apName || apName[0] == '\0') apName = DEFAULT_SSID;

  String qrPayload = "WIFI:T:WPA;S:" + escapeWifiQrField(apName) +
                     ";P:" + escapeWifiQrField(DEFAULT_WIFIPW) + ";;";

  u8g2.clearBuffer();

  esp_qrcode_config_t cfg = {
    .display_func = ssd1306_drawQrCallback,
    .max_qrcode_version = 4,
    .qrcode_ecc_level = (qrPayload.length() <= 42) ? ESP_QRCODE_ECC_MED : ESP_QRCODE_ECC_LOW
  };
  esp_qrcode_generate(&cfg, qrPayload.c_str());

  // Sag yari: KURULUM rozeti ve hizli bilgiler
  u8g2.drawRBox(67, 0, 61, 12, 2);
  u8g2.setFontMode(0);
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setDrawColor(0);
  u8g2.drawStr(80, 9, "KURULUM");
  u8g2.setDrawColor(1);
  u8g2.setFontMode(1);

  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.drawStr(68, 23, "QR OKUT &");
  u8g2.drawStr(68, 32, "BAGLAN!");

  u8g2.drawHLine(67, 36, 61);

  String apShort = String(apName);
  if (apShort.length() > 15) apShort = apShort.substring(0, 15);

  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.drawStr(67, 45, apShort.c_str());
  u8g2.drawStr(67, 53, DEFAULT_WIFIPW);
  u8g2.drawStr(67, 62, "3dbrint.local");

  u8g2.sendBuffer();
}

void ssd1306Display_DoLedStuff(unsigned long frame) {}
void ssd1306Display_AnimateCurrentScreen(unsigned long frame) {}

CyclicScreenFunction ssd1306DisplayCyclicScreens[] = {
    ssd1306Display_Screen1,
    ssd1306Display_Screen2,
    ssd1306Display_Screen3
};

DisplayDriver ssd1306DisplayDriver = {
    ssd1306Display_Init,
    ssd1306Display_AlternateScreenState,
    ssd1306Display_AlternateRotation,
    ssd1306Display_LoadingScreen,
    ssd1306Display_SetupScreen,
    ssd1306DisplayCyclicScreens,
    ssd1306Display_AnimateCurrentScreen,
    ssd1306Display_DoLedStuff,
    SCREENS_ARRAY_SIZE(ssd1306DisplayCyclicScreens),
    0,
    128,
    64,
};

#endif
