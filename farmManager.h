#ifndef _FARM_MANAGER_H_
#define _FARM_MANAGER_H_

#include <Arduino.h>
#include <IPAddress.h>
#include "drivers/storage/storage.h"

#define FARM_FW_VERSION "3dbrint Farm v2.1"
#define FARM_UDP_DISCOVERY_PORT 33345
#define FARM_UDP_CMD_PORT 33346
#define MAX_FARM_NODES 24

#if defined(CUSTOM_ESP32_C3_NO_DISPLAY)
  #define FARM_VARIANT_NAME "Ekransiz"
#elif defined(ENABLE_CAP_BUTTONS)
  #define FARM_VARIANT_NAME "Butonlu"
#else
  #define FARM_VARIANT_NAME "Ekranli"
#endif

enum FarmRole {
  FARM_ROLE_NONE = 0,
  FARM_ROLE_MASTER = 1,
  FARM_ROLE_WORKER = 2
};

const char* farmGetDeviceName();
void farmEnsureDeviceName(TSettings* cfg);
String farmGetMacString();
FarmRole farmGetRole();
int farmGetOnlineCount();
bool farmIsIdentifying();

bool farmTryEspNowProvision(TSettings* cfg, String& outSsid, String& outPass, IPAddress& outMasterIp);
void farmInitAfterWifi(IPAddress knownMasterIp);
void farmLoop();

#endif // _FARM_MANAGER_H_
