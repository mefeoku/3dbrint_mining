#ifndef _CUSTOM_ESP32_C3_NO_DISPLAY_H
#define _CUSTOM_ESP32_C3_NO_DISPLAY_H

// ===== ESP32-C3 Supermini Ekransiz (Headless) Ayarlari =====

#define NO_DISPLAY

// Dahili Mavi LED (ESP32-C3 Supermini Pin 8)
#define LED_PIN 8
#define ACTIVE_LED LOW
#define INACTIVE_LED HIGH

// Seri monitor hizi
#define MONITOR_SPEED 115200

// Buton (OneButton kütüphanesi için dummy pin)
#define PIN_BUTTON_1 0

#endif
