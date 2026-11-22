#include "drivers/displays/displayDriver.h"

#ifdef NO_DISPLAY

#include <Arduino.h>
#include "monitor.h"
#include "wManager.h"
#include "mining.h"
#include "farmManager.h"

extern monitor_data mMonitor;
bool ledOn = true;

void noDisplay_Init(void)
{
  Serial.println("No display driver initialized (ESP32-C3 LED Pin 8)");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, INACTIVE_LED);
}

void noDisplay_AlternateScreenState(void)
{
  Serial.println("Switching LED state");
  ledOn = !ledOn;
  if (!ledOn) {
    digitalWrite(LED_PIN, INACTIVE_LED);
  }
}

void noDisplay_AlternateRotation(void)
{
}

bool getDisplayPowerState(void)
{
  return ledOn;
}

void drawStatusScreen(const char* title, const char* line1, const char* line2)
{
  Serial.printf("[DURUM] %s | %s | %s\n", title ? title : "", line1 ? line1 : "", line2 ? line2 : "");
}

void noDisplay_NoScreen(unsigned long mElapsed)
{
  mining_data data = getMiningData(mElapsed);

  Serial.printf(">>> [BTC Solo Ekransiz] Hashrate: %s KH/s | Shares: %s | Valid: %s | Best Diff: %s | Temp: %s C\n",
                data.currentHashRate.c_str(), data.completedShares.c_str(), data.valids.c_str(), data.bestDiff.c_str(), data.temp.c_str());
}

void noDisplay_LoadingScreen(void)
{
  Serial.println("Initializing...");
}

void noDisplay_SetupScreen(void)
{
  Serial.println("Setup Portal Active (AP: 3dbrint-Miner)...");
}

static unsigned long previousMillis = 0;

void noDisplay_DoLedStuff(unsigned long frame)
{
  unsigned long currentMillis = millis();

  // Farm "Bul" (Identify) komutu: LED kapali olsa bile hizli flaş yapar
  if (farmIsIdentifying())
  {
    if (currentMillis - previousMillis >= 40)
    {
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    return;
  }

  if (!ledOn)
  {
    digitalWrite(LED_PIN, INACTIVE_LED);
    return;
  }

  if (is_mining_paused_flag)
  {
    if (currentMillis - previousMillis >= 1500)
    {
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    return;
  }

  switch (mMonitor.NerdStatus)
  {
  case NM_waitingConfig:
    digitalWrite(LED_PIN, ACTIVE_LED);
    break;

  case NM_Connecting:
    if (currentMillis - previousMillis >= 500)
    {
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    break;

  case NM_hashing:
    if (currentMillis - previousMillis >= 100)
    {
      previousMillis = currentMillis;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }
    break;
  }
}

void noDisplay_AnimateCurrentScreen(unsigned long frame)
{
}

CyclicScreenFunction noDisplayCyclicScreens[] = {noDisplay_NoScreen};

DisplayDriver noDisplayDriver = {
    noDisplay_Init,
    noDisplay_AlternateScreenState,
    noDisplay_AlternateRotation,
    noDisplay_LoadingScreen,
    noDisplay_SetupScreen,
    noDisplayCyclicScreens,
    noDisplay_AnimateCurrentScreen,
    noDisplay_DoLedStuff,
    SCREENS_ARRAY_SIZE(noDisplayCyclicScreens),
    0,
    0,
    0,
};

#endif
