#ifndef _CUSTOM_ESP32_C3_OLED_H
#define _CUSTOM_ESP32_C3_OLED_H

// ===== ESP32-C3 Supermini Özel Kasa Ayarları =====

// I2C Pinleri (SSD1306 OLED)
#define SDA_PIN 8
#define SCL_PIN 9

// 0.96 inch SSD1306 128x64 OLED ekran
#define OLED_SSD1306_128X64_DISPLAY

// Seri monitor hızı
#define MONITOR_SPEED 115200

// Buton (OneButton kütüphanesi için gerekli - kullanılmasa da tanımlanmalı)
// Kapasitif butonlarımız var ama OneButton compile edebilmek için
// PIN_BUTTON_1 tanımı gerekiyor
#define PIN_BUTTON_1 0

// Kapasitif dokunmatik pinler (gerçekte kullandığımız butonlar)
#define PIN_CAP_LEFT  4
#define PIN_CAP_RIGHT 3

// Not: GPIO 8 dahili LED ile paylasimlidir ve yukarida I2C SDA (OLED) olarak kullanilmaktadir.
// Cakismayi onlemek icin LED_PIN tanimlanmaz.

#endif
