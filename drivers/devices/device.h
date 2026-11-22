#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <Arduino.h>
#include "build_config.h"

#if defined(CONFIG_IDF_TARGET_ESP32C3)
  #if (CURRENT_BUILD_PROFILE == BUILD_PROFILE_HEADLESS) || defined(HEADLESS) || defined(CUSTOM_ESP32_C3_NO_DISPLAY)
    #include "customEsp32C3NoDisplay.h"
  #else
    #include "customEsp32C3Oled.h"
  #endif
#elif defined(CONFIG_IDF_TARGET_ESP32)
  #if (CURRENT_BUILD_PROFILE == BUILD_PROFILE_HEADLESS) || defined(HEADLESS) || defined(NO_DISPLAY) || defined(DEVKIT_HEADLESS)
    #include "esp32DevKit.h"
  #else
    #include "esp32DevKitOled.h"
  #endif
#elif defined(CUSTOM_ESP32_C3_NO_DISPLAY)
  #include "customEsp32C3NoDisplay.h"
#elif defined(CUSTOM_ESP32_C3_OLED)
  #include "customEsp32C3Oled.h"
#elif defined(DEVKITV1)
  #include "esp32DevKit.h"
#else
  #include "customEsp32C3Oled.h"
#endif

#endif // __DEVICE_H__
